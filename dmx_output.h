#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <stdint.h>
#include <esp_dmx.h>

void initDmxOutput();
void sendDmxPacket(const uint8_t *packet, uint16_t bytesToSend);

extern const dmx_port_t DMX_OUT_PORT;
