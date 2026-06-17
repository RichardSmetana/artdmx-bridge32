#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <stdint.h>
#include <esp_dmx.h>

// One queue item is a complete DMX packet:
// byte 0 = start code, bytes 1..512 = channels 1..512.
struct DmxFrame {
  uint16_t bytesToSend;
  uint8_t data[DMX_PACKET_SIZE];
};
