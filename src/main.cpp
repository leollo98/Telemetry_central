#include <Wire.h>
#include <BH1750.h>
#include "MHZ19.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <FastLED.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "time.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <InfluxDbClient.h>
#include <credenciais.h>

#define DEVICE "ESP32"
AsyncWebServer server(80);


InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

Point sensor("Quarto_Leo");

#define LED_PIN 19

#define CANDLE 0xFF9329 // 0xFF9900

// Information about the LED strip itself
#define NUM_LEDS 16
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

uint64_t tempo[] = {0, 0, 0, 0};

uint8_t light = 0;

hw_timer_t *My_timer = NULL;

// struct
// {
// 	uint8_t hora = 9;
// 	uint8_t minutos = 35;
// 	uint8_t duracao = 35;
// } horarioLuz;

/// @brief alarmes -> {hora de inicio, minuto de inicio, duração das luzes, luzes no maximo}
uint8_t horarioLuz[][4] = {
	{9, 10, 25, 5},
};

uint8_t alarme = 0;

#define lin(a) (a * 24 + 4)

#define TFT_RST 4 // we use the seesaw for resetting to save a pin
#define SPI_SDA 23
#define SPI_CLK 25
#define TFT_CS 15
#define TFT_DC 18
// #define SCL 27
// #define SDA 26
#define RX 33
#define TX 32
#define vcc 14
#define back 26

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, SPI_SDA, SPI_CLK, TFT_RST);

TwoWire I2C = TwoWire(0);

BH1750 lightMeter(0x23);  // addr nc
BH1750 lightMeter2(0x5C); // addr vcc
MHZ19 myMHZ19;
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

const char *ntpServer = "br.pool.ntp.org";

uint16_t valor[] = {0, 0, 0, 0, 0};
float medido[] = {0, 0, 0, 0, 0, 0};
float tempetura[] = {0, 0};

bool FLED = true;
bool FLED_override = false;
bool started = false;
uint16_t hue = 0;
uint8_t sat = 0;
uint8_t intenc = 0;
uint64_t vezes = 0;

uint8_t dia = 0;

/// @brief cuida de executar as luzes dos alarmes cadastrados
void onTimer()
{
	if (!started)
	{
		return;
	}
	if (vezes <= horarioLuz[alarme][2] * 60 * 2)
	{
		uint16_t potencia = min((int)(255.0 * vezes / (horarioLuz[alarme][2] * 60.0 * 2.0)), 255);
		fill_solid(leds, 16, CHSV(25, 255, potencia));
		vezes++;
		FastLED.show();
	}
	else
	{
		fill_solid(leds, 16, CHSV(25, 255, 0));
		vezes = 0;
		started = false;
		FastLED.show();
	}
}

/// @brief inicia o display com todas as informações que não serão alteradas
void display_init()
{
	tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip
	Serial.println("TFT initialized");
	tft.setRotation(1);

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

	tft.drawLine(0, 24, 160, 24, 0xffff);

	linha++;
	tft.setCursor(4, lin(linha));
	tft.print("Pres: ");
	tft.setCursor(84, lin(linha));
	tft.print("   hPa");

	tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, 0xffff);

	linha++;
	tft.setCursor(4, lin(linha));
	tft.print("Luz :");
	tft.setTextSize(1);
	tft.setCursor(40, lin(linha) + 10);
	tft.print("int");
	tft.setTextSize(2);
	tft.setCursor(84, lin(linha));
	tft.print("   Nit");

	tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, 0xffff);

	linha++;
	tft.setCursor(4, lin(linha));
	tft.print("umi :");
	tft.setCursor(84, lin(linha));
	tft.print("   %");

	tft.drawLine(0, 24 + 24 * linha, 160, 24 + 24 * linha, 0xffff);

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

/// @brief pega o horario via NTP
tm localTime()
{
	// https://cplusplus.com/reference/ctime/tm/
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return timeinfo;
	}
	return timeinfo;
}

