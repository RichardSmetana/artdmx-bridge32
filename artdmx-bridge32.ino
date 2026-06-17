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
 *
 * DmxTask (Core 1): DMX1 output (always), DMX2 output or DMX1 input on port 2.
 * Note: ESP32 Core 3.3.x + esp_dmx 4.x may require a library patch — see README.
 */

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "globals.h"
#include "device_log.h"
#include "device_config.h"
#include "dmx_test.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "ota_manager.h"
#include "dmx_output.h"
#include "dmx_output2.h"
#include "dmx_input.h"
#include "artnet_handler.h"
#include "led.h"
#include "tasks.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  appendLogf("%s %s starting...", DEVICE_NAME, VERSION);
  appendLog(COPYRIGHT_LINE);
  appendLog(LICENSE_LINE);

  pinMode(RESET_PIN, INPUT_PULLUP);
  dmxTestInit();
  loadConfigFromNvs();
  appendLog("DMX test mode off (default after boot)");
  validateGpioMap();
  initLeds();

  dmxQueue = xQueueCreate(1, sizeof(DmxFrame));
  if (dmxQueue == nullptr) {
    appendLog("ERROR: failed to create dmxQueue");
    delay(2000);
    ESP.restart();
  }

  // Initialize DMX hardware before WiFi/web — matches original gateway boot order.
  initDmxOutput();

  if (cfg.enableDmxInput) {
    initDmxInput();
    appendLog("DMX1 input enabled (DMX2 output disabled)");
  } else if (cfg.enableDmx2Output) {
    initDmx2Output();
    appendLog("DMX2 output enabled");
  } else {
    appendLog("DMX1 output only (port 2 unused)");
  }

  prepareWifi();
  setupWebServer();

  xTaskCreatePinnedToCore(
    statusLedTask,
    "StatusLedTask",
    STATUS_LED_TASK_STACK,
    nullptr,
    STATUS_LED_TASK_PRIO,
    nullptr,
    NETWORK_TASK_CORE
  );

  initWifi();

  initArtNet();

  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    NETWORK_TASK_STACK,
    nullptr,
    NETWORK_TASK_PRIO,
    &networkTaskHandle,
    NETWORK_TASK_CORE
  );

  xTaskCreatePinnedToCore(
    dmxTask,
    "DmxTask",
    DMX_TASK_STACK,
    nullptr,
    DMX_TASK_PRIO,
    &dmxTaskHandle,
    DMX_TASK_CORE
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
