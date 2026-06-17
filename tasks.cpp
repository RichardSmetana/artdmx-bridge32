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
#include "tasks.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "device_config.h"
#include "globals.h"
#include "types.h"
#include "led.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "device_log.h"
#include "dmx_output.h"
#include "dmx_output2.h"
#include "dmx_input.h"
#include "dmx_test.h"
#include "ota_manager.h"

void networkTask(void *parameter) {
  appendLogf("NetworkTask running on core %d", xPortGetCoreID());

  for (;;) {
    checkResetPin();
    connectWiFi();

    if (isWifiConnected()) {
      if (!isOtaUpdateActive() && !dmxTestModeActive()) {
        artnet.read();
      }
      ArduinoOTA.handle();
    }

    if (artnetActive && millis() - lastArtNetPacketMs > cfg.artnetTimeoutMs) {
      artnetActive = false;
      appendLog("ArtNet timeout");
    }

    updateLeds();
    server.handleClient();

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void statusLedTask(void *parameter) {
  for (;;) {
    updateStatusLed();
    vTaskDelay(pdMS_TO_TICKS(STATUS_LED_UPDATE_MS));
  }
}

static void getFilterRange(uint16_t &start, uint16_t &end) {
  start = cfg.dmx2FilterStart;
  end = cfg.dmx2FilterEnd;
  if (start < 1) {
    start = 1;
  }
  if (end > 512) {
    end = 512;
  }
  if (start > end) {
    uint16_t tmp = start;
    start = end;
    end = tmp;
  }
}

static void applyChannelFilter(const uint8_t *source, uint16_t sourceSize, DmxFrame &frame) {
  memset(frame.data, 0, sizeof(frame.data));
  frame.data[0] = 0x00;

  uint16_t start;
  uint16_t end;
  getFilterRange(start, end);

  for (uint16_t ch = start; ch <= end; ch++) {
    if (ch < sourceSize) {
      frame.data[ch] = source[ch];
    }
  }

  if (cfg.sendFullPacket) {
    frame.bytesToSend = DMX_PACKET_SIZE;
  } else {
    frame.bytesToSend = min((uint16_t)DMX_PACKET_SIZE, (uint16_t)(end + 1));
  }
}

static bool filterRangeChanged(const uint8_t *packet, uint8_t *lastValues, bool &hasBaseline) {
  uint16_t start;
  uint16_t end;
  getFilterRange(start, end);

  if (!hasBaseline) {
    return true;
  }

  for (uint16_t ch = start; ch <= end; ch++) {
    if (packet[ch] != lastValues[ch]) {
      return true;
    }
  }
  return false;
}

static void rememberFilterRange(const uint8_t *packet, uint8_t *lastValues, bool &hasBaseline) {
  uint16_t start;
  uint16_t end;
  getFilterRange(start, end);

  for (uint16_t ch = start; ch <= end; ch++) {
    lastValues[ch] = packet[ch];
  }
  hasBaseline = true;
}

static void applyDmx1InputFilter(const uint8_t *source, uint16_t sourceSize, DmxFrame &frame) {
  applyChannelFilter(source, sourceSize, frame);
}

static uint8_t lastDmx2FilterValues[DMX_PACKET_SIZE];
static bool dmx2FilterBaseline = false;

static void sendDmxOutputs(const uint8_t *packet, uint16_t bytesToSend, bool newArtNetFrame) {
  sendDmxPacket(packet, bytesToSend);
  if (newArtNetFrame) {
    touchDmx1OutTraffic();
    blinkDmx1OutLed();
  }

  if (!cfg.enableDmxInput && cfg.enableDmx2Output && isDmx2OutputReady()) {
    if (filterRangeChanged(packet, lastDmx2FilterValues, dmx2FilterBaseline)) {
      DmxFrame dmx2Frame;
      applyChannelFilter(packet, DMX_PACKET_SIZE, dmx2Frame);
      sendDmx2Packet(dmx2Frame.data, dmx2Frame.bytesToSend);
      rememberFilterRange(packet, lastDmx2FilterValues, dmx2FilterBaseline);
      touchDmx2OutTraffic();
      if (newArtNetFrame) {
        blinkDmx2OutLed();
      }
    }
  }
}

void dmxTask(void *parameter) {
  appendLogf("DmxTask running on core %d", xPortGetCoreID());

  DmxFrame currentFrame;
  memset(&currentFrame, 0, sizeof(currentFrame));
  currentFrame.data[0] = 0x00;
  currentFrame.bytesToSend = DMX_PACKET_SIZE;

  uint32_t lastRefreshMs = 0;
  uint32_t lastDmxInLogMs = 0;

  sendDmxOutputs(currentFrame.data, currentFrame.bytesToSend, false);

  for (;;) {
    if (isOtaUpdateActive()) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (dmxTestModeActive()) {
      bool doSend = false;
      if (dmxTestSendOnChangeOnly()) {
        doSend = dmxTestConsumePendingSend();
        if (!doSend) {
          vTaskDelay(pdMS_TO_TICKS(5));
        }
      } else if (dmxTestConsumePendingSend()) {
        doSend = true;
      } else if (millis() - lastRefreshMs >= cfg.dmxRefreshMs) {
        doSend = true;
      }

      if (doSend) {
        DmxFrame testFrame;
        dmxTestBuildFrame(testFrame);
        currentFrame = testFrame;
        sendDmxOutputs(currentFrame.data, currentFrame.bytesToSend, true);
        lastRefreshMs = millis();
      }
      taskYIELD();
      continue;
    }

    DmxFrame newFrame;

    if (xQueueReceive(dmxQueue, &newFrame, pdMS_TO_TICKS(2)) == pdTRUE) {
      currentFrame = newFrame;
      sendDmxOutputs(currentFrame.data, currentFrame.bytesToSend, true);
      lastRefreshMs = millis();
    }

    if (millis() - lastRefreshMs >= cfg.dmxRefreshMs) {
      sendDmxOutputs(currentFrame.data, currentFrame.bytesToSend, false);
      lastRefreshMs = millis();
    }

    if (cfg.enableDmxInput && !artnetActive) {
      dmx_packet_t packet;
      int packetSize = dmx_receive(DMX_IN_PORT, &packet, 0);

      if (packetSize > 0) {
        if (packet.err == DMX_OK) {
          memset(dmxInData, 0, DMX_PACKET_SIZE);
          dmx_read(DMX_IN_PORT, dmxInData, packet.size);

          applyDmx1InputFilter(dmxInData, packet.size, currentFrame);
          sendDmxPacket(currentFrame.data, currentFrame.bytesToSend);
          blinkDmx1OutLed();
          blinkDmx1InLed();

          if (millis() - lastDmxInLogMs > 1000) {
            lastDmxInLogMs = millis();
            appendLogf("DMX1 input ch %u-%u forwarded, bytes: %d",
                       cfg.dmx2FilterStart,
                       cfg.dmx2FilterEnd,
                       packet.size);
          }
        } else {
          appendLogf("DMX1 input error: %d", packet.err);
        }
      }
    }

    taskYIELD();
  }
}