/// @brief definição dos pinos como saida
void pinDef()
{
	pinMode(vcc, OUTPUT);
	digitalWrite(vcc, 1);
	pinMode(back, OUTPUT);
	ledcSetup(0, 5e3, 8);
	ledcAttachPin(back, 0);
	ledcWrite(0, 0);
}

/// @brief inicialização da TODOS os sensores
void sensorsInit()
{
	Serial1.begin(9600, SERIAL_8N1, RX, TX);
	myMHZ19.begin(Serial1);
	myMHZ19.autoCalibration();
	bmp.begin();
	bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,	/* Operating Mode. */
					Adafruit_BMP280::SAMPLING_X2,	/* Temp. oversampling */
					Adafruit_BMP280::SAMPLING_X16,	/* Pressure oversampling */
					Adafruit_BMP280::FILTER_X16,	/* Filtering. */
					Adafruit_BMP280::STANDBY_MS_1); /* Standby time. */
	aht.begin();
	// Initialize the I2C bus (BH1750 library doesn't do this automatically)
	while (!I2C.begin())
	// while ( !I2C.begin(SDA,SCL,400e3) )
	{
		Serial.println("Wire problem");
		delay(100);
	}
	if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23))
	{
		Serial.println(F("BH1750 Advanced begin"));
	}
	else
	{
		Serial.println(F("Error initialising BH1750"));
	}
	if (lightMeter2.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C))
	{
		Serial.println(F("BH1750 2 Advanced begin"));
	}
	else
	{
		Serial.println(F("Error initialising BH1750 2"));
	}
	configTime(-10800, 0, ntpServer);
}

/// @brief HTML da pagina inicial `/`
String SendbaseHTML()
{
	String ptr = "<!DOCTYPE html> <html>\n";
	ptr += "<head><meta name=\"viewport\" http-equiv=\"refresh\" content=\"5\" charset= \"utf-8\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
	ptr += "<title>Sensors Control</title>\n";
	ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
	ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
	ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
	ptr += ".button-on {background-color: #3498db;}\n";
	ptr += ".button-on:active {background-color: #2980b9;}\n";
	ptr += ".button-off {background-color: #34495e;}\n";
	ptr += ".button-off:active {background-color: #2c3e50;}\n";
	ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
	ptr += "</style>\n";
	ptr += "</head>\n";
	ptr += "<body>\n";
	ptr += "<h1>ESP32 Web Server</h1>\n";
	ptr += "<h3>Sensores:</h3>\n";
	ptr += "<h4>Temperatura quarto: </h4><h3>";
	ptr += medido[0];
	ptr += "ºC</h3>\n";
	ptr += "<h4>Temperatura caixa: </h4><h3>";
	ptr += tempetura[0];
	ptr += "ºC</h3>\n";
	ptr += "<h4>Temperatura ESP32: </h4><h3>";
	ptr += temperatureRead();
	ptr += "ºC</h3>\n";
	ptr += "<h4>Pressão: </h4><h3>";
	ptr += medido[1];
	ptr += " Pa</h3>\n";
	ptr += "<h4>Luz1: </h4><h3>";
	ptr += medido[2];
	ptr += " Lux</h3>\n";
	ptr += "<h4>luz2: </h4><h3>";
	ptr += medido[3];
	ptr += " Lux</h3>\n";
	ptr += "<h4>CO2: </h4><h3>";
	ptr += medido[4];
	ptr += " ppm</h3>\n";
	ptr += "<h4>humidade: </h4><h3>";
	ptr += medido[5];
	ptr += "%</h3>\n";
	ptr += "</body>\n";
	ptr += "</html>\n";
	return ptr;
}

