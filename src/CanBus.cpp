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

void CanBus::setup(class MqttPubSub &mqtt_client, Bytes2WiFi &wifiport, Bytes2WiFi &portDebug)
{
  mqttClientCan = &mqtt_client;
  b2w = &wifiport;
  b2wdebug = &portDebug;

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
                              mqttClientCan->sendMessage(String(value), String(wifiSettings.hostname) + "/out/collectors/" + name); });
    collectors[i]->setup();
  }
}

void CanBus::handle()
{
  if (status.ivtsCommand.length() > 2)
  {
    Serial.printf("Sending %s...\n", status.ivtsCommand);
    if (status.ivtsCommand.startsWith("initialize"))
      initializeIVTS();
    else if (status.ivtsCommand.startsWith("restart"))
      restartIVTS();
    else if (status.ivtsCommand.startsWith("default"))
      defaultIVTS();
    else if (status.ivtsCommand.startsWith("initcurrent"))
      initCurrentIVTS();
    else if (status.ivtsCommand.startsWith("stop"))
      stopIVTS();
    else if (status.ivtsCommand.startsWith("stop500"))
      stopIVTS500();
    else if (status.ivtsCommand.startsWith("stop511"))
      stopIVTS511();
    else if (status.ivtsCommand.startsWith("stop600"))
      stopIVTS600();
    else if (status.ivtsCommand.startsWith("stop611"))
      stopIVTS611();
    else if (status.ivtsCommand.startsWith("stop700"))
      stopIVTS700();
    else if (status.ivtsCommand.startsWith("stop711"))
      stopIVTS711();
    else if (status.ivtsCommand.startsWith("start"))
      startIVTS();
    // reset executed command
    status.ivtsCommand = "";
  }

  // execute binary can message
  if (b2w->wifiCmdPos > 5)
  {
    CAN_FRAME(newFrame);
    newFrame.id = 0x411;
    newFrame.extended = 0;
    newFrame.rtr = 1;
    newFrame.length = b2w->wifiCommand[1];
    for (size_t i = 0; i < b2w->wifiCmdPos - 2; i++)
      newFrame.data.bytes[i] = b2w->wifiCommand[i + 1];
    CAN0.sendFrame(newFrame);
    b2wdebug->addBuffer("Message sent!", 13);
    b2wdebug->send();
  }
  b2w->wifiCmdPos = 0;

  CAN_FRAME frame;
  if (CAN0.read(frame))
  {
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
    case 0x680:
      handle680(frame); // current counter
      break;
    case 0x525:
      handle525(frame);
    default:
      break;
    }

    // store message to buffer
    b2w->addBuffer(0xf1);
    b2w->addBuffer(0x00); // 0 = canbus frame sending
    uint32_t now = micros();
    b2w->addBuffer(now & 0xFF);
    b2w->addBuffer(now >> 8);
    b2w->addBuffer(now >> 16);
    b2w->addBuffer(now >> 24);
    b2w->addBuffer(frame.id & 0xFF);
    b2w->addBuffer(frame.id >> 8);
    b2w->addBuffer(frame.id >> 16);
    b2w->addBuffer(frame.id >> 24);
    b2w->addBuffer(frame.length + (uint8_t)(((int)0) << 4)); // 2 ibus address
    for (int c = 0; c < frame.length; c++)
      b2w->addBuffer(frame.data.uint8[c]);
    b2w->addBuffer(0x0a); // new line for SavvyCan and serial monitor
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

int CanBus::handle680(CAN_FRAME frame)
{
  const long v = (long)((frame.data.bytes[2] << 24) | (frame.data.bytes[3] << 16) | (frame.data.bytes[4] << 8) | (frame.data.bytes[5]));
  collectors[settingsCollectors.getCollectorIndex("counter")]->handle((int)v);
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

// IVTS related
void CanBus::initializeIVTS()
{
  b2wdebug->addBuffer("Init...\r\n", 9);
  b2wdebug->handle();
  // TODO firstframe = false;
  stopIVTS();
  delay(700);
  Serial.println("initialization \n");
  for (int i = 0; i < 9; i++)
  {
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::stopIVTS()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}


void CanBus::stopIVTS500()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x500;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::stopIVTS511()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x511;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::stopIVTS600()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x600;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::stopIVTS611()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x611;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::stopIVTS700()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x700;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}


void CanBus::stopIVTS711()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = 0x711;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::startIVTS()
{
  b2wdebug->addBuffer("Start...\r\n", 10);
  b2wdebug->send();
  // SEND START///////
  outframe.id = 0x600;   // Set our transmission address ID
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
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
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
  Serial.println("Restarting IVTS...");
  // Has the effect of zeroing AH and KWH
  outframe.id = 0x600;   // Set our transmission address ID
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
  outframe.id = 0x600;   // Set our transmission address ID
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