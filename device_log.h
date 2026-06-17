#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>

#define LOG_LINES    32
#define LOG_LINE_LEN 80

void appendLog(const char *msg);
void appendLogf(const char *fmt, ...);
String buildLogJson();
