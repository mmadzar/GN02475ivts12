#include "MqttMessageHandler.h"

MqttMessageHandler::MqttMessageHandler()
{
}

void MqttMessageHandler::HandleMessage(const char *command, const char *message, int length)
{
  if (strcmp(command, "ivts") == 0)
  {
    status.ivtsCommand = message;
  }
  else if (strcmp(command, "can") == 0)
  {
    char messagec[length + 1];
    for (size_t cnt = 0; cnt < length + 1; cnt++)
    {
      // convert byte to its ascii representation
      sprintf(&messagec[cnt], "%C", message[cnt]);
    }

    // convert hex representation of message to bytes
    // Ex. 41 41 41 41 41 -> AAAAA
    char tc[2] = {0x00, 0x00};
    for (size_t i = 0; i < (length) / 3; i++)
    {
      tc[0] = char(message[i * 3]);
      tc[1] = char(message[(i * 3) + 1]);
      // TODO status.ibusSend[i] = strtol(tc, 0, 16);
    }
  }
}

void MqttMessageHandler::callback(char *topic, byte *message, unsigned int length)
{

}

void MqttMessageHandler::handle()
{
}