#include <servidor/servidor.h>

AsyncWebServer server(80);
HTTPClient http;
AsyncClient TCP;
bool FLED = true;
bool FLED_override = false;
uint8_t saveAlarme = true;
uint16_t hue = 0;
uint8_t sat = 0;
uint8_t intenc = 0;
Preferences prefs;
uint8_t horarioLuz[quantidadeAlarmes][4] = {
    {9, 0, 15, 25}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0},   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
};
uint8_t alarme = 0;
bool started = false;
uint64_t vezes = 0;

/// @brief inicia o armazenamento para dados persisitentes
void storageInit() {
  prefs.begin("alarms", false);
  for (uint8_t i = 0; i < quantidadeAlarmes; i++) {
    for (size_t j = 0; j < 4; j++) {
      String chave = (i * 4 + j) + "a";
      horarioLuz[i][j] = prefs.getInt(chave.c_str(), 0);
    }
  }
}

/// @brief HTML da pagina de controle dos alarmes `/alarme`
String SendAlarmeHTML(tm data) {
  String html = ALARM_HTML;

  html.replace("%HORA%", String(horarioLuz[saveAlarme - 1][0]));
  html.replace("%MINUTO%", String(horarioLuz[saveAlarme - 1][1]));
  html.replace("%FADE%", String(horarioLuz[saveAlarme - 1][2]));
  html.replace("%MAX%", String(horarioLuz[saveAlarme - 1][3]));

  char dateStr[30];
  sprintf(dateStr, "%d-%02d-%02d %02d:%02d:%02d", data.tm_year + 1900,
          data.tm_mon + 1, data.tm_mday, data.tm_hour, data.tm_min,
          data.tm_sec);
  html.replace("%DATA%", String(dateStr));

  html.replace(
      "%POTENCIA%",
      String((int)(100 * vezes / (horarioLuz[alarme][2] * 60.0 * 2.0))));
  html.replace("%VEZES%", String((int)vezes));
  html.replace("%DURACAO%", String(horarioLuz[alarme][2]));
  return html;
}

String SendEcolhaAlarmeHTML() {
  String ptr = ESCOLHA_ALARME_INICIO_HTML;
#ifdef save
  ptr += ESCOLHA_ALARME_SAVE_HTML;
#endif
  ptr += ESCOLHA_ALARME_MEIO_HTML;
  for (uint8_t i = 0; i < quantidadeAlarmes; i++) {
    ptr += "<tr>";
    ptr += "<td>";
    ptr += i + 1;
    ptr += "</td>";
    for (uint8_t j = 0; j < 4; j++) {
      ptr += "<td>";
      ptr += horarioLuz[i][j];
      ptr += "</td>";
    }
    ptr += "</tr>";
  }
  ptr += ESCOLHA_ALARME_FINAL_HTML;

  return ptr;
}

void handle_alarme(AsyncWebServerRequest *request) {
  int paramsNr = request->params();
  int ok = 0;
  for (int i = 0; i < paramsNr; i++) {
    AsyncWebParameter *p = request->getParam(i);
    if (p->name() == "alarme") {
      if ((p->value()).toInt() < quantidadeAlarmes) {
        saveAlarme = (p->value()).toInt();
      } else {
        request->send(200, "text/html", SendAlarmeHTML(localTime()));
        return;
      }
    }
    if (p->name() == "hora") {
      horarioLuz[saveAlarme - 1][0] = (p->value()).toInt();
      ok++;
    }
    if (p->name() == "minuto") {
      horarioLuz[saveAlarme - 1][1] = (p->value()).toInt();
      ok++;
    }
    if (p->name() == "fade") {
      horarioLuz[saveAlarme - 1][2] = (p->value()).toInt();
      ok++;
    }
    if (p->name() == "max") {
      horarioLuz[saveAlarme - 1][3] = (p->value()).toInt();
      ok++;
    }
    if (ok == 4) {
      for (uint8_t i = 0; i < quantidadeAlarmes; i++) {
        for (size_t j = 0; j < 4; j++) {
          String chave = (i * 4 + j) + "a";
          prefs.putInt(chave.c_str(), horarioLuz[i][j]);
        }
      }
      prefs.end();
      storageInit();
      saveAlarme = 0;
    }
  }
  if (saveAlarme != 0)
    request->send(200, "text/html", SendAlarmeHTML(localTime()));
  else
    request->send(200, "text/html", SendEcolhaAlarmeHTML());
}





/// @brief HTML da pagina de controle dos leds `/led`
String SendLEDHTML() {
  String html = LED_HTML;

  html.replace("%LED_F_STATE%", FLED ? "off" : "on");
  html.replace("%LED_F_BUTTON%", FLED ? "on" : "off");
  html.replace("%LED_F_LABEL%", FLED ? "ON" : "OFF");

  html.replace("%LED_O_STATE%", FLED_override ? "off" : "on");
  html.replace("%LED_O_BUTTON%", FLED_override ? "on" : "off");
  html.replace("%LED_O_LABEL%", FLED_override ? "OVERRIDE ON" : "OVERRIDE OFF");

  html.replace("%HUE%", String(hue));
  html.replace("%SAT%", String(sat));
  html.replace("%INT%", String(intenc));

  struct tm data =
      localTime(); // Suponha que localTime() retorne uma struct tm válida
  char dateStr[50];
  sprintf(dateStr, "%d - %02d:%02d:%02d", data.tm_wday, data.tm_hour,
          data.tm_min, data.tm_sec);
  html.replace("%DATE%", String(dateStr));

  int potenciaAtual = (int)(100 * vezes / (horarioLuz[alarme][2] * 60.0 * 2.0));
  html.replace("%POTENCIA_ATUAL%", String(potenciaAtual));
  html.replace("%VEZES_ATUAL%", String(vezes));
  html.replace("%DURACAO_ATUAL%", String(horarioLuz[alarme][2]));
  return html;
}

