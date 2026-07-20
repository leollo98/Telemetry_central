#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <globals.h>
#include <servidor/servidor.h>
#include <tft/tft.h>

void ArduinoOTAInit();
void espRestartSafe();