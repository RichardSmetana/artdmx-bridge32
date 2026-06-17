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
#include "config.h"
#include <Arduino.h>
#include "device_log.h"

// ---------------------------------------------------------------------------
// GPIO map (no pin used twice)
//
//  DMX1 output : TX=17  RX=16  EN=4   (DMX_NUM_1)
//  DMX2 output : TX=19  RX=18  EN=21  (same UART — pins switched per frame)
//  DMX1 input  : TX=19  RX=18  EN=21  (DMX_NUM_2 — needs esp_dmx UART2 patch)
//  Traffic LED : DMX1=13  DMX2/in=14  WiFi status=22
//  Reset       : 15 (hold LOW 3 s)
//
// Avoid GPIO 6–11 (flash), 0/2/4/5/12/15 strapping at boot where possible.
// ---------------------------------------------------------------------------

const int LED_DMX1_PIN   = 13;
const int LED_DMX2_PIN   = 14;
const int LED_STATUS_PIN = 22;
const int RESET_PIN      = 15;

const int DMX_OUT_TX_PIN = 17;
const int DMX_OUT_RX_PIN = 16;
const int DMX_OUT_EN_PIN = 4;

const int DMX2_OUT_TX_PIN = 19;
const int DMX2_OUT_RX_PIN = 18;
const int DMX2_OUT_EN_PIN = 21;

const int DMX1_IN_TX_PIN = DMX2_OUT_TX_PIN;
const int DMX1_IN_RX_PIN = DMX2_OUT_RX_PIN;
const int DMX1_IN_EN_PIN = DMX2_OUT_EN_PIN;

const BaseType_t NETWORK_TASK_CORE = 0;
const BaseType_t DMX_TASK_CORE     = 1;

const UBaseType_t NETWORK_TASK_PRIO = 1;
const UBaseType_t DMX_TASK_PRIO     = 3;
const UBaseType_t STATUS_LED_TASK_PRIO = 0;

const uint32_t NETWORK_TASK_STACK = 8192;
const uint32_t DMX_TASK_STACK     = 6144;
const uint32_t STATUS_LED_TASK_STACK = 2048;

static bool gpioUsedByOther(int pin, const int *others, size_t count) {
  if (pin < 0) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    if (pin == others[i]) {
      return true;
    }
  }
  return false;
}

void validateGpioMap() {
  const int dmxPins[] = {
    DMX_OUT_TX_PIN, DMX_OUT_RX_PIN, DMX_OUT_EN_PIN,
    DMX2_OUT_TX_PIN, DMX2_OUT_RX_PIN, DMX2_OUT_EN_PIN
  };
  const int ledPins[] = { LED_DMX1_PIN, LED_DMX2_PIN, LED_STATUS_PIN };

  if (LED_DMX1_PIN == LED_DMX2_PIN) {
    appendLog("ERROR: LED_DMX1_PIN and LED_DMX2_PIN must differ");
  }

  for (int led : ledPins) {
    if (led == RESET_PIN) {
      appendLogf("ERROR: LED GPIO %d conflicts with RESET_PIN", led);
    }
    if (gpioUsedByOther(led, dmxPins, sizeof(dmxPins) / sizeof(dmxPins[0]))) {
      appendLogf("ERROR: LED GPIO %d conflicts with a DMX pin", led);
    }
  }
}