/// @brief HTML da pagina inicial `/`
String SendbaseHTML() {
  String html = BASE_HTML;
  html.replace("%TEMP_QUARTO%", String(medido[0]));
  html.replace("%TEMP_CAIXA%", String(tempetura[0]));
  html.replace("%TEMP_ESP%", String(temperatureRead()));
  html.replace("%PRESSAO%", String(medido[1]));
  html.replace("%LUZ1%", String(medido[2]));
  html.replace("%LUZ2%", String(medido[3]));
  html.replace("%CO2%", String(medido[4]));
  html.replace("%HUMIDADE%", String(medido[5]));
  return html;
}

/// @brief HTML da pagina do prometheus `/metrics`
String SendPrometheusHTML() {
  String html = PROMETHEUS_HTML;
  html.replace("%TEMP_QUARTO%", String(medido[0]));
  html.replace("%TEMP_CAIXA%", String(tempetura[0]));
  html.replace("%TEMP_ESP%", String(temperatureRead()));
  html.replace("%PRESSAO%", String(medido[1]));
  html.replace("%LUZ1%", String(medido[2]));
  html.replace("%LUZ2%", String(medido[3]));
  html.replace("%CO2%", String(medido[4]));
  html.replace("%HUMIDADE%", String(medido[5]));
  return html;
}

String SendERRORHTML() { return ERROR_HTML; }

void handle_OnConnect(AsyncWebServerRequest *request) {
  request->send(200, "text/html", SendbaseHTML());
}

void handle_Prometheus(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", SendPrometheusHTML());
}

void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void handle_led_v2(AsyncWebServerRequest *request) {
  int paramsNr = request->params();

  for (int i = 0; i < paramsNr; i++) {
    AsyncWebParameter *p = request->getParam(i);
    if (p->name() == "Fon") {
      FLED = true;
    }
    if (p->name() == "Foff") {
      FLED = false;
    }
    if (p->name() == "hue") {
      hue = (p->value()).toInt();
    }
    if (p->name() == "sat") {
      sat = (p->value()).toInt();
    }
    if (p->name() == "int") {
      intenc = (p->value()).toInt();
    }
    if (p->name() == "Oon") {
      FLED_override = true;
    }
    if (p->name() == "Ooff") {
      FLED_override = false;
    }
  }
  if (FLED_override) {

    preencheLeds((float)hue * 255 / 360, sat * 2.55, intenc * 2.55);
  } else {
    preencheLeds(25, 255, 0);
  }
  FastLED.show();
  request->send(200, "text/html", SendLEDHTML());
}

tm localTime() {
  // https://cplusplus.com/reference/ctime/tm/
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return timeinfo;
  }
  return timeinfo;
}

void alarmeControl(struct tm timeinfo) {
  if (FLED_override)
    return;
  if (!started) {
    if (timeinfo.tm_wday == 0)
      return;
    if (timeinfo.tm_wday == 6)
      return;
    bool ok = false;
    for (uint8_t i = 0; i < sizeof horarioLuz / sizeof horarioLuz[0]; i++) {
      if (timeinfo.tm_hour == horarioLuz[i][0])
        if (timeinfo.tm_min == horarioLuz[i][1]) {
          ok = true;
          alarme = i;
          break;
        }
    }
    if (!ok) {
      return;
    }

    started = true;
  }
}

void serverSetup() {
  server.on("/",
            [](AsyncWebServerRequest *request) { handle_OnConnect(request); });
  server.on("/led",
            [](AsyncWebServerRequest *request) { handle_led_v2(request); });
  server.on("/alarme",
            [](AsyncWebServerRequest *request) { handle_alarme(request); });
  server.on("/metrics",
            [](AsyncWebServerRequest *request) { handle_Prometheus(request); });
  server.onNotFound(
      [](AsyncWebServerRequest *request) { handle_NotFound(request); });
  server.begin();
}

void stopServer() { server.end(); }

/// @brief cuida de executar as luzes dos alarmes cadastrados
void onTimer() {
  if (!started) {
    return;
  }
  if (horarioLuz[alarme][0] == 0) {
    if (horarioLuz[alarme][1] == 0) {
      return;
    }
  }

  if (vezes <= (horarioLuz[alarme][2] + horarioLuz[alarme][3]) * 60 * 2) {
    uint16_t potencia = min((uint64_t)(255.0 * (double)vezes /
                                       (horarioLuz[alarme][2] * 60.0 * 2.0)),
                            (uint64_t)254);
    preencheLeds(25, 255, potencia);
    vezes++;
    FastLED.show();
  } else {
    preencheLeds(25, 255, 0);
    vezes = 0;
    started = false;
    FastLED.show();
  }
}

bool makeRequest(String serverName) {
  http.begin(serverName.c_str());
  if (http.GET() > 0) {
    return true;
  }
  return false;
}