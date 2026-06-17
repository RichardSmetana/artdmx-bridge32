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
#include "led.h"
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "globals.h"
#include "ota_manager.h"
#include "wifi_manager.h"
#include "dmx_test.h"

static uint32_t dmx1OutLedUntilMs = 0;
static uint32_t dmx2OutLedUntilMs = 0;
static uint32_t dmx1InLedUntilMs  = 0;

static bool trafficRecent(volatile uint32_t &lastMs) {
  uint32_t last = lastMs;
  return last != 0 && (millis() - last) < TRAFFIC_WEB_ACTIVE_MS;
}

static void setStatusLedLevel(uint8_t level) {
  ledcWrite(LED_STATUS_PIN, level);
}

void initLeds() {
  pinMode(LED_DMX1_PIN, OUTPUT);
  digitalWrite(LED_DMX1_PIN, LOW);
  pinMode(LED_STATUS_PIN, OUTPUT);
  ledcAttach(LED_STATUS_PIN, 1000, 8);
  setStatusLedLevel(0);
}

void initDmx2Led() {
  pinMode(LED_DMX2_PIN, OUTPUT);
  digitalWrite(LED_DMX2_PIN, LOW);
}

void blinkDmx1OutLed() {
  if (isOtaUpdateActive()) {
    return;
  }
  uint32_t now = millis();
  dmx1OutLedUntilMs = now + LED_ACTIVITY_MS;
  lastDmx1OutTrafficMs = now;
  digitalWrite(LED_DMX1_PIN, HIGH);
}

void blinkDmx2OutLed() {
  if (isOtaUpdateActive()) {
    return;
  }
  uint32_t now = millis();
  dmx2OutLedUntilMs = now + LED_ACTIVITY_MS;
  lastDmx2OutTrafficMs = now;
  digitalWrite(LED_DMX2_PIN, HIGH);
}

void blinkDmx1InLed() {
  if (isOtaUpdateActive()) {
    return;
  }
  uint32_t now = millis();
  dmx1InLedUntilMs = now + LED_ACTIVITY_MS;
  lastDmx1InTrafficMs = now;
  digitalWrite(LED_DMX2_PIN, HIGH);
}

void touchDmx1OutTraffic() {
  if (isOtaUpdateActive()) {
    return;
  }
  lastDmx1OutTrafficMs = millis();
}

void touchDmx2OutTraffic() {
  if (isOtaUpdateActive()) {
    return;
  }
  lastDmx2OutTrafficMs = millis();
}

void touchDmx1InTraffic() {
  if (isOtaUpdateActive()) {
    return;
  }
  lastDmx1InTrafficMs = millis();
}

void disableDmxTrafficLeds() {
  dmx1OutLedUntilMs = 0;
  dmx2OutLedUntilMs = 0;
  dmx1InLedUntilMs = 0;
  digitalWrite(LED_DMX1_PIN, LOW);
  digitalWrite(LED_DMX2_PIN, LOW);
}

void updateLeds() {
  if (isOtaUpdateActive()) {
    disableDmxTrafficLeds();
    return;
  }

  uint32_t now = millis();
  if (dmx1OutLedUntilMs != 0 && now >= dmx1OutLedUntilMs) {
    digitalWrite(LED_DMX1_PIN, LOW);
    dmx1OutLedUntilMs = 0;
  }
  if (dmx2OutLedUntilMs != 0 && now >= dmx2OutLedUntilMs) {
    digitalWrite(LED_DMX2_PIN, LOW);
    dmx2OutLedUntilMs = 0;
  }
  if (dmx1InLedUntilMs != 0 && now >= dmx1InLedUntilMs) {
    digitalWrite(LED_DMX2_PIN, LOW);
    dmx1InLedUntilMs = 0;
  }
}

void updateStatusLed() {
  uint32_t now = millis();

  if (isOtaUpdateActive()) {
    disableDmxTrafficLeds();

    const uint32_t on = WIFI_OTA_TRIPLE_ON_MS;
    const uint32_t gap = WIFI_OTA_TRIPLE_GAP_MS;
    const uint32_t step = on + gap;
    const uint32_t cycle = step * 3 + WIFI_OTA_TRIPLE_PAUSE_MS;
    const uint32_t phase = now % cycle;
    bool ledOn = false;

    for (int i = 0; i < 3; i++) {
      uint32_t start = (uint32_t)i * step;
      if (phase >= start && phase < start + on) {
        ledOn = true;
        break;
      }
    }

    setStatusLedLevel(ledOn ? 255 : 0);
    return;
  }

  if (dmxTestModeActive()) {
    const uint32_t period = DMX_TEST_BREATH_MS;
    const uint32_t half = period / 2;
    const uint32_t phase = now % period;
    uint8_t level;

    if (phase < half) {
      level = (uint8_t)((phase * 255UL) / half);
    } else {
      level = (uint8_t)(255 - ((phase - half) * 255UL) / half);
    }

    // Keep a visible minimum so the LED never fully disappears.
    level = (uint8_t)(40 + ((uint16_t)level * 215U / 255U));
    setStatusLedLevel(level);
    return;
  }

  if (isWifiConnected()) {
    setStatusLedLevel(255);
    return;
  }

  if (isApModeActive()) {
    uint32_t cycle = WIFI_AP_DOUBLE_ON_MS + WIFI_AP_DOUBLE_GAP_MS +
                     WIFI_AP_DOUBLE_ON_MS + WIFI_AP_DOUBLE_PAUSE_MS;
    uint32_t phase = now % cycle;
    uint32_t secondOnStart = WIFI_AP_DOUBLE_ON_MS + WIFI_AP_DOUBLE_GAP_MS;
    bool on = phase < WIFI_AP_DOUBLE_ON_MS ||
              (phase >= secondOnStart &&
               phase < secondOnStart + WIFI_AP_DOUBLE_ON_MS);
    setStatusLedLevel(on ? 255 : 0);
    return;
  }

  if (isWifiConnecting()) {
    bool on = (now % (WIFI_STATUS_BLINK_MS * 2)) < WIFI_STATUS_BLINK_MS;
    setStatusLedLevel(on ? 255 : 0);
    return;
  }

  setStatusLedLevel(0);
}

bool isDmx1OutTrafficActive() {
  if (isOtaUpdateActive()) {
    return false;
  }
  return trafficRecent(lastDmx1OutTrafficMs);
}

bool isDmx2OutTrafficActive() {
  if (isOtaUpdateActive()) {
    return false;
  }
  return trafficRecent(lastDmx2OutTrafficMs);
}

bool isDmx1InTrafficActive() {
  if (isOtaUpdateActive()) {
    return false;
  }
  return trafficRecent(lastDmx1InTrafficMs);
}

bool isDmx1OutTrafficForWeb() {
  return isDmx1OutTrafficActive();
}

bool isDmx2OutTrafficForWeb() {
  return isDmx2OutTrafficActive();
}
