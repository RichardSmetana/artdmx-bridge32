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
#include "artnet_handler.h"
#include <Arduino.h>
#include <ArtnetWifi.h>
#include <stdio.h>
#include "device_config.h"
#include "device_log.h"
#include "globals.h"
#include "led.h"
#include "dmx_test.h"
#include "ota_manager.h"
#include "types.h"

static uint32_t artnetDebugPacketCount = 0;
static uint8_t lastDebugLogValues[513];
static bool lastDebugLogValid = false;

static void getDebugChannelRange(uint16_t &start, uint16_t &end);

static void rememberDebugLogValues(const uint8_t *frameData) {
  uint16_t start;
  uint16_t end;
  getDebugChannelRange(start, end);

  for (uint16_t ch = start; ch <= end; ch++) {
    lastDebugLogValues[ch] = frameData[ch];
  }
  lastDebugLogValid = true;
}

static bool debugLogRangeChanged(const uint8_t *frameData) {
  uint16_t start;
  uint16_t end;
  getDebugChannelRange(start, end);

  if (!lastDebugLogValid) {
    rememberDebugLogValues(frameData);
    return true;
  }

  for (uint16_t ch = start; ch <= end; ch++) {
    if (frameData[ch] != lastDebugLogValues[ch]) {
      rememberDebugLogValues(frameData);
      return true;
    }
  }
  return false;
}

static void getDebugChannelRange(uint16_t &start, uint16_t &end) {
  start = cfg.artnetDebugChStart;
  end = cfg.artnetDebugChEnd;
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

static bool shouldLogArtNetPacket(bool matched) {
  if (cfg.artnetDebugMode == 0) {
    return false;
  }
  if (cfg.artnetDebugMode == 1 && !matched) {
    return false;
  }
  if (cfg.artnetDebugMode == 2 && matched) {
    return false;
  }

  artnetDebugPacketCount++;
  uint16_t every = cfg.artnetDebugEvery;
  if (every < 1) {
    every = 1;
  }
  return (artnetDebugPacketCount % every) == 0;
}

static void logArtNetChannelValues(uint16_t universe,
                                   uint16_t sequence,
                                   uint16_t length,
                                   const uint8_t *frameData) {
  uint16_t start;
  uint16_t end;
  getDebugChannelRange(start, end);

  char buf[LOG_LINE_LEN];
  int pos = snprintf(buf,
                     sizeof(buf),
                     "ArtNet u%u seq%u len%u ch%u",
                     universe,
                     sequence,
                     length,
                     start);
  if (start != end && pos > 0 && pos < (int)sizeof(buf)) {
    pos += snprintf(buf + pos, sizeof(buf) - pos, "-%u", end);
  }
  if (pos <= 0 || pos >= (int)sizeof(buf) - 2) {
    appendLog("ArtNet log line overflow");
    return;
  }

  buf[pos++] = ':';

  // Keep log lines short; show the full configured range when it fits.
  const uint16_t maxChannels = 16;
  uint16_t showEnd = end;
  if ((end - start + 1) > maxChannels) {
    showEnd = start + maxChannels - 1;
  }

  for (uint16_t ch = start; ch <= showEnd && pos < (int)sizeof(buf) - 6; ch++) {
    int written = snprintf(buf + pos, sizeof(buf) - pos, " %u", frameData[ch]);
    if (written <= 0) {
      break;
    }
    pos += written;
  }

  if (showEnd < end && pos < (int)sizeof(buf) - 8) {
    snprintf(buf + pos, sizeof(buf) - pos, " ...");
  }

  appendLog(buf);
}

void onArtNetFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data) {
  if (isOtaUpdateActive() || dmxTestModeActive()) {
    return;
  }

  bool matched = (universe == cfg.artnetUniverse);

  if (!matched) {
    if (shouldLogArtNetPacket(false)) {
      appendLogf("ArtNet uni %u ignored (want %u) seq%u len%u",
                 universe, cfg.artnetUniverse, sequence, length);
    }
    return;
  }

  DmxFrame frame;
  memset(&frame, 0, sizeof(frame));

  frame.data[0] = 0x00;

  uint16_t dmxSlots = length;
  if (dmxSlots > 512) {
    dmxSlots = 512;
  }

  for (uint16_t i = 0; i < dmxSlots; i++) {
    frame.data[i + 1] = data[i];
  }

  if (cfg.sendFullPacket) {
    frame.bytesToSend = DMX_PACKET_SIZE;
  } else {
    frame.bytesToSend = dmxSlots + 1;
  }

  lastArtNetPacketMs = millis();
  artnetActive = true;
  dmxTestUpdateLastArtNet(frame.data);

  if (dmxQueue != nullptr) {
    xQueueOverwrite(dmxQueue, &frame);
    touchDmx1OutTraffic();
  }

  if (shouldLogArtNetPacket(true)) {
    if (!cfg.artnetDebugOnChange || debugLogRangeChanged(frame.data)) {
      logArtNetChannelValues(universe, sequence, length, frame.data);
    }
  }
}

void initArtNet() {
  artnet.setArtDmxCallback(onArtNetFrame);
  artnet.begin(cfg.hostname);

  appendLogf("ArtNet started, universe: %u", cfg.artnetUniverse);
}
