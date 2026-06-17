#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>

void prepareWifi();
void initWifi();
void connectWiFi();
bool isWifiConnected();
bool isApModeActive();
bool isWifiConnecting();
bool isOtaReady();
String apSsid();
