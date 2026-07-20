#include <failsafe/failsafe.h>


void espRestartSafe(){
  for (uint8_t i = 0; i < 50; i++)
  {
    ArduinoOTA.handle();
    delay(25);
  }
  esp_restart();
}

void ArduinoOTAInit() {
  ArduinoOTA
      .onStart([]() {
        update = true;
        stopServer();
        updateTft();

        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        updateTft(progress, total);
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        updateTft(error);
        delay(10000);
        espRestartSafe();
      });
  ArduinoOTA.begin();
}