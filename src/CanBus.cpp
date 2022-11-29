#include "CanBus.h"

StaticJsonDocument<512> docJ;
CAN_FRAME frames[3];
long displayValue = 0;
int consumptionCounter = 0; // 0 - 65535(0xFFFF)
MqttPubSub *mqttClientCan;
PinsSettings settingsCollectors;

CanBus::CanBus()
{
  init();
}

void CanBus::init()
{
  // provide power for can bus board
  pinMode(pinsSettings.canpwr, OUTPUT);
  digitalWrite(pinsSettings.canpwr, HIGH);
  CAN0.setCANPins(pinsSettings.can0_rx, pinsSettings.can0_tx);
}

void CanBus::setup(class MqttPubSub &mqtt_client)
{
  mqttClientCan = &mqtt_client;
  CAN0.begin(500000);
  CAN0.watchFor();

  for (size_t i = 0; i < CollectorCount; i++)
  {
    CollectorConfig *sc = &pinsSettings.collectors[i];
    configs[i] = new CollectorConfig(sc->name, sc->sendRate);
    collectors[i] = new Collector(*configs[i]);
    collectors[i]->onChange([](const char *name, int value)
                            { 
                              status.collectors[settingsCollectors.getCollectorIndex(name)]=value;
                              if(strcmp(name, "voltage")==0) 
                                status.ivtsVoltage1=value;
                              else if (strcmp(name, "current")==0)
                                status.ivtsCurrent=value;
                              mqttClientCan->sendMessage(String(value), String(wifiSettings.hostname) + "/out/collectors/" + name); });
    collectors[i]->setup();
  }
}

void CanBus::handle()
{
  if (status.ivtsCommand.length() > 2)
  {
    if (status.ivtsCommand.startsWith("initialize"))
      initializeIVTS();
    else if (status.ivtsCommand.startsWith("restart"))
      restartIVTS();
    else if (status.ivtsCommand.startsWith("default"))
      defaultIVTS();
    else if (status.ivtsCommand.startsWith("initcurrent"))
      initCurrentIVTS();
    // reset executed command
    status.ivtsCommand = "";
  }

  CAN_FRAME frame;
  if (CAN0.read(frame))
  {
    // status.receivedCount++;

    switch (frame.id)
    {
      // ISA IVT Shunt codes
    case 0x521:
    case 0x630:
      handle521(frame); // U1
      break;
    case 0x522:
    case 0x620:
      handle522(frame); // I
      break;
    case 0x525:
      handle525(frame);
    default:
      break;
    }

    // store message to buffer
    status.canBytes[status.canBytesSize++] = 0xF1;
    status.canBytes[status.canBytesSize++] = 0; // 0 = canbus frame sending
    uint32_t now = micros();
    status.canBytes[status.canBytesSize++] = (uint8_t)(now & 0xFF);
    status.canBytes[status.canBytesSize++] = (uint8_t)(now >> 8);
    status.canBytes[status.canBytesSize++] = (uint8_t)(now >> 16);
    status.canBytes[status.canBytesSize++] = (uint8_t)(now >> 24);
    status.canBytes[status.canBytesSize++] = (uint8_t)(frame.id & 0xFF);
    status.canBytes[status.canBytesSize++] = (uint8_t)(frame.id >> 8);
    status.canBytes[status.canBytesSize++] = (uint8_t)(frame.id >> 16);
    status.canBytes[status.canBytesSize++] = (uint8_t)(frame.id >> 24);
    status.canBytes[status.canBytesSize++] = frame.length + (uint8_t)(((int)1) << 4); // 0 can bus address
    for (int c = 0; c < frame.length; c++)
    {
      status.canBytes[status.canBytesSize++] = frame.data.uint8[c];
    }
    status.canBytes[status.canBytesSize++] = (uint8_t)0;
    sprintf((char *)&status.canBytes[status.canBytesSize], "\r\n");
    status.canBytesSize += 2;
  }
}

