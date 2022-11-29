#include <Arduino.h>
#include "appconfig.h"
#include "status.h"
#include "WiFiOTA.h"
#include "MqttPubSub.h"
#include <ArduinoOTA.h>
#include <string.h>
#include "Bytes2WiFi.h"
#include "CanBus.h"
//#include "LedCtrl.h"
#include "Sensors.h"
#include "Switches.h"

Status status;
PinsSettings pins;
Intervals intervals;
WiFiSettings wifiSettings;
MqttSettings mqttSettings;
BrakesSettings brakesSettings;
WiFiOTA wota;
MqttPubSub mqtt;
Switches pwmCtrl;
Sensors sensors;
Bytes2WiFi bytesWiFi;
//LedCtrl ledCtrl;

CanBus can;
uint32_t lastBytesSent = 0; // millis when last packet is sent

long loops = 0;
long lastLoopReport = 0;

bool firstRun = true;

long lastVacuumReadTime = 0;
int lastVacuumRead = 0;

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Serial started!");
  status.bootedMillis = millis();
  pinMode(pins.led, OUTPUT);
  wota.setupWiFi();
  wota.setupOTA();
  mqtt.setup();
  bytesWiFi.setup();
  can.setup(mqtt);
  //ledCtrl.setup();
  sensors.setup(mqtt);
  pwmCtrl.setup(mqtt);
}

void loop()
{
  status.currentMillis = millis();
  if (status.currentMillis - lastLoopReport > 1000) // number of loops in 1 second - for performance measurement
  {
    lastLoopReport = status.currentMillis;
    Serial.print("Loops in a second: ");
    Serial.println(loops);
    status.loops = loops;
    loops = 0;
    status.freeMem = esp_get_free_heap_size();
    status.minFreeMem = esp_get_minimum_free_heap_size();
  }
  else
  {
    loops++;
  }

  wota.handleWiFi();
  wota.handleOTA();
  if (loops % 10 == 0) // check mqtt every 10th cycle
    mqtt.handle();

  //ledCtrl.handle();
  pwmCtrl.handle();
  sensors.handle();
  // Send can messages
  //can.sendMessageSet();
  can.handle();

  long microsec = micros();
  // If the max time has passed or the buffer is almost filled then send buffered data out
  if ((microsec - lastBytesSent > 20000 /* SER_BUFF_FLUSH_INTERVAL */) || status.canBytesSize > (2048 /* WIFI_BUFF_SIZE */ - 40))
  {
    if (status.canBytesSize > 0)
    {
      bytesWiFi.send();
      lastBytesSent = microsec;
    }
  }
  bytesWiFi.handle();

  if (firstRun)
  {
    mqtt.publishStatus(false);
    firstRun = false;
  }

  mqtt.publishStatus(true);
}
