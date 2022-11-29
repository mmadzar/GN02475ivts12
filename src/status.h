#ifndef STATUS_H_
#define STATUS_H_

#include "appconfig.h"
#include "shared/status_base.h"

struct Status : public StatusBase
{
  String ivtsCommand = "";
  int collectors[CollectorCount];

    JsonObject GenerateJson()
  {

    JsonObject root = this->PrepareRoot();

    JsonObject jcollectors = root.createNestedObject("collectors");
    for (size_t i = 0; i < CollectorCount; i++)
      jcollectors[pinsSettings.collectors[i].name] = collectors[i];

    return root;
  }
};

extern Status status;

#endif /* STATUS_H_ */
