#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <Arduino.h>
#include <ArtnetWifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "config.h"
#include "types.h"

extern ArtnetWifi artnet;

extern TaskHandle_t networkTaskHandle;
extern TaskHandle_t dmxTaskHandle;
extern QueueHandle_t dmxQueue;

extern volatile bool artnetActive;
extern volatile uint32_t lastArtNetPacketMs;
extern volatile uint32_t lastDmx1OutTrafficMs;
extern volatile uint32_t lastDmx2OutTrafficMs;
extern volatile uint32_t lastDmx1InTrafficMs;
