#include <Arduino.h>

#include <credenciais.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>

#define MAX_ITER 50

// tft
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library
// web server
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <InfluxDbClient.h>

#include <FastLED.h>

#include "time.h"

#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <MHZ19.h>
#include <BH1750.h>

/// @brief data storage
// #define data
#ifdef data
#include <Preferences.h>
Preferences pref;
#endif

/// @brief macros
#define lin(a) (a * 24 + 4)

/// @brief tft
#define back 37
#define TFT_RST 36 // we use the seesaw for resetting to save a pin
#define SPI_SDA 40
#define SPI_CLK 39
#define TFT_CS 38
#define TFT_DC 35
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, SPI_SDA, SPI_CLK, TFT_RST);

/// @brief  web server

AsyncWebServer server(80);
HTTPClient http;

/// @brief influxDB

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point sensor("Quarto_Leo");

/// @brief FastLED

#define LED_PIN 13
#define NUM_LEDS 16
CRGB leds[NUM_LEDS];
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
bool FLED = true;
bool FLED_override = false;
bool started = false;
uint16_t hue = 0;
uint8_t sat = 0;
uint8_t intenc = 0;
uint64_t vezes = 0;

/// @brief alarmes -> {hora de inicio, minuto de inicio, duração do fade, luzes no maximo}
#define quantidadeAlarmes 8
uint8_t saveAlarme = 0;
uint8_t horarioLuz[quantidadeAlarmes][4] = {
	{9, 30, 15, 25},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
};
uint8_t alarme = 0;

/// @brief variaveis de controle
uint64_t tempo[] = {0, 0, 0, 0, 0};
uint64_t aux[] = {0, 0};
enum error
{
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

/// @brief sensores

#define RX1 11
#define TX1 12
// #define SDA 8
// #define SCL 9
uint8_t light = 255;
uint8_t oldlight = 255;
uint16_t valor[] = {0, 0, 0, 0, 0};
float medido[] = {0, 0, 0, 0, 0, 0};
float tempetura[] = {0, 0};
BH1750 lightMeter(0x23);  // addr nc
BH1750 lightMeter2(0x5C); // addr vcc
MHZ19 myMHZ19;
// TwoWire I2C = TwoWire(0);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

void preencheLeds(int16_t hue, int16_t sat, int16_t potencia)
{
	fill_solid(leds, 16, CHSV(hue, sat, potencia));
}

/// @brief cuida de executar as luzes dos alarmes cadastrados
void onTimer()
{
	if (!started)
	{
		return;
	}
	if (horarioLuz[alarme][0] == 0)
	{
		if (horarioLuz[alarme][1] == 0)
		{
			return;
		}
	}

	if (vezes <= (horarioLuz[alarme][2] + horarioLuz[alarme][3]) * 60 * 2)
	{
		uint16_t potencia = min((uint64_t)(255.0 * (double)vezes / (horarioLuz[alarme][2] * 60.0 * 2.0)), (uint64_t)254);
		preencheLeds(25, 255, potencia);
		vezes++;
		FastLED.show();
	}
	else
	{
		preencheLeds(25, 255, 0);
		vezes = 0;
		started = false;
		FastLED.show();
	}
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

void resetOnTime(struct tm timeinfo)
{
	if (timeinfo.tm_hour != 4)
	{
		return;
	}
	if (timeinfo.tm_min != 0)
	{
		return;
	}
	if (timeinfo.tm_sec < 5)
	{
		return;
	}
	esp_restart();
}

void scanI2C()
{
	byte error, address;
	int nDevices;
	Serial.println("Scanning...");
	nDevices = 0;
	for (address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			Serial.print("I2C device found at address 0x");
			if (address < 16)
			{
				Serial.print("0");
			}
			Serial.println(address, HEX);
			nDevices++;
		}
		else if (error == 4)
		{
			Serial.print("Unknow error at address 0x");
			if (address < 16)
			{
				Serial.print("0");
			}
			Serial.println(address, HEX);
		}
	}
	if (nDevices == 0)
	{
		Serial.println("No I2C devices found\n");
	}
	else
	{
		Serial.println("done\n");
	}
}

void display_Wire_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Wire");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_bmp_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("bmp");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_aht_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("aht");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_BH1750_Error(char num)
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("BH1750 ");
	tft.print(num);
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_WiFi_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("WiFi");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_Server_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Server");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_Internet_Error()
{

	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Internet");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_Router_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Router");
	tft.setCursor(4, lin(2));
	tft.print("Error");
}