/// @brief HTML da pagina de controle dos leds `/led`
String SendLEDHTML()
{
	String ptr = "<!DOCTYPE html> <html>\n";
	ptr += "<head><meta name=\"viewport\" charset= \"utf-8\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
	ptr += "<title>Sensors Control</title>\n";
	ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
	ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
	ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 16px 32px;text-decoration: none;font-size: 100px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
	ptr += ".button-on {background-color: #3498db;}\n";
	ptr += ".button-on:active {background-color: #2980b9;}\n";
	ptr += ".button-off {background-color: #34495e;}\n";
	ptr += ".button-off:active {background-color: #2c3e50;}\n";
	ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
	ptr += "</style>\n";
	ptr += "</head>\n";
	ptr += "<body>\n";
	ptr += "<h1>ESP32 Web Server</h1>\n";
	ptr += "<h3>LED:</h3>\n";
	if (FLED)
	{
		ptr += "<p><a href=\"/led?Foff=1\"><button class=\"button-on\">ON</button></a></p>\n";
	}
	else
	{
		ptr += "<p><a href=\"/led?Fon=1\"><button class=\"button-off\">OFF</button></a></p>\n";
	}
	if (FLED_override)
	{
		ptr += "<p><a href=\"/led?Ooff=1\"><button class=\"button-on\">OVERRIDE ON</button></a></p>\n";
	}
	else
	{
		ptr += "<p><a href=\"/led?Oon=1\"><button class=\"button-off\">OVERRIDE OFF</button></a></p>\n";
	}
	ptr += "<form action=\"/led\">\n";
	ptr += "Hue (0-360): <input type=\"text\" name=\"hue\" value=\"";
	ptr += hue;
	ptr += "\">\n";
	ptr += "Saturação (0-100): <input type=\"text\" name=\"sat\" value=\"";
	ptr += sat;
	ptr += "\">\n";
	ptr += "Intencidade (0-100): <input type=\"text\" name=\"int\" value=\"";
	ptr += intenc;
	ptr += "\">\n";
	ptr += "<input type=\"submit\" value=\"Submit\">\n";
	ptr += "</form>\n";
	ptr += "<p>date: ";
	struct tm data = localTime();
	ptr += data.tm_wday;
	ptr += " - ";
	ptr += data.tm_hour;
	ptr += ":";
	ptr += data.tm_min;
	ptr += ":";
	ptr += data.tm_sec;
	ptr += "\n <p> Potencia atual: ";
	ptr += (int)(100 * vezes / (horarioLuz[alarme][2] * 60.0 * 2.0));
	ptr += "%</p>\n";
	ptr += "<p> Vezes atual: ";
	ptr += (int)vezes;
	ptr += "</p>\n";
	ptr += "<p> duração atual: ";
	ptr += horarioLuz[alarme][2];
	ptr += "</p>\n";
	ptr += "</body>\n";
	ptr += "</html>\n";
	return ptr;
}

void handle_OnConnect(AsyncWebServerRequest *request)
{
	request->send(200, "text/html", SendbaseHTML());
}

void handle_NotFound(AsyncWebServerRequest *request)
{
	request->send(404, "text/plain", "Not found");
}

void handle_led_v2(AsyncWebServerRequest *request)
{
	int paramsNr = request->params();

	for (int i = 0; i < paramsNr; i++)
	{
		AsyncWebParameter *p = request->getParam(i);
		if (p->name() == "Fon")
		{
			FLED = true;
		}
		if (p->name() == "Foff")
		{
			FLED = false;
		}
		if (p->name() == "hue")
		{
			hue = (p->value()).toInt();
		}
		if (p->name() == "sat")
		{
			sat = (p->value()).toInt();
		}
		if (p->name() == "int")
		{
			intenc = (p->value()).toInt();
		}
		if (p->name() == "Oon")
		{
			FLED_override = true;
		}
		if (p->name() == "Ooff")
		{
			FLED_override = false;
		}
	}
	if (FLED_override)
	{

		fill_solid(leds, 16, CHSV((float)hue * 0.708, sat * 2.55, intenc * 2.55));
	}
	else
	{
		fill_solid(leds, 16, CHSV(25, 255, 0));
	}
	FastLED.show();
	request->send(200, "text/html", SendLEDHTML());
}

