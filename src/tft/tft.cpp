#include <tft/tft.h>

Adafruit_ST7735 tft =
    Adafruit_ST7735(TFT_CS, TFT_DC, SPI_SDA, SPI_CLK, TFT_RST);
float medido_antigo[] = {0, 0, 0, 0, 0, 0};
uint8_t oldlight = 160;

uint16_t RGB565(uint8_t red, uint8_t green, uint8_t blue) {
  uint16_t red5 = red >> 3;
  uint16_t green6 = green >> 2;
  uint16_t blue5 = blue >> 3;
  return (red5 << 11) | (green6 << 5) | blue5;
}

void display_Wire_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Wire");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_bmp_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("bmp");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_aht_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("aht");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_BH1750_Error(char num) {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("BH1750 ");
  tft.print(num);
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_WiFi_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("WiFi");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_Server_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Server");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_Internet_Error() {

  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Internet");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_Router_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Router");
  tft.setCursor(4, lin(2));
  tft.print("Error");
}

void display_Swtich2_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Switch");
  tft.setCursor(4, lin(2));
  tft.print("Sala");
}

void display_Swtich3_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Switch");
  tft.setCursor(4, lin(2));
  tft.print("Server");
}

void display_Swtich4_Error() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, lin(1));
  tft.print("Switch");
  tft.setCursor(4, lin(2));
  tft.print("Leo");
}

void fill_display() {
  tft.fillScreen(0);
  tft.setTextSize(2);

  uint8_t linha = 0;
  tft.setCursor(4, lin(linha));
  tft.print("Temp: ");
  tft.setCursor(84, lin(linha));
  tft.print("    ");
  tft.setTextSize(1);
  tft.print("o");
  tft.setTextSize(2);
  tft.print("C");

  tft.drawLine(0, 24, 160, 24, RGB565(255, 255, 255));

  linha++;
  tft.setCursor(4, lin(linha));
  tft.print("Pres: ");
  tft.setCursor(84, lin(linha));
  tft.print("   hPa");

  tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, RGB565(255, 255, 255));

  linha++;
  tft.setCursor(4, lin(linha));
  tft.print("Luz :");
  tft.setTextSize(1);
  tft.setCursor(40, lin(linha) + 10);
  tft.print("int");
  tft.setTextSize(2);
  tft.setCursor(84, lin(linha));
  tft.print("   Nit");

  tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, RGB565(255, 255, 255));

  linha++;
  tft.setCursor(4, lin(linha));
  tft.print("umid:");
  tft.setCursor(84, lin(linha));
  tft.print("   %");

  tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, RGB565(255, 255, 255));

  linha++;
  tft.setCursor(4, lin(linha));
  tft.print("CO  :");
  tft.setTextSize(1);
  tft.setCursor(30, lin(linha) + 6);
  tft.print("2");
  tft.setTextSize(2);
  tft.setCursor(84, lin(linha));
  tft.print("   ppm");
}

void updateTft() {
  tft.fillScreen(0);
  tft.setTextSize(3);
  tft.setCursor(4, 56);
  tft.print("Updating");
}
void updateTft(uint32_t progress, uint32_t total) {
  tft.drawRect(0, 120, (progress / (total / 160)), 20, ST7735_WHITE);
}

void updateTft(ota_error_t error) {
  tft.fillScreen(0);
  tft.setTextSize(2);
  tft.setCursor(4, 56);
  if (error == OTA_AUTH_ERROR) {
    Serial.println("Auth Failed");
    tft.print("Auth Failed");
  } else if (error == OTA_BEGIN_ERROR) {
    Serial.println("Begin Failed");
    tft.print("Begin Failed");
  } else if (error == OTA_CONNECT_ERROR) {
    Serial.println("Connect      Failed");
    tft.print("Connect Failed");
  } else if (error == OTA_RECEIVE_ERROR) {
    Serial.println("Receive      Failed");
    tft.print("Receive Failed");
  } else if (error == OTA_END_ERROR) {
    Serial.println("End Failed");
    tft.print("End Failed");
  }
}