void display_Swtich2_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Switch");
	tft.setCursor(4, lin(2));
	tft.print("Sala");
}

void display_Swtich3_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Switch");
	tft.setCursor(4, lin(2));
	tft.print("Server");
}

void display_Swtich4_Error()
{
	tft.fillScreen(0);
	tft.setTextSize(3);
	tft.setCursor(4, lin(1));
	tft.print("Switch");
	tft.setCursor(4, lin(2));
	tft.print("Leo");
}

void fill_display()
{
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
	tft.print("umid:");
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

void display_Error(error erro)
{
	uint64_t i = 0;
	switch (erro)
	{
	case inicio:
		while (!WiFi.isConnected() && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_WiFi_Error();
			}
			Serial.print(".");
			delay(500);
			i = i + 1;
		}
		Serial.println();
		if (!client.validateConnection())
		{
			display_Server_Error();
			Serial.println("rebooting");
			delay(5000);
			ESP.restart();
		}
		break;
	case wire:
		while (!Wire.begin() && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_Wire_Error();
			}
			Serial.println("Wire problem");
			delay(100);

			i = i + 1;
		}
		break;
	case Sbmp:
		while (!bmp.begin() && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_bmp_Error();
			}
			Serial.println("Error initialising bmp");
			delay(100);

			i = i + 1;
		}
		break;
	case Saht:
		while (!aht.begin() && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_aht_Error();
			}
			Serial.println("Error initialising aht");
			delay(100);

			i = i + 1;
		}
		break;
	case SlightMeter:
		while (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23) && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_BH1750_Error('1');
			}
			Serial.println("Error initialising BH1750");
			delay(100);

			i = i + 1;
		}
		break;
	case SlightMeter2:
		while (!lightMeter2.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C) && i < MAX_ITER)
		{
			if (i == 0)
			{
				display_BH1750_Error('2');
			}
			Serial.println("Error initialising BH1750 2");
			delay(100);
			i = i + 1;
		}
		break;
	case connected:
		if (WiFi.localIP().toString() == "0.0.0.0")
		{
			Serial.println("Wifi connection lost");
			WiFi.disconnect(true, true);
			WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
			delay(10);
			while (WiFi.localIP().toString() == "0.0.0.0" && i < MAX_ITER)
			{
				if (i == 0)
				{
					display_WiFi_Error();
				}
				Serial.print(".");
				delay(50);
				i = i + 1;
			}
		}
		break;
	case ponto:
		i = 0;
		while (!client.writePoint(sensor) && i < MAX_ITER) // Write point
		{
			if (i == 0)
			{
				display_Server_Error();
			}
			delay(50);
			Serial.print("error Write - ");
			Serial.print(client.getLastErrorMessage());
			Serial.print(" -;- ");
			Serial.println(client.getLastStatusCode());

			i = i + 1;
			tft.drawFastHLine(140, 124, 20, 0xf800);
			tft.drawFastHLine(140, 125, 20, 0xf800);
			tft.drawFastHLine(140, 126, 20, 0xf800);
		}
		if (i == 0)
		{
			tft.drawFastHLine(140, 124, 20, 0x0000);
			tft.drawFastHLine(140, 125, 20, 0x0000);
			tft.drawFastHLine(140, 126, 20, 0x0000);
		}
		break;
	case check:
		if (WiFi.isConnected())
		{
			tft.drawFastHLine(0, 124, 20, 0x0000);
			tft.drawFastHLine(0, 125, 20, 0x0000);
			tft.drawFastHLine(0, 126, 20, 0x0000);
		}
		else
		{
			tft.drawFastHLine(0, 124, 20, 0xf800);
			tft.drawFastHLine(0, 125, 20, 0xf800);
			tft.drawFastHLine(0, 126, 20, 0xf800);
		}
		if (bmp.begin())
		{
			tft.drawFastHLine(40, 124, 20, 0x0000);
			tft.drawFastHLine(40, 125, 20, 0x0000);
			tft.drawFastHLine(40, 126, 20, 0x0000);
		}
		else
		{
			tft.drawFastHLine(40, 124, 20, 0xf800);
			tft.drawFastHLine(40, 125, 20, 0xf800);
			tft.drawFastHLine(40, 126, 20, 0xf800);
		}
		if (aht.begin())
		{
			tft.drawFastHLine(60, 124, 20, 0x0000);
			tft.drawFastHLine(60, 125, 20, 0x0000);
			tft.drawFastHLine(60, 126, 20, 0x0000);
		}
		else
		{
			tft.drawFastHLine(60, 124, 20, 0xfbe0);
			tft.drawFastHLine(60, 125, 20, 0xfbe0);
			tft.drawFastHLine(60, 126, 20, 0xfbe0);
		}
		if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23))
		{
			tft.drawFastHLine(80, 124, 20, 0x0000);
			tft.drawFastHLine(80, 125, 20, 0x0000);
			tft.drawFastHLine(80, 126, 20, 0x0000);
		}
		else
		{
			tft.drawFastHLine(80, 124, 20, 0xf800);
			tft.drawFastHLine(80, 125, 20, 0xf800);
			tft.drawFastHLine(80, 126, 20, 0xf800);
		}
		if (lightMeter2.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x5C))
		{
			tft.drawFastHLine(100, 124, 20, 0x0000);
			tft.drawFastHLine(100, 125, 20, 0x0000);
			tft.drawFastHLine(100, 126, 20, 0x0000);
		}
		else
		{
			tft.drawFastHLine(100, 124, 20, 0xfbe0);
			tft.drawFastHLine(100, 125, 20, 0xfbe0);
			tft.drawFastHLine(100, 126, 20, 0xfbe0);
		}
		if (WiFi.localIP().toString() == "0.0.0.0")
		{
			tft.drawFastHLine(120, 124, 20, 0xf800);
			tft.drawFastHLine(120, 125, 20, 0xf800);
			tft.drawFastHLine(120, 126, 20, 0xf800);
		}
		else
		{

			tft.drawFastHLine(120, 124, 20, 0x0000);
			tft.drawFastHLine(120, 125, 20, 0x0000);
			tft.drawFastHLine(120, 126, 20, 0x0000);
		}
		break;
	default:
		break;
	}
	if (i > 0)
	{
		fill_display();
	}
}

