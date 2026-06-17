#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <esp_dmx.h>

extern const dmx_port_t DMX_IN_PORT;
extern uint8_t dmxInData[DMX_PACKET_SIZE];

void initDmxInput();