void display_init() {
  tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip
  tft.setRotation(1);
  tft.fillScreen(0);
  tft.setTextSize(1);
}

/// @brief configura os dados para a formatação correta e posiciona eles na tela
void dados(uint8_t linha, uint16_t dado, uint16_t cor) {
  tft.setTextColor(cor);
  if (dado < 10) {

    tft.setCursor(104, linha);
    tft.print(dado);
  } else if (dado < 100) {
    tft.setCursor(92, linha);
    tft.print(dado);
  } else if (dado < 1000) {
    tft.setCursor(80, linha);
    tft.print(dado);
  } else if (dado < 10000) {
    tft.setCursor(68, linha);
    tft.print(dado);
  } else if (dado < 0x8000) {
    tft.setCursor(80, linha);
    tft.print(dado / 1000);
    tft.setCursor(104, linha);
    tft.print("K");
  } else {
    tft.setCursor(68, linha);
    tft.print("ERRO");
  }
  tft.setTextColor(ST7735_WHITE);
}

/// @brief configura os dados para a formatação correta e posiciona eles na tela
void dados(uint8_t linha, float dado, uint16_t cor) {
  tft.setTextColor(cor);
  if (dado < 10) {
    tft.setCursor(80, linha);
    tft.print(dado);
  } else if (dado < 100) {
    tft.setCursor(68, linha);
    tft.print(dado);
  } else {
    tft.setCursor(68, linha);
    tft.print("ERRO");
  }
  tft.setTextColor(ST7735_WHITE);
}

/// @brief organiza os dados para colocação na tela
void display(float temp, float pres, float lux, float humid, float co2) {
  uint8_t linha = 0;
  if ((uint16_t)temp != (uint16_t)medido_antigo[0]) {
    tft.fillRect(68, lin(linha), 12 * 5, 16, 0);
    temp < 24   ? dados(lin(linha), temp, ST7735_CYAN)
    : temp < 26 ? dados(lin(linha), temp, ST7735_WHITE)
                : dados(lin(linha), temp, ST7735_ORANGE);
  }
  linha++;
  if ((uint16_t)pres != (uint16_t)medido_antigo[1]) {
    tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
    dados(lin(linha), (uint16_t)pres, ST7735_WHITE);
  }
  linha++;
  if ((uint16_t)lux != (uint16_t)medido_antigo[2]) {
    tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
    dados(lin(linha), (uint16_t)lux, ST7735_WHITE);
  }
  linha++;
  if ((uint16_t)humid != (uint16_t)medido_antigo[3]) {
    tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
    humid < 40   ? dados(lin(linha), (uint16_t)humid, ST7735_ORANGE)
    : humid < 75 ? dados(lin(linha), (uint16_t)humid, ST7735_WHITE)
                 : dados(lin(linha), (uint16_t)humid, ST7735_CYAN);
  }
  linha++;
  if ((uint16_t)co2 != (uint16_t)medido_antigo[4]) {
    tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
    co2 < 1000 ? dados(lin(linha), (uint16_t)co2, ST7735_WHITE)
               : dados(lin(linha), (uint16_t)co2, ST7735_ORANGE);
  }
  medido_antigo[0] = temp;
  medido_antigo[1] = pres;
  medido_antigo[2] = lux;
  medido_antigo[3] = humid;
  medido_antigo[4] = co2;
}

void lightDisplay(uint16_t light) {
  if (light != oldlight) {
    tft.drawFastHLine(light, 127, 160, ST7735_BLACK);
    tft.drawFastHLine(0, 127, light, ST7735_WHITE);
  }
  oldlight = light;
}

void showErrorTft(uint8_t position, bool hide) {
  for (uint8_t i = 124; i < 127; i++) {
    tft.drawFastHLine(position * 20, i, 20, RGB565(255*hide, 0, 0));
  }
}