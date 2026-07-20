#include <main.h>

/// @brief variaveis de controle
uint64_t tempo[] = {0, 0, 0, 0, 0};
uint64_t aux[] = {0, 0, 0};
float medido[6] = {0};
float tempetura[2] = {0};
enum error {
  check,
  inicio,
  wire,
  Sbmp,
  Saht,
  SlightMeter,
  SlightMeter2,
  connected,
  ponto
};
bool reboot = false;
bool update = false;

/// @brief sensores
// #define SDA 8
// #define SCL 9
uint16_t light = 160;
uint16_t luz = 0;
uint16_t oldluz = 0;
uint16_t valor[] = {0, 0, 0, 0, 0};
BH1750 lightMeter(0x23);  // addr nc
BH1750 lightMeter2(0x5C); // addr vcc
MHZ19 myMHZ19;
// TwoWire I2C = TwoWire(0);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

void resetOnTime(struct tm timeinfo) {
  if (timeinfo.tm_hour != 4) {
    return;
  }
  if (timeinfo.tm_min != 0) {
    return;
  }
  if (timeinfo.tm_sec < 5) {
    return;
  }
  if (reboot == false) {
    return;
  }
  espRestartSafe();
}



void display_Error(error erro) {
  uint64_t i = 0;
  switch (erro) {
  case inicio:
    while (!WiFi.isConnected() && i < MAX_ITER) {
      if (i == 0) {
        display_WiFi_Error();
      }
      Serial.print(".");
      delay(50);
      i = i + 1;
    }
    Serial.println();
    break;
  case wire:
    while (!Wire.begin() && i < MAX_ITER) {
      if (i == 0) {
        display_Wire_Error();
      }
      Serial.println("Wire problem");
      delay(50);
      i = i + 1;
    }
    break;
  case Sbmp:
    while (!bmp.begin() && i < MAX_ITER) {
      if (i == 0) {
        display_bmp_Error();
      }
      Serial.println("Error initialising bmp");
      delay(100);

      i = i + 1;
    }
    break;
  case Saht:
    while (!aht.begin() && i < MAX_ITER) {
      if (i == 0) {
        display_aht_Error();
      }
      Serial.println("Error initialising aht");
      delay(100);

      i = i + 1;
    }
    break;
  case SlightMeter:
    while (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23) &&
           i < MAX_ITER) {
      if (i == 0) {
        display_BH1750_Error('1');
      }
      Serial.println("Error initialising BH1750");
      delay(100);

      i = i + 1;
    }
    break;
  case SlightMeter2:
    while (!lightMeter2.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C) &&
           i < MAX_ITER) {
      if (i == 0) {
        display_BH1750_Error('2');
      }
      Serial.println("Error initialising BH1750 2");
      delay(100);
      i = i + 1;
    }
    break;
  case connected:
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi connection lost");
      while (WiFi.status() != WL_CONNECTED && i < MAX_ITER) {
        WiFi.disconnect(true, true);
        if (i == 0) {
          display_WiFi_Error();
        }
        delay(10);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print(".");
        delay(200);
        i = i + 1;
      }
    }
    break;
  case check:
    aux[2] += showErrorTft(0, !WiFi.isConnected());
    aux[2] += showErrorTft(2, !bmp.begin());
    aux[2] += showErrorTft(3, !aht.begin());
    aux[2] += showErrorTft(4, !lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23));
    aux[2] += showErrorTft(5, !lightMeter2.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C));
    break;
  default:
    break;
    
  }
  
  if (i > 0) {
    fill_display();
  }
  if (i >= MAX_ITER)
  {
    espRestartSafe();
  }
  if (aux[2] >= MAX_ITER) {
    espRestartSafe();
  }
}

