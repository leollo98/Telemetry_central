#pragma once

#include <Arduino.h>
#define MAX_ITER 50

#include <leds/leds.h>
#include <tft/tft.h>
#include <servidor/servidor.h>
#include <globals.h>

#include <credenciais.h>



// web server
#include <ArduinoOTA.h>

// date
#include "time.h"

// sensors
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <MHZ19.h>

/// @brief data storage
#include <Preferences.h>



/// @brief tft
#define back 37

/// @brief sensores
#define RX1 11
#define TX1 12