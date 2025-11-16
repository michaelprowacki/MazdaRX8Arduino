#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <TaskScheduler.h>

#include "espComm.hpp"
#include "logger.h"
#include "secrets.h"

#include "elm_comm.hpp"

void initElmCb();
