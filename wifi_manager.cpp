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
#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "config.h"
#include "device_config.h"
#include "device_log.h"
#include "led.h"
#include "ota_manager.h"

static unsigned long lastWiFiAttemptMs = 0;
static unsigned long lastDisconnectMs = 0;
static unsigned long wifiConnectStartedMs = 0;
static unsigned long lastWiFiHealthCheckMs = 0;
static bool wifiHealthCache = false;
static bool mdnsReady = false;
static bool apModeActive = false;

bool isApModeActive() {
  return apModeActive;
}

bool isWifiConnecting() {
  if (isOtaUpdateActive()) {
    return false;
  }
  if (isApModeActive()) {
    return false;
  }
  return !isWifiConnected();
}

static bool wifiHasValidIp() {
  return WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

static bool wifiLinkAlive() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  if (!wifiHasValidIp()) {
    return false;
  }

  wifi_ap_record_t ap;
  return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}

bool isWifiConnected() {
  unsigned long now = millis();

  if (lastDisconnectMs > 0 ||
      (now - lastWiFiHealthCheckMs) >= WIFI_HEALTH_CHECK_MS) {
    lastWiFiHealthCheckMs = now;
    wifiHealthCache = wifiLinkAlive();
  }

  return wifiHealthCache;
}

static unsigned long wifiDownSinceMs() {
  unsigned long now = millis();
  if (lastDisconnectMs > 0) {
    return now - lastDisconnectMs;
  }
  if (wifiConnectStartedMs > 0) {
    return now - wifiConnectStartedMs;
  }
  return 0;
}

bool isOtaReady() {
  return otaIsReady();
}

String apSsid() {
  return String(cfg.hostname) + "-setup";
}

static void applyWifiHostname() {
  if (cfg.hostname[0] == '\0') {
    return;
  }

  // Arduino API: must run before WiFi.mode()/begin(); STA_START also sets netif below.
  WiFi.setHostname(cfg.hostname);

  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif != nullptr) {
    esp_err_t err = esp_netif_set_hostname(netif, cfg.hostname);
    if (err != ESP_OK) {
      appendLogf("WARN: esp_netif_set_hostname failed (%d) for '%s'",
                 static_cast<int>(err), cfg.hostname);
    }
  }

  const char *active = WiFi.getHostname();
  if (active != nullptr && strcmp(active, cfg.hostname) == 0) {
    appendLogf("DHCP client hostname: %s", active);
  } else {
    appendLogf("DHCP client hostname: %s (configured; netif pending)",
               cfg.hostname);
  }
}

static void disableWiFiPowerManagement() {
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
}

static void setupMdns() {
  if (mdnsReady) {
    return;
  }
  if (MDNS.begin(cfg.hostname)) {
    MDNS.addService("http", "tcp", 80);
    mdnsReady = true;
    appendLogf("mDNS: http://%s.local", cfg.hostname);
  } else {
    appendLog("mDNS start failed");
  }
}

static void stopConfigAp() {
  if (!apModeActive) {
    return;
  }
  WiFi.softAPdisconnect(true);
  apModeActive = false;
  appendLog("Config access point stopped");
}

static void startConfigAp() {
  if (apModeActive) {
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  String ssid = apSsid();
  if (!WiFi.softAP(ssid.c_str())) {
    appendLog("Failed to start config access point");
    return;
  }

  apModeActive = true;
  appendLogf("Config AP '%s' at http://%s (no WiFi — configure here)",
             ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

static void onWiFiConnected() {
  disableWiFiPowerManagement();
  applyWifiHostname();
  lastDisconnectMs = 0;
  wifiConnectStartedMs = 0;
  wifiHealthCache = true;
  lastWiFiHealthCheckMs = millis();
  appendLogf("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
  const char *dhcpHost = WiFi.getHostname();
  if (dhcpHost != nullptr && dhcpHost[0] != '\0') {
    appendLogf("DHCP hostname active: %s", dhcpHost);
  }
  appendLogf("Open http://%s.local", cfg.hostname);
  appendLogf("RSSI: %d dBm", WiFi.RSSI());

  stopConfigAp();
  setupMdns();

  if (!otaIsReady()) {
    setupOTA();
  }
}

static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      applyWifiHostname();
      appendLog("WiFi STA started");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      appendLog("WiFi associated with AP");
      disableWiFiPowerManagement();
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      onWiFiConnected();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      otaResetReady();
      mdnsReady = false;
      lastWiFiAttemptMs = 0;
      wifiHealthCache = false;
      lastWiFiHealthCheckMs = 0;
      if (lastDisconnectMs == 0) {
        appendLog("WiFi disconnected");
        lastDisconnectMs = millis();
        wifiConnectStartedMs = lastDisconnectMs;
      }
      break;

    default:
      break;
  }
}

static void markWifiLost(const char *reason) {
  unsigned long now = millis();

  otaResetReady();
  mdnsReady = false;
  lastWiFiAttemptMs = 0;
  wifiHealthCache = false;
  lastWiFiHealthCheckMs = 0;

  if (lastDisconnectMs == 0) {
    appendLog(reason);
    lastDisconnectMs = now;
    wifiConnectStartedMs = now;
  }
}

void connectWiFi() {
  if (isOtaUpdateActive()) {
    return;
  }

  if (isWifiConnected()) {
    lastDisconnectMs = 0;
    wifiConnectStartedMs = 0;
    return;
  }

  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    markWifiLost("WiFi link lost (no IP or AP info)");
    WiFi.disconnect(false);
  } else if (lastDisconnectMs == 0) {
    markWifiLost("WiFi not connected, reconnecting...");
  }

  if (!apModeActive && wifiDownSinceMs() >= WIFI_CONNECT_TIMEOUT_MS) {
    startConfigAp();
  }

  if (now - lastWiFiAttemptMs < WIFI_RECONNECT_MS) {
    return;
  }
  lastWiFiAttemptMs = now;

  applyWifiHostname();
  appendLogf("WiFi reconnect attempt (status=%d)", WiFi.status());
  WiFi.begin(cfg.wifiSsid, cfg.wifiPass);
}

void initWifi() {
  wifiConnectStartedMs = millis();
  lastDisconnectMs = 0;

  applyWifiHostname();
  WiFi.begin(cfg.wifiSsid, cfg.wifiPass);
  appendLogf("Connecting to WiFi '%s'...", cfg.wifiSsid);
  lastWiFiAttemptMs = millis();

  unsigned long connectStart = millis();
  while (!isWifiConnected() &&
         millis() - connectStart < WIFI_CONNECT_TIMEOUT_MS) {
    updateStatusLed();
    delay(50);
  }

  if (!isWifiConnected()) {
    lastDisconnectMs = connectStart;
    wifiConnectStartedMs = connectStart;
    startConfigAp();
  } else {
    lastDisconnectMs = 0;
    wifiConnectStartedMs = 0;
    wifiHealthCache = true;
    lastWiFiHealthCheckMs = millis();
    // GOT_IP event usually calls onWiFiConnected(); ensure OTA/mDNS if event raced ahead.
    if (!otaIsReady()) {
      onWiFiConnected();
    }
  }
}

void prepareWifi() {
  WiFi.onEvent(onWiFiEvent);

  // ESP32 Arduino assigns esp32-<MAC> if mode/begin run before setHostname.
  WiFi.mode(WIFI_MODE_NULL);
  applyWifiHostname();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  disableWiFiPowerManagement();
}