/// @brief inicializa o wifi e conexão com o banco de dados
void wifiInit()
{
	WiFi.mode(WIFI_STA);
	delay(10);
	// wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	delay(10);
	// while (wifiMulti.run() != WL_CONNECTED)
	while (WiFi.localIP().toString() == "0.0.0.0")
	{
		Serial.print(".");
		delay(500);
	}
	Serial.println();
	if (!client.validateConnection())
	{
		tft.fillScreen(0);
		tft.setTextSize(3);
		tft.setCursor(4, lin(1));
		tft.print("Server");
		tft.setCursor(4, lin(2));
		tft.print("Error");
		Serial.println("rebooting");
		delay(5000);
		ESP.restart();
	}
	server.on("/", [](AsyncWebServerRequest *request)
			  { handle_OnConnect(request); });
	server.on("/led", [](AsyncWebServerRequest *request)
			  { handle_led_v2(request); });
	// server.on("/led/on",[](AsyncWebServerRequest *request){handle_led_on(request);});
	// server.on("/led/off",[](AsyncWebServerRequest *request){handle_led_off(request);});
	// server.on("/override",[](AsyncWebServerRequest *request){handle_override(request);});
	server.onNotFound([](AsyncWebServerRequest *request)
					  { handle_NotFound(request); });
	server.begin();
}