/// @brief inicialização da TODOS os sensores
void sensorsInit() {
  Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
  myMHZ19.begin(Serial1);
  myMHZ19.autoCalibration();

  display_Error(wire);

  display_Error(Sbmp);
  Serial.println("BMP Init");
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,   /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,   /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,  /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,    /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */
  Serial.println("BMP Config");

  display_Error(Saht);

  Serial.println("aht Init");
  display_Error(SlightMeter);

  Serial.println("BH1750 Advanced begin");
  display_Error(SlightMeter2);

  Serial.println("BH1750 2 Advanced begin");
  const char *ntpServer0 = "192.168.1.10";
  const char *ntpServer1 = "pool.ntp.org";
  const char *ntpServer2 = "ntp.br";
  configTime(-10800, 0, ntpServer0, ntpServer1, ntpServer2);
}

void wifiInit() {
  WiFi.mode(WIFI_STA);
  delay(10);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(10);
  int16_t i = 0;
  display_Error(inicio);
}

void pinDef() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(back, OUTPUT);
  gpio_set_drive_capability((gpio_num_t)back, GPIO_DRIVE_CAP_MAX);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(SPI_SDA, OUTPUT);
  pinMode(SPI_CLK, OUTPUT);
  ledcSetup(0, 5e3, 8);
  ledcAttachPin(back, 0);
  ledcWrite(0, 255 - light);
}

void timerinit() {
  for (uint8_t i = 0; i < 4; i++) {
    tempo[i] = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("(┛ಠ_ಠ)┛彡┻━┻");

  pinDef();
  display_init();
  wifiInit();
  ArduinoOTAInit();
  ArduinoOTA.handle();
  serverSetup();

  sensorsInit();
  fastledinit();
  timerinit();
  Serial.println("init OK");
}

void controleBack() {
  if (medido[2] < 2) {
    if (medido[3] < 2) {
      if (light != 0) {
        light--;
      }
    }
  }
  if (medido[2] > 2) {
    if (medido[3] > 2) {
      if (light != 160) {
        light++;
      }
    }
  }
  ledcWrite(0, 255 - light);
  lightDisplay(light);
}

void medidaLuz() {
  if (lightMeter.measurementReady()) {
    medido[3] = lightMeter.readLightLevel();
  }
  if (lightMeter2.measurementReady()) {
    medido[2] = lightMeter2.readLightLevel();
  }
}

void pegaValores() {
  sensors_event_t humidity, temperatura;
  aht.getEvent(&humidity, &temperatura);
  tempetura[0] = bmp.readTemperature();
  tempetura[1] = temperatura.temperature;
  medido[0] = tempetura[0];
  // floorf((((tempetura[0] + tempetura[1]) / 2.0) + 0.5) * 100.0) / 100.0;
  medido[0] = tempetura[1];
  medido[1] = bmp.readPressure();
  if (lightMeter.measurementReady()) {
    medido[3] = lightMeter.readLightLevel();
  }
  if (lightMeter2.measurementReady()) {
    medido[2] = lightMeter2.readLightLevel();
  }
  medido[4] = myMHZ19.getCO2();
  medido[5] = humidity.relative_humidity;
}

void loop() {
  long myMillis = millis();

  ArduinoOTA.handle();
  if (tempo[0] + 15 <= myMillis) // controle luz da tela
  {
    controleBack();
    ArduinoOTA.handle();
    tempo[0] = tempo[0] + 15;
  }
  if (tempo[1] + 100 <= myMillis) {
    ArduinoOTA.handle();
    pegaValores();
    display(medido[0], medido[1] / 1e2, medido[3], medido[5], medido[4]);
    tempo[1] = tempo[1] + 100;
  }
  if (tempo[2] + 500 <= myMillis) {
    ArduinoOTA.handle();
    medidaLuz();
    onTimer();
    resetOnTime(localTime());
    display_Error(check);
    tempo[2] = tempo[2] + 500;
  }
  if (tempo[3] + 4000 <= myMillis) {
    ArduinoOTA.handle();

    tempo[3] = tempo[3] + 4000;
  }
  if (tempo[4] + 600000 <= myMillis) {
    ArduinoOTA.handle();
    reboot = true;
    tempo[4] = tempo[4] + 600000;
  }
  while (update) {
    ArduinoOTA.handle();
  }
}