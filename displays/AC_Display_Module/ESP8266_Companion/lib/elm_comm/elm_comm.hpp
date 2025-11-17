#pragma once
#include <Arduino.h>
#include <ELMo.h>

#include "../../include/pins.h"

#include "logger.h"

class elm_comm {
  public:
    void init();
    void tick();

    bool isInitialized() { return hasInitializedElm; }
    void initializeElm();

  private:
    ELMo* elm;

    bool hasInitializedElm = false;

    unsigned long lastLoop = 0;
};