/// @brief inicialização da TODOS os sensores
void sensorsInit()
{
	Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
	myMHZ19.begin(Serial1);
	myMHZ19.autoCalibration();

	display_Error(wire);
	// scanI2C();

	display_Error(Sbmp);
	Serial.println("BMP Init");
	bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,	/* Operating Mode. */
					Adafruit_BMP280::SAMPLING_X2,	/* Temp. oversampling */
					Adafruit_BMP280::SAMPLING_X16,	/* Pressure oversampling */
					Adafruit_BMP280::FILTER_X16,	/* Filtering. */
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

/// @brief inicia o armazenamento para dados persisitentes
void storageInit()
{
#ifdef data
	pref.begin("alarms", false);

	for (uint8_t i = 0; i < quantidadeAlarmes; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			String chave = (i * 4 + j) + "a";
			horarioLuz[i][j] = pref.getInt(chave.c_str(), 0);
		}
	}
#endif
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
	ptr += "";
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

/// @brief HTML da pagina de controle dos alarmes `/alarme`
String SendAlarmeHTML()
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
	ptr += "<h3>Alarmes:</h3>\n";
	ptr += "<form action=\"/alarme\">\n";
	ptr += "hora (0-23): <input type=\"text\" name=\"hora\" value=\"";
	ptr += horarioLuz[saveAlarme - 1][0];
	ptr += "\">\n";
	ptr += "minuto (0-59): <input type=\"text\" name=\"minuto\" value=\"";
	ptr += horarioLuz[saveAlarme - 1][1];
	ptr += "\">\n";
	ptr += "tempo fade in (0-59): <input type=\"text\" name=\"fade\" value=\"";
	ptr += horarioLuz[saveAlarme - 1][2];
	ptr += "\">\n";
	ptr += "tempo maximos (0-59): <input type=\"text\" name=\"max\" value=\"";
	ptr += horarioLuz[saveAlarme - 1][3];
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

String SendEcolhaAlarmeHTML()
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
	ptr += "table {font-family: arial, sans-serif;  border-collapse: collapse;  width: 100%;}td, th {  border: 1px solid #dddddd;  text-align: left;  padding: 8px;}tr:nth-child(even) {background-color: #dddddd;}\n";
	ptr += "</style>\n";
	ptr += "</head>\n";
	ptr += "<body>\n";
	ptr += "<h1>ESP32 Web Server</h1>\n";
	ptr += "<h3>Alarmes:</h3>\n";
#ifdef data
	ptr += "<form action=\"/alarme\">\n";
	ptr += "alarme (1-8): <input type=\"text\" name=\"alarme\" value=\"";
	ptr += 1;
	ptr += "\">\n";
	ptr += "<input type=\"submit\" value=\"Submit\">\n";
	ptr += "</form>\n";
#endif
	ptr += "<table>";
	ptr += "<tr>";
	ptr += "<th>Alarme</th>";
	ptr += "<th>Hora</th>";
	ptr += "<th>Minuto</th>";
	ptr += "<th>Fade In</th>";
	ptr += "<th>Maximo</th>";
	ptr += "</tr>";

	for (uint8_t i = 0; i < quantidadeAlarmes; i++)
	{
		ptr += "<tr>";
		ptr += "<td>";
		ptr += i + 1;
		ptr += "</td>";
		for (uint8_t j = 0; j < 4; j++)
		{
			ptr += "<td>";
			ptr += horarioLuz[i][j];
			ptr += "</td>";
		}
		ptr += "</tr>";
	}
	ptr += "</table>";

	return ptr;
}

