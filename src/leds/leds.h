#pragma once

#include <Arduino.h>
#define MAX_ITER 50

#include <FastLED.h>

#define LED_PIN 13
#define NUM_LEDS 16
#define CHIPSET WS2812B
#define COLOR_ORDER GRB

void preencheLeds(int16_t hue, int16_t sat, int16_t potencia);
void fastledinit();