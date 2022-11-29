#include "MqttPubSub.h"

MqttPubSub::MqttPubSub()
{
}

void MqttPubSub::setup()
{
  hostname = wifiSettings.hostname;
  strcat(channelStatus, hostname);
  strcat(channelStatus, "/status");
  strcat(channelIn, hostname);
  strcat(channelIn, "/in/#");
  strcat(channelOut, hostname);
  strcat(channelOut, "/out");

  client.setClient(espClient);
  client.setBufferSize(1024);
  client.setKeepAlive(30);
  client.setCallback(callback);
}

void MqttPubSub::callback(char *topic, byte *message, unsigned int length)
{
  char msg[length + 1];
  for (size_t i = 0; i < length; i++)
    msg[i] = (char)message[i];
  msg[length] = 0x0a; // add char to string

  String t = String(topic);
  String cmd = t.substring(String(wifiSettings.hostname).length() + 4, t.length());
  if (length > 0)
  {
    if (cmd == "restart" && String(msg).toInt() == 1)
      ESP.restart();
    else if (cmd == "reconnect" && String(msg).toInt() == 1)
      WiFi.disconnect(false, false);
    else if (cmd == "ivts")
      status.ivtsCommand = String(msg);
    else
    {
      for (size_t i = 0; i < SwitchCount; i++)
      {
        SwitchConfig *sc = &pinsSettings.switches[i];
        // find switch in settings and set status value by index
        if (String(sc->name).equals(cmd))
        {
          status.switches[i] = String(msg).toInt();
          break;
        }
      }
    }
    status.receivedCount++;
  }
}

bool MqttPubSub::reconnect()
{
  if (status.SSID != "")
  {
    if (status.SSID != currentMqttSettings[0])
    {
      for (size_t i = 0; i < wifiSettings.APsCount; i++)
      {
        if (status.SSID.equals(mqttSettings.Servers[i][0]))
        {
          currentMqttSettings[0] = mqttSettings.Servers[i][0];
          currentMqttSettings[1] = mqttSettings.Servers[i][1];
          if (currentMqttSettings[1] == "gateway")
            currentMqttSettings[1] = status.gatewayAddress;
          currentMqttSettings[2] = mqttSettings.Servers[i][2];
          currentMqttSettings[3] = mqttSettings.Servers[i][3];
          currentMqttSettings[4] = mqttSettings.Servers[i][4];
          break;
        }
      }
    }

    if (currentMqttSettings[1] != "")
    {
      Serial.println("Reconnecting to MQTT...");
      // Attempt to connect
      client.setServer(currentMqttSettings[1], atoi(currentMqttSettings[2]));
      if (connect(hostname, currentMqttSettings[3], currentMqttSettings[4]))
      {
        Serial.println("Connected to MQTT.");
        client.subscribe(channelIn);
        Serial.print("Listening: ");
        Serial.println(channelIn);
        digitalWrite(pinsSettings.led, LOW);
        publishStatus(false);
      }
    }
  }
  return client.connected();
}

bool MqttPubSub::connect(const char *id, const char *username, const char *password)
{
  if (username == "")
  {
    return client.connect(id);
  }
  else
  {
    return client.connect(id, username, password);
  }
}

void MqttPubSub::publishStatus(bool waitForInterval) // TODO pass additional status values
{
  if (!waitForInterval || ((millis() - lastMillis) > intervals.statusPublish))
  {
    lastMillis = millis();
    JsonObject root = doc.to<JsonObject>();

    root["uptime"] = (status.currentMillis - status.bootedMillis) / 1000;
    root["loops"] = status.loops;
    root["currentMillis"] = status.currentMillis;
    root["freeMem"] = status.freeMem;
    root["minFreeMem"] = status.minFreeMem;
    root["ipAddress"] = status.ipAddress;
    root["gateway"] = status.gatewayAddress;
    root["location"] = "car";
    root["SSID"] = status.SSID;
    root["RSSI"] = WiFi.RSSI(); // status.rssi;
    root["receivedCount"] = status.receivedCount;

    JsonObject collectors = root.createNestedObject("collectors");
    for (size_t i = 0; i < CollectorCount; i++)
    {
      collectors[pinsSettings.collectors[i].name] = status.collectors[i];
    }

    serializeJson(doc, tempBuffer);
    client.publish(channelStatus, tempBuffer);

    status.missedSend = 0;
  }
  else
  {
    status.missedSend++;
  }
}

void MqttPubSub::handle()
{
  if (status.SSID == "")
    lastReconnectAttempt = -10000; // reconnects as soon as connected to WiFi

  if (status.SSID != "")
  {
    if (!client.connected())
    {
      digitalWrite(pinsSettings.led, HIGH);                                                                                                                                                                                                                                               // set notification led
      if (!(status.currentMillis - lastReconnectAttempt < 200 || (status.currentMillis - lastReconnectAttempt > 500 && status.currentMillis - lastReconnectAttempt < 700) || (status.currentMillis - lastReconnectAttempt > 1000 && status.currentMillis - lastReconnectAttempt < 1200))) // reset notification led
        digitalWrite(pinsSettings.led, LOW);
      if (status.currentMillis - lastReconnectAttempt > 10000) // reconnect to MQTT every 10 seconds
      {
        // Serial.println("Mqtt - Client not connected!");
        lastReconnectAttempt = status.currentMillis;
        // Attempt to reconnect
        if (reconnect())
        {
          lastReconnectAttempt = -10000;
          Serial.print("mqtt connected."); // Expired waiting on inactive connection...");
        }
      }
    }
    else
    {
      // Client connected
      client.loop();
    }
  }
}

void MqttPubSub::sendMessage(String message)
{
  char msg[255];
  message.toCharArray(msg, 255);
  client.publish(channelOut, msg);
}

void MqttPubSub::sendMessage(String message, String channel)
{
  char msg[255];
  message.toCharArray(msg, 255);
  char chnl[255];
  channel.toCharArray(chnl, 255);

  client.publish(chnl, msg);
}

void MqttPubSub::sendMesssageToTopic(const char *topic, String message)
{
  char msg[255];
  message.toCharArray(msg, 255);
  client.publish(topic, msg);
}

void MqttPubSub::sendMesssageToTopic(const char *topic, const char *message)
{
  client.publish(topic, message);
}