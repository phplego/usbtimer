#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_MQTT_Client.h>

String getContentType(String filename);
bool mountSpiffs(void); 
String fileGetContents(const char * filename);
bool loadConfig(const char * filename, std::function<void(DynamicJsonDocument)> onLoadCallback);
bool saveConfig(const char * filename, DynamicJsonDocument json);
bool saveConfig(const char * filename, String jsonStr);
void MQTT_connect(Adafruit_MQTT_Client * mqtt);
String beautifyTime(int millis);