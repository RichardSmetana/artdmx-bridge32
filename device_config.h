#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>
#include <Preferences.h>

#define NVS_NAMESPACE   "artdmx-bridge32"
#define WIFI_SSID_MAX   32
#define WIFI_PASS_MAX   63
#define HOSTNAME_MAX    31
#define OTA_PASS_MAX    31
#define WEB_PASS_MAX    31

struct DeviceConfig {
  char wifiSsid[WIFI_SSID_MAX + 1];
  char wifiPass[WIFI_PASS_MAX + 1];
  char hostname[HOSTNAME_MAX + 1];
  char otaPassword[OTA_PASS_MAX + 1];
  char webPassword[WEB_PASS_MAX + 1];
  uint16_t artnetUniverse;
  uint32_t artnetTimeoutMs;
  uint32_t dmxRefreshMs;
  bool sendFullPacket;
  bool enableDmxInput;
  bool enableDmx2Output;
  uint16_t dmx2FilterStart;
  uint16_t dmx2FilterEnd;
  uint8_t artnetDebugMode;   // 0=off 1=matched universe 2=ignored 3=all
  uint16_t artnetDebugEvery; // log every Nth packet when mode != 0
  uint16_t artnetDebugChStart;
  uint16_t artnetDebugChEnd;
  bool artnetDebugOnChange;
};

extern Preferences prefs;
extern DeviceConfig cfg;

void loadConfigFromNvs();
void saveConfigToNvs();
void resetConfigToDefaults();
void factoryResetAndRestart();
void checkResetPin();
bool copyArgField(char *dest, size_t destSize, const char *value, const char *fieldName);
String buildConfigJson();
String exportConfigText();
bool importConfigText(const String &text, String &error);
String jsonEscape(const String &s);
