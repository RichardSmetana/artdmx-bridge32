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
#include "ota_manager.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "device_config.h"
#include "device_log.h"
#include "dmx_output.h"
#include "dmx_output2.h"
#include "globals.h"
#include "led.h"
#include "types.h"

static bool otaReady = false;
static volatile bool otaUpdateActive = false;

bool otaIsReady() {
  return otaReady;
}

bool isOtaUpdateActive() {
  return otaUpdateActive;
}

void otaResetReady() {
  otaReady = false;
}

static void sendDmxBlackout() {
  uint8_t blackout[DMX_PACKET_SIZE];
  memset(blackout, 0, sizeof(blackout));

  sendDmxPacket(blackout, DMX_PACKET_SIZE);
  if (isDmx2OutputReady()) {
    sendDmx2Packet(blackout, DMX_PACKET_SIZE);
  }
}

static void onOtaUpdateStart() {
  appendLog("OTA update started — DMX disabled");
  sendDmxBlackout();

  otaUpdateActive = true;
  artnetActive = false;

  if (dmxQueue != nullptr) {
    xQueueReset(dmxQueue);
  }

  disableDmxTrafficLeds();
  updateStatusLed();
}

static void onOtaUpdateFinished() {
  otaUpdateActive = false;
}

void setupOTA() {
  ArduinoOTA.setHostname(cfg.hostname);
  if (strlen(cfg.otaPassword) > 0) {
    ArduinoOTA.setPassword(cfg.otaPassword);
  }

  ArduinoOTA.onStart([]() {
    onOtaUpdateStart();
  });

  ArduinoOTA.onEnd([]() {
    appendLog("OTA update finished");
    onOtaUpdateFinished();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
    updateStatusLed();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    appendLogf("OTA error code: %u", error);
    onOtaUpdateFinished();
  });

  ArduinoOTA.begin();
  otaReady = true;
  appendLogf("OTA ready as '%s'", cfg.hostname);
}