String SendERRORHTML()
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
	ptr += "<h3>Parametro fora de limite:</h3>\n";
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

		preencheLeds((float)hue * 255 / 360, sat * 2.55, intenc * 2.55);
	}
	else
	{
		preencheLeds(25, 255, 0);
	}
	FastLED.show();
	request->send(200, "text/html", SendLEDHTML());
}

void handle_alarme(AsyncWebServerRequest *request)
{
	int paramsNr = request->params();
	int ok = 0;
	for (int i = 0; i < paramsNr; i++)
	{
		AsyncWebParameter *p = request->getParam(i);
		if (p->name() == "alarme")
		{
			if ((p->value()).toInt() < quantidadeAlarmes)
			{
				saveAlarme = (p->value()).toInt();
			}
			else
			{
				request->send(200, "text/html", SendAlarmeHTML());
				return;
			}
		}
		if (p->name() == "hora")
		{
			horarioLuz[saveAlarme - 1][0] = (p->value()).toInt();
			ok++;
		}
		if (p->name() == "minuto")
		{
			horarioLuz[saveAlarme - 1][1] = (p->value()).toInt();
			ok++;
		}
		if (p->name() == "fade")
		{
			horarioLuz[saveAlarme - 1][2] = (p->value()).toInt();
			ok++;
		}
		if (p->name() == "max")
		{
			horarioLuz[saveAlarme - 1][3] = (p->value()).toInt();
			ok++;
		}
		if (ok == 4)
		{
#ifdef data
			for (uint8_t i = 0; i < quantidadeAlarmes; i++)
			{
				for (size_t j = 0; j < 4; j++)
				{
					String chave = (i * 4 + j) + "a";
					pref.putInt(chave.c_str(), horarioLuz[i][j]);
				}
			}
			pref.end();
			storageInit();
#endif
			saveAlarme = 0;
		}
	}
	if (saveAlarme != 0)
		request->send(200, "text/html", SendAlarmeHTML());
	else
		request->send(200, "text/html", SendEcolhaAlarmeHTML());
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

void display_init()
{
	Serial.println("TFT start");
	tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip
	Serial.println("TFT initialized");
	tft.setRotation(1);

	tft.fillScreen(0);
	tft.setTextSize(1);
}

void wifiInit()
{
	WiFi.mode(WIFI_STA);
	delay(10);
	// wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	delay(10);
	// while (wifiMulti.run() != WL_CONNECTED)
	int16_t i = 0;
	display_Error(inicio);

	server.on("/", [](AsyncWebServerRequest *request)
			  { handle_OnConnect(request); });
	server.on("/led", [](AsyncWebServerRequest *request)
			  { handle_led_v2(request); });
	server.on("/alarme", [](AsyncWebServerRequest *request)
			  { handle_alarme(request); });
	server.onNotFound([](AsyncWebServerRequest *request)
					  { handle_NotFound(request); });
	server.begin();
}

void pinDef()
{
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
	ledcWrite(0, 255);
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
				if (timeinfo.tm_min == horarioLuz[i][1])
				{
					ok = true;
					alarme = i;
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

void fastledinit()
{
	FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
	FastLED.setBrightness(255);
}

void timerinit()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		tempo[i] = millis();
	}
}

void setup()
{
	Serial.begin(115200);
	Serial.println("(┛ಠ_ಠ)┛彡┻━┻");

	pinDef();
	display_init();
	wifiInit();
	ArduinoOTAInit();
	ArduinoOTA.handle();

	sensorsInit();
	fastledinit();
	timerinit();
	Serial.println("init OK");
}

void controleBack()
{
	if (medido[2] > 2)
	{
		if (medido[3] > 2)
		{
			if (light != 255)
			{
				light++;
			}
		}
	}
	if (medido[2] < 2)
	{
		if (medido[3] < 2)
		{
			if (light != 0)
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
}

void medidaLuz()
{
	if (lightMeter.measurementReady())
	{
		medido[3] = lightMeter.readLightLevel();
	}
	if (lightMeter2.measurementReady())
	{
		medido[2] = lightMeter2.readLightLevel();
	}
}

void enviaValores()
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
	display_Error(connected);
	display_Error(ponto);
	alarmeControl(localTime());
	FastLED.show();
}

void pegaValores()
{
	sensors_event_t humidity, temperatura;
	aht.getEvent(&humidity, &temperatura);
	tempetura[0] = bmp.readTemperature();
	tempetura[1] = temperatura.temperature;
	medido[0] = floorf((((tempetura[0] + tempetura[1]) / 2.0) + 0.5) * 100.0) / 100.0;
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
}

bool makeRequest(String serverName)
{

	http.begin(serverName.c_str());
	if (http.GET() > 0)
	{
		return true;
	}
	return false;
}

void verificaRede()
{
	aux[0]++;
	display_Error(connected);
	switch (aux[0])
	{
	case 1:
		if (!makeRequest("google.com"))
		{
			display_Internet_Error();
			aux[1] = 1;
		}
		break;
	case 2:
		if (!makeRequest("192.168.1.1"))
		{
			display_Router_Error();
			aux[1] = 2;
		}
		break;
	case 3:
		if (!makeRequest("192.168.1.2"))
		{
			display_Swtich2_Error();
			aux[1] = 3;
		}
		break;
	case 4:
		if (!makeRequest("192.168.1.3"))
		{
			display_Swtich2_Error();
			aux[1] = 4;
		}
		break;
	case 5:
		if (!makeRequest("192.168.1.4"))
		{
			display_Swtich2_Error();
			aux[1] = 5;
		}
		break;
	case 6:
		aux[0] = 0;
		switch (aux[1])
		{
		case 1:
			if (makeRequest("google.com"))
			{
				fill_display();
			}

			break;
		case 2:
			if (makeRequest("192.168.1.9:8006"))
			{
				fill_display();
			}
			break;
		case 3:
			if (makeRequest("192.168.1.2"))
			{
				fill_display();
			}
			break;
		case 4:
			if (makeRequest("192.168.1.3"))
			{
				fill_display();
			}
			break;
		case 5:
			if (makeRequest("192.168.1.4"))
			{
				fill_display();
			}
		default:
			break;
		}

		break;
	default:
		aux[0] = 0;
		break;
	}
}

void loop()
{
	long myMillis = millis();

	ArduinoOTA.handle();
	if (tempo[0] + 15 <= myMillis) // controle luz da tela
	{
		controleBack();
		tempo[0] = tempo[0] + 25;
	}
	if (tempo[1] + 500 <= myMillis)
	{
		medidaLuz();
		onTimer();
		resetOnTime(localTime());
		tempo[1] = tempo[1] + 500;
	}
	if (tempo[2] + 2000 <= myMillis)
	{
		pegaValores();
		display_Error(check);
		display(medido[0], medido[1] / 1e2, medido[2], medido[5], medido[4]);
		tempo[2] = tempo[2] + 2000;
	}
	if (tempo[3] + 4000 <= myMillis)
	{
		enviaValores();
		Serial.println("4s OK");
		tempo[3] = tempo[3] + 4000;
	}
	if (tempo[4] + 10000 <= myMillis)
	{
		// verificaRede();
		tempo[4] = tempo[4] + 10000;
	}
}