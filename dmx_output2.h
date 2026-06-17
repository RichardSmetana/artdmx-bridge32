#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <stdint.h>

void initDmx2Output();
void sendDmx2Packet(const uint8_t *packet, uint16_t bytesToSend);
bool isDmx2OutputReady();
