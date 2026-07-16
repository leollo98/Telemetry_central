#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

// tft
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library

/// @brief macros
#define lin(a) (a * 24 + 4)

#define TFT_RST 36 // we use the seesaw for resetting to save a pin
#define SPI_SDA 48
#define SPI_CLK 47
#define TFT_CS 38
#define TFT_DC 35

void display_Wire_Error();
void display_bmp_Error();
void display_aht_Error();
void display_BH1750_Error(char num);
void display_WiFi_Error();
void display_Server_Error();
void display_Internet_Error();
void display_Router_Error();
void display_Swtich2_Error();
void display_Swtich3_Error();
void display_Swtich4_Error();
void fill_display();
void updateTft();
void updateTft(uint32_t progress, uint32_t total);
void updateTft(ota_error_t error);
void display_init();
void lightDisplay(uint16_t light);
void display(float temp, float pres, float lux, float humid, float co2);
void showErrorTft(uint8_t position, bool hide);