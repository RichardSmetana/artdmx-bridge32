/*
 * artdmx-bridge32 - Art-Net to DMX512 gateway for ESP32
 * Copyright (C) 2026 Richard Smetana
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "device_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdarg.h>

static char logBuffer[LOG_LINES][LOG_LINE_LEN];
static uint8_t logHead = 0;
static uint8_t logCount = 0;
static uint32_t logSeq = 0;
static SemaphoreHandle_t logMutex = nullptr;

static void ensureLogMutex() {
  if (logMutex == nullptr) {
    logMutex = xSemaphoreCreateMutex();
  }
}

void appendLog(const char *msg) {
  Serial.println(msg);
  ensureLogMutex();
  if (xSemaphoreTake(logMutex, portMAX_DELAY) != pdTRUE) {
    return;
  }
  strncpy(logBuffer[logHead], msg, LOG_LINE_LEN - 1);
  logBuffer[logHead][LOG_LINE_LEN - 1] = '\0';
  logHead = (logHead + 1) % LOG_LINES;
  if (logCount < LOG_LINES) {
    logCount++;
  }
  logSeq++;
  xSemaphoreGive(logMutex);
}

void appendLogf(const char *fmt, ...) {
  char buf[LOG_LINE_LEN];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  appendLog(buf);
}

static String jsonEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '\\' || c == '"') {
      out += '\\';
    }
    out += c;
  }
  return out;
}

String buildLogJson() {
  ensureLogMutex();
  if (xSemaphoreTake(logMutex, portMAX_DELAY) != pdTRUE) {
    return "{\"seq\":0,\"lines\":[]}";
  }

  String json = "{\"seq\":";
  json += logSeq;
  json += ",\"lines\":[";
  uint8_t start = (logHead + LOG_LINES - logCount) % LOG_LINES;
  for (uint8_t i = 0; i < logCount; i++) {
    if (i > 0) {
      json += ",";
    }
    uint8_t idx = (start + i) % LOG_LINES;
    json += "\"" + jsonEscape(String(logBuffer[idx])) + "\"";
  }
  json += "]}";
  xSemaphoreGive(logMutex);
  return json;
}
