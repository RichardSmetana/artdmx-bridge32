#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>
#include <WebServer.h>

extern WebServer server;

void setupWebServer();
String buildStatusJson();
