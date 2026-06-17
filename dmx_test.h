#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>
#include "types.h"

#define DMX_TEST_SLIDER_COUNT 16

bool dmxTestModeActive();
void dmxTestInit();
void dmxTestSetActive(bool active);
bool dmxTestSendOnChangeOnly();
void dmxTestSetSendOnChangeOnly(bool onChangeOnly);
bool dmxTestLiveSliderSync();
void dmxTestSetLiveSliderSync(bool enabled);
void dmxTestRequestSend();
uint16_t dmxTestStartChannel();
void dmxTestSetStartChannel(uint16_t channel);
uint8_t dmxTestSliderValue(uint8_t index);
bool dmxTestSetSlider(uint8_t index, uint8_t value);
bool dmxTestConsumePendingSend();
void dmxTestUpdateLastArtNet(const uint8_t *frameData);
void dmxTestImportSlidersFromArtNet();
void dmxTestBuildFrame(DmxFrame &frame);
String dmxTestBufferDump(uint16_t fromChannel, uint16_t toChannel);
String dmxTestFullBufferDump();
