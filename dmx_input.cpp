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
#include "dmx_input.h"

#include <Arduino.h>
#include <esp_dmx.h>
#include "config.h"
#include "device_log.h"
#include "led.h"

const dmx_port_t DMX_IN_PORT = DMX_NUM_2;
uint8_t dmxInData[DMX_PACKET_SIZE];

void initDmxInput() {
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {
    {1, "DMX1 Input"}
  };

  dmx_driver_install(DMX_IN_PORT, &config, personalities, 1);
  dmx_set_pin(DMX_IN_PORT, DMX1_IN_TX_PIN, DMX1_IN_RX_PIN, DMX1_IN_EN_PIN);

  memset(dmxInData, 0, DMX_PACKET_SIZE);
  initDmx2Led();
  appendLog("DMX input (DMX1) initialized");
}
