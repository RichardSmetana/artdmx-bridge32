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
#include "dmx_output2.h"
#include <Arduino.h>
#include "config.h"
#include "device_config.h"
#include "device_log.h"
#include "dmx_output.h"
#include "led.h"
#include "ota_manager.h"

static bool dmx2Ready = false;

bool isDmx2OutputReady() {
  return dmx2Ready;
}

void initDmx2Output() {
  // DMX2 shares the DMX_NUM_1 driver; pins are switched per frame (no DMX_NUM_2).
  initDmx2Led();
  dmx2Ready = true;
  appendLog("DMX2 output ready (shared UART, GPIO 19/18/21)");
}

void sendDmx2Packet(const uint8_t *packet, uint16_t bytesToSend) {
  if (isOtaUpdateActive()) {
    return;
  }
  if (!dmx2Ready) {
    return;
  }
  if (bytesToSend > DMX_PACKET_SIZE) {
    bytesToSend = DMX_PACKET_SIZE;
  }
  if (bytesToSend < 1) {
    bytesToSend = 1;
  }

  if (!dmx_set_pin(DMX_OUT_PORT, DMX2_OUT_TX_PIN, DMX2_OUT_RX_PIN, DMX2_OUT_EN_PIN)) {
    return;
  }

  dmx_write(DMX_OUT_PORT, packet, bytesToSend);

  if (cfg.sendFullPacket) {
    dmx_send_num(DMX_OUT_PORT, DMX_PACKET_SIZE);
  } else {
    dmx_send_num(DMX_OUT_PORT, bytesToSend);
  }

  dmx_wait_sent(DMX_OUT_PORT, DMX_TIMEOUT_TICK);

  dmx_set_pin(DMX_OUT_PORT, DMX_OUT_TX_PIN, DMX_OUT_RX_PIN, DMX_OUT_EN_PIN);
}
