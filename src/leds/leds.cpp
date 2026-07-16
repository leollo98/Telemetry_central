#include <leds/leds.h>

CRGB leds[NUM_LEDS];

void preencheLeds(int16_t hue, int16_t sat, int16_t potencia) {
  fill_solid(leds, 16, CHSV(hue, sat, potencia));
}

void fastledinit() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

