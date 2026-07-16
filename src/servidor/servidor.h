#pragma once

#include <Arduino.h>
#include <alarm/alarm.h>
#include <Preferences.h>
#include <leds/leds.h>
#include <globals.h>


#include <HTML/alarmHTML.h>
#include <HTML/baseHTML.h>
#include <HTML/erroHTML.h>
#include <HTML/ledHTML.h>
#include <HTML/prometheusHTML.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

void stopServer();
void onTimer();
tm localTime();