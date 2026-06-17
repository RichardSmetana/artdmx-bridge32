#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---------------------------------------------------------------------------
// Device identity
// ---------------------------------------------------------------------------
#define DEVICE_NAME    "artdmx-bridge32"
#define VERSION        "2.3.0-web-dmx"

#include "license.h"

// First-boot defaults (overridden via web UI after save).
#define DEFAULT_HOSTNAME          "artdmx-bridge32"
#define DEFAULT_WEB_PASSWORD      ""
#define DEFAULT_ARTNET_UNIVERSE   0
#define DEFAULT_ARTNET_TIMEOUT_MS 15000
#define DEFAULT_DMX_REFRESH_MS    30
#define DEFAULT_SEND_FULL_PACKET  1
#define DEFAULT_ENABLE_DMX_INPUT  0
#define DEFAULT_ENABLE_DMX2_OUTPUT 0
#define DEFAULT_DMX2_FILTER_START 1
#define DEFAULT_DMX2_FILTER_END   512
#define DEFAULT_DEBUG_ARTNET      0
#define DEFAULT_ARTNET_DEBUG_MODE 0   // 0=off 1=matched 2=ignored 3=all
#define DEFAULT_ARTNET_DEBUG_EVERY 1  // log every Nth packet (when mode != 0)
#define DEFAULT_ARTNET_DEBUG_CH_START 1
#define DEFAULT_ARTNET_DEBUG_CH_END   4
#define DEFAULT_ARTNET_DEBUG_ON_CHANGE 0

#define WIFI_STATUS_BLINK_MS     500
#define WIFI_AP_DOUBLE_ON_MS     120
#define WIFI_AP_DOUBLE_GAP_MS    120
#define WIFI_AP_DOUBLE_PAUSE_MS  640
#define WIFI_OTA_TRIPLE_ON_MS    120
#define WIFI_OTA_TRIPLE_GAP_MS   120
#define WIFI_OTA_TRIPLE_PAUSE_MS 520

#define WIFI_RECONNECT_MS        5000
#define WIFI_CONNECT_TIMEOUT_MS  5000
#define WIFI_HEALTH_CHECK_MS     2000
#define RESET_HOLD_MS            3000
#define LED_ACTIVITY_MS          150
#define TRAFFIC_WEB_ACTIVE_MS    2000
#define STATUS_LED_UPDATE_MS     50
#define DMX_TEST_BREATH_MS     2000

// ---------------------------------------------------------------------------
// GPIO pins (hardware wiring — not web-configurable)
// ---------------------------------------------------------------------------
extern const int LED_DMX1_PIN;
extern const int LED_DMX2_PIN;
extern const int LED_STATUS_PIN;
extern const int RESET_PIN;

// DMX1 output (DMX_NUM_1)
extern const int DMX_OUT_TX_PIN;
extern const int DMX_OUT_RX_PIN;
extern const int DMX_OUT_EN_PIN;

// Port 2 GPIOs: DMX2 output (pin switching on DMX_NUM_1) or DMX1 input (DMX_NUM_2)
extern const int DMX2_OUT_TX_PIN;
extern const int DMX2_OUT_RX_PIN;
extern const int DMX2_OUT_EN_PIN;
extern const int DMX1_IN_TX_PIN;
extern const int DMX1_IN_RX_PIN;
extern const int DMX1_IN_EN_PIN;

// ---------------------------------------------------------------------------
// FreeRTOS task layout
// Core 0: network / ArtNet / OTA
// Core 1: DMX output
// ---------------------------------------------------------------------------
extern const BaseType_t NETWORK_TASK_CORE;
extern const BaseType_t DMX_TASK_CORE;
extern const UBaseType_t NETWORK_TASK_PRIO;
extern const UBaseType_t DMX_TASK_PRIO;
extern const UBaseType_t STATUS_LED_TASK_PRIO;
extern const uint32_t NETWORK_TASK_STACK;
extern const uint32_t DMX_TASK_STACK;
extern const uint32_t STATUS_LED_TASK_STACK;

void validateGpioMap();