int CanBus::handle521(CAN_FRAME frame)
{
  const long v = (long)((frame.data.bytes[2] << 24) | (frame.data.bytes[3] << 16) | (frame.data.bytes[4] << 8) | (frame.data.bytes[5]));
  collectors[settingsCollectors.getCollectorIndex("voltage")]->handle((int)v);
  return v;
}

int CanBus::handle522(CAN_FRAME frame)
{
  const long v = (long)((frame.data.bytes[2] << 24) | (frame.data.bytes[3] << 16) | (frame.data.bytes[4] << 8) | (frame.data.bytes[5]));
  collectors[settingsCollectors.getCollectorIndex("current")]->handle((int)v);
  return v;
}

int CanBus::handle525(CAN_FRAME frame)
{
  // TODO
  //  const long v = ((long)((frame.data.bytes[2] << 24) | (frame.data.bytes[3] << 16) | (frame.data.bytes[4] << 8) | (frame.data.bytes[5]))) / 10.0;
  //  if (status.ivtsTemp != v)
  //  {
  //    mqttClientCan->sendMessage(String(v), String(wifiSettings.hostname) + "/out/IVTS12/temp");
  //    status.ivtsTemp = v;
  //  }
  return 0;
}

void CanBus::sendMessageSet()
{
  // // create and send can message in intervals
  // if (status.currentMillis - previousMillis >= intervals.CANsend)
  // {
  //   previousMillis = status.currentMillis;

  //   // update rpm and coolant temp values
  //   displayValue = status.rpm * 6.4;
  //   frames[1].data.uint8[2] = (int)((displayValue & 0X000000FF));      // rpm 0,2
  //   frames[1].data.uint8[3] = (int)((displayValue & 0x0000FF00) >> 8); // rpm 2,4
  //   displayValue = status.coolant_temp * 2;
  //   frames[2].data.uint8[1] = (int)((displayValue & 0X000000FF));

  //   CAN0.sendFrame(frames[0]);
  //   CAN0.sendFrame(frames[1]);
  //   CAN0.sendFrame(frames[2]);
  // }
}

// IVTS related
void CanBus::initializeIVTS()
{
  // TODO firstframe = false;
  stopIVTS();
  delay(700);
  for (int i = 0; i < 9; i++)
  {
    Serial.println("initialization \n");
    outframe.id = 0x411;   // Set our transmission address ID
    outframe.length = 8;   // Data payload 8 bytes
    outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
    outframe.rtr = 1;      // No request
    outframe.data.bytes[0] = (0x20 + i);
    outframe.data.bytes[1] = 0x42;
    outframe.data.bytes[2] = 0x02;
    outframe.data.bytes[3] = (0x60 + (i * 18));
    outframe.data.bytes[4] = 0x00;
    outframe.data.bytes[5] = 0x00;
    outframe.data.bytes[6] = 0x00;
    outframe.data.bytes[7] = 0x00;
    CAN0.sendFrame(outframe);
    delay(500);

    storeIVTS();
    delay(500);
  }
  startIVTS();
  delay(500);
}

void CanBus::stopIVTS()
{
  // SEND STOP///////
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = 0x34;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x01;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::startIVTS()
{
  // SEND START///////
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = 0x34;
  outframe.data.bytes[1] = 0x01;
  outframe.data.bytes[2] = 0x01;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::storeIVTS()
{
  // SEND STORE///////
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = 0x32;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::restartIVTS()
{
  // Has the effect of zeroing AH and KWH
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = 0x3F;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::defaultIVTS()
{
  // Returns module to original defaults
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = 0x3D;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::initCurrentIVTS()
{
  stopIVTS();
  delay(500);
  outframe.id = 0x411;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 1;      // No request
  outframe.data.bytes[0] = (0x21);
  outframe.data.bytes[1] = 0x42;
  outframe.data.bytes[2] = 0x01;
  outframe.data.bytes[3] = (0x61);
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;

  CAN0.sendFrame(outframe);
  storeIVTS();
  delay(500);

  startIVTS();
  delay(500);
}