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
#include "dmx_test.h"
#include <Arduino.h>
#include <string.h>
#include "types.h"

static bool testActive = false;
static bool sendOnChangeOnly = true;
static bool liveSliderSync = false;
static uint16_t startChannel = 1;
static uint8_t sliderValues[DMX_TEST_SLIDER_COUNT];
static uint8_t dmxBuffer[DMX_PACKET_SIZE];
static uint8_t lastArtNetFrame[DMX_PACKET_SIZE];
static volatile bool pendingSend = false;

static void clampStartChannel() {
  if (startChannel < 1) {
    startChannel = 1;
  }
  if (startChannel > 512 - DMX_TEST_SLIDER_COUNT + 1) {
    startChannel = 512 - DMX_TEST_SLIDER_COUNT + 1;
  }
}

static void applySlidersToBuffer() {
  memcpy(dmxBuffer, lastArtNetFrame, DMX_PACKET_SIZE);
  dmxBuffer[0] = 0x00;

  for (uint8_t i = 0; i < DMX_TEST_SLIDER_COUNT; i++) {
    uint16_t ch = startChannel + i;
    if (ch >= 1 && ch <= 512) {
      dmxBuffer[ch] = sliderValues[i];
    }
  }
}

void dmxTestInit() {
  testActive = false;
  sendOnChangeOnly = true;
  liveSliderSync = false;
  startChannel = 1;
  memset(sliderValues, 0, sizeof(sliderValues));
  memset(dmxBuffer, 0, sizeof(dmxBuffer));
  memset(lastArtNetFrame, 0, sizeof(lastArtNetFrame));
  pendingSend = false;
}

bool dmxTestModeActive() {
  return testActive;
}

void dmxTestSetActive(bool active) {
  bool wasActive = testActive;
  testActive = active;
  if (active && !wasActive) {
    applySlidersToBuffer();
    if (!sendOnChangeOnly) {
      pendingSend = true;
    }
  } else if (!active) {
    pendingSend = false;
  }
}

bool dmxTestSendOnChangeOnly() {
  return sendOnChangeOnly;
}

void dmxTestSetSendOnChangeOnly(bool onChangeOnly) {
  sendOnChangeOnly = onChangeOnly;
  if (testActive && !onChangeOnly) {
    pendingSend = true;
  } else if (testActive && onChangeOnly) {
    pendingSend = false;
  }
}

bool dmxTestLiveSliderSync() {
  return liveSliderSync;
}

void dmxTestSetLiveSliderSync(bool enabled) {
  liveSliderSync = enabled;
}

void dmxTestRequestSend() {
  if (testActive) {
    pendingSend = true;
  }
}

uint16_t dmxTestStartChannel() {
  return startChannel;
}

void dmxTestSetStartChannel(uint16_t channel) {
  startChannel = channel;
  clampStartChannel();
  dmxTestImportSlidersFromArtNet();
  if (testActive) {
    pendingSend = true;
  }
}

uint8_t dmxTestSliderValue(uint8_t index) {
  if (index >= DMX_TEST_SLIDER_COUNT) {
    return 0;
  }
  return sliderValues[index];
}

bool dmxTestSetSlider(uint8_t index, uint8_t value) {
  if (index >= DMX_TEST_SLIDER_COUNT) {
    return false;
  }

  sliderValues[index] = value;
  uint16_t ch = startChannel + index;
  if (ch >= 1 && ch <= 512) {
    dmxBuffer[ch] = value;
  }
  pendingSend = true;
  return true;
}

bool dmxTestConsumePendingSend() {
  if (!pendingSend) {
    return false;
  }
  pendingSend = false;
  return true;
}

void dmxTestUpdateLastArtNet(const uint8_t *frameData) {
  if (frameData == nullptr) {
    return;
  }
  memcpy(lastArtNetFrame, frameData, DMX_PACKET_SIZE);
}

void dmxTestImportSlidersFromArtNet() {
  for (uint8_t i = 0; i < DMX_TEST_SLIDER_COUNT; i++) {
    uint16_t ch = startChannel + i;
    if (ch >= 1 && ch <= 512) {
      sliderValues[i] = lastArtNetFrame[ch];
    } else {
      sliderValues[i] = 0;
    }
  }
  applySlidersToBuffer();
}

void dmxTestBuildFrame(DmxFrame &frame) {
  memset(frame.data, 0, sizeof(frame.data));
  memcpy(frame.data, dmxBuffer, DMX_PACKET_SIZE);
  frame.bytesToSend = DMX_PACKET_SIZE;
}

String dmxTestBufferDump(uint16_t fromChannel, uint16_t toChannel) {
  if (fromChannel < 1) {
    fromChannel = 1;
  }
  if (toChannel > 512) {
    toChannel = 512;
  }
  if (fromChannel > toChannel) {
    uint16_t tmp = fromChannel;
    fromChannel = toChannel;
    toChannel = tmp;
  }

  String out;
  out.reserve((toChannel - fromChannel + 1) * 8 + 32);
  out += "ch";
  out += String(fromChannel);
  if (fromChannel != toChannel) {
    out += "-";
    out += String(toChannel);
  }
  out += ": ";

  for (uint16_t ch = fromChannel; ch <= toChannel; ch++) {
    if (ch > fromChannel) {
      out += " ";
    }
    out += String(dmxBuffer[ch]);
  }
  return out;
}

String dmxTestFullBufferDump() {
  String out;
  out.reserve(4096);
  out += "DMX buffer (ch1-512):\n";

  for (uint16_t ch = 1; ch <= 512; ch++) {
    if (ch > 1 && ((ch - 1) % 16) == 0) {
      out += "\n";
    }
    if (((ch - 1) % 16) == 0) {
      out += "ch";
      out += String(ch);
      out += "-";
      out += String(min((uint16_t)(ch + 15), (uint16_t)512));
      out += ": ";
    }
    out += String(dmxBuffer[ch]);
    if (ch % 16 != 0 && ch < 512) {
      out += " ";
    }
  }
  return out;
}
