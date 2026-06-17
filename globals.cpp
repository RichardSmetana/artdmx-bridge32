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
#include "globals.h"

ArtnetWifi artnet;

TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t dmxTaskHandle     = nullptr;
QueueHandle_t dmxQueue         = nullptr;

volatile bool artnetActive = false;
volatile uint32_t lastArtNetPacketMs = 0;
volatile uint32_t lastDmx1OutTrafficMs = 0;
volatile uint32_t lastDmx2OutTrafficMs = 0;
volatile uint32_t lastDmx1InTrafficMs  = 0;