/// @brief inicializa o OTA do Arduino
void ArduinoOTAInit()
{
	ArduinoOTA.onStart([]()
					   {
      tft.fillScreen(0);
	  tft.setTextSize(3);
	  tft.setCursor(4, 56);
	  tft.print("Updating");
	  String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
		.onEnd([]()
			   { Serial.println("\nEnd"); })
		.onProgress([](unsigned int progress, unsigned int total)
					{ Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
		.onError([](ota_error_t error)
				 {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

	ArduinoOTA.begin();
}

void setup()
{
	Serial.begin(115200);
	pinDef();
	wifiInit();
	sensorsInit();
	display_init();
	ArduinoOTAInit();
	ArduinoOTA.handle();

	FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
	FastLED.setBrightness(255);

	tempo[0] = {millis()};
	tempo[1] = {millis()};
	tempo[2] = {millis()};
	tempo[3] = {millis()};
	Serial.println("init OK");
}

/// @brief configura os dados para a formatação correta e posiciona eles na tela
void dados(uint8_t linha, uint16_t dado)
{
	if (dado < 10)
	{
		tft.setCursor(104, linha);
		tft.print(dado);
	}
	else if (dado < 100)
	{
		tft.setCursor(92, linha);
		tft.print(dado);
	}
	else if (dado < 1000)
	{
		tft.setCursor(80, linha);
		tft.print(dado);
	}
	else if (dado < 10000)
	{
		tft.setCursor(68, linha);
		tft.print(dado);
	}
	else if (dado < 0x8000)
	{
		tft.setCursor(80, linha);
		tft.print(dado / 1000);
		tft.setCursor(104, linha);
		tft.print("K");
	}
	else
	{
		tft.setCursor(68, linha);
		tft.print("ERRO");
	}
}

/// @brief configura os dados para a formatação correta e posiciona eles na tela
void dados(uint8_t linha, float dado)
{
	if (dado < 10)
	{
		tft.setCursor(80, linha);
		tft.print(dado);
	}
	else if (dado < 100)
	{
		tft.setCursor(68, linha);
		tft.print(dado);
	}
	else
	{
		tft.setCursor(68, linha);
		tft.print("ERRO");
	}
}

/// @brief organiza os dados para colocação na tela
void display(float temp, uint16_t pres, uint16_t lux, uint16_t lux2, uint16_t co2)
{
	uint8_t linha = 0;
	tft.fillRect(68, lin(linha), 12 * 5, 16, 0);
	dados(lin(linha), temp);
	linha++;
	tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
	dados(lin(linha), pres);
	linha++;
	tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
	dados(lin(linha), lux);
	linha++;
	tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
	dados(lin(linha), lux2);
	linha++;
	tft.fillRect(68, lin(linha), 12 * 4, 16, 0);
	dados(lin(linha), co2);
}

/// @brief controle do acionamento do alarme
void alarmeControl(struct tm timeinfo)
{
	if (FLED_override)
		return;
	if (!started)
	{
		if (timeinfo.tm_wday == 0)
			return;
		if (timeinfo.tm_wday == 6)
			return;
		bool ok = false;
		for (uint8_t i = 0; i < sizeof horarioLuz / sizeof horarioLuz[0]; i++)
		{
			if (timeinfo.tm_hour == horarioLuz[i][0])
				if (timeinfo.tm_min == horarioLuz[i][1]){
					ok = true;
					alarme=i;
					break;
				}
		}
		if (!ok)
		{
			return;
		}

		started = true;
	}
}

// fill_solid(leds, 16, CHSV(25, 255, 0)); // cor, saturação, intencidade
uint8_t oldlight = light;
void loop()
{
	long myMillis = millis();
	sensors_event_t humidity, temp, temp2, pres;
	ArduinoOTA.handle();

	if (tempo[0] + 15 <= myMillis) // controle luz da tela
	{

		if (medido[2] < 2)
		{
			if (medido[3] < 2)
			{
				if (light != 255)
				{
					light++;
				}
			}
		}
		if (medido[2] > 2)
		{
			if (medido[3] > 2)
			{
				if (light != 5)
				{
					light--;
				}
			}
		}
		ledcWrite(0, light);
		if (light != oldlight)
		{
			tft.drawLine(0, 127, 160, 127, 0xffff);
			tft.drawLine(240 - light, 127, 160, 127, 0x0000);
		}
		oldlight = light;
		tempo[0] = tempo[0] + 25;
	}
	if (tempo[1] + 500 <= myMillis)
	{

		if (lightMeter.measurementReady())
		{
			medido[3] = lightMeter.readLightLevel();
		}
		if (lightMeter2.measurementReady())
		{
			medido[2] = lightMeter2.readLightLevel();
		}
		onTimer();
		tempo[1] = tempo[1] + 500;
	}
	if (tempo[3] + 2500 <= myMillis)
	{

		aht.getEvent(&humidity, &temp);
		tempetura[0] = bmp.readTemperature();
		tempetura[1] = temp.temperature;
		// medido[0] = floorf((((tempetura[0] + tempetura[1]) / 2.0) + 0.5) * 100.0) / 100.0;
		medido[0] = tempetura[1];
		medido[1] = bmp.readPressure();
		if (lightMeter.measurementReady())
		{
			medido[3] = lightMeter.readLightLevel();
		}
		if (lightMeter2.measurementReady())
		{
			medido[2] = lightMeter2.readLightLevel();
		}
		medido[4] = myMHZ19.getCO2();
		medido[5] = humidity.relative_humidity;
		display(medido[0], medido[1] / 1e2, medido[2], medido[5], medido[4]);
		tempo[3] = tempo[3] + 2500;
	}
	if (tempo[2] + 4000 <= myMillis)
	{
		sensor.clearFields();
		sensor.addField("temperatura", medido[0]);
		sensor.addField("temperatura_caixa", tempetura[0]);
		sensor.addField("temperatura_ESP32", temperatureRead());
		sensor.addField("Pressão", medido[1]);
		sensor.addField("Luz1", medido[2]);
		sensor.addField("luz2", medido[3]);
		sensor.addField("CO2", medido[4] == 10000 ? 0 : medido[4]);
		sensor.addField("humidade", medido[5]);
		if (WiFi.localIP().toString() == "0.0.0.0")
		{
			Serial.println("Wifi connection lost");
			WiFi.disconnect(true, true);
			WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
			delay(10);
			while (WiFi.localIP().toString() == "0.0.0.0")
			{
				Serial.print(".");
				delay(50);
			}
		}
		while (!client.writePoint(sensor)) // Write point
		{
			delay(10);
			Serial.println("error Write");
		}
		alarmeControl(localTime());
		FastLED.show();
		Serial.println("5s OK");
		tempo[2] = tempo[2] + 4000;
	}
}