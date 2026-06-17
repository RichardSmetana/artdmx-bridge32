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
#include "device_config.h"
#include "config.h"
#include "device_log.h"
#include "secrets.h"

Preferences prefs;
DeviceConfig cfg;

static const char *const CONFIG_REQUIRED_KEYS[] = {
  "wifi_ssid",
  "wifi_pass",
  "hostname",
  "web_password",
  "ota_password",
  "artnet_universe",
  "artnet_timeout_ms",
  "dmx_refresh_ms",
  "send_full_packet",
  "enable_dmx_input",
  "enable_dmx2_output",
  "dmx2_filter_start",
  "dmx2_filter_end",
  "artnet_debug_mode",
  "artnet_debug_every",
  "artnet_debug_ch_start",
  "artnet_debug_ch_end",
  "artnet_debug_on_change",
};
static const size_t CONFIG_REQUIRED_KEY_COUNT =
  sizeof(CONFIG_REQUIRED_KEYS) / sizeof(CONFIG_REQUIRED_KEYS[0]);

struct ConfigKeyFlags {
  bool wifiSsid;
  bool wifiPass;
  bool hostname;
  bool webPassword;
  bool otaPassword;
  bool artnetUniverse;
  bool artnetTimeoutMs;
  bool dmxRefreshMs;
  bool sendFullPacket;
  bool enableDmxInput;
  bool enableDmx2Output;
  bool dmx2FilterStart;
  bool dmx2FilterEnd;
  bool artnetDebugMode;
  bool artnetDebugEvery;
  bool artnetDebugChStart;
  bool artnetDebugChEnd;
  bool artnetDebugOnChange;
};

static void copyStringField(char *dest, size_t destSize, const String &src) {
  src.toCharArray(dest, destSize);
}

static void applyConfigDefaults() {
  copyStringField(cfg.wifiSsid, sizeof(cfg.wifiSsid), SECRETS_WIFI_SSID);
  copyStringField(cfg.wifiPass, sizeof(cfg.wifiPass), SECRETS_WIFI_PASS);
  copyStringField(cfg.hostname, sizeof(cfg.hostname), DEFAULT_HOSTNAME);
  copyStringField(cfg.otaPassword, sizeof(cfg.otaPassword), SECRETS_OTA_PASSWORD);
  copyStringField(cfg.webPassword, sizeof(cfg.webPassword), DEFAULT_WEB_PASSWORD);
  cfg.artnetUniverse = DEFAULT_ARTNET_UNIVERSE;
  cfg.artnetTimeoutMs = DEFAULT_ARTNET_TIMEOUT_MS;
  cfg.dmxRefreshMs = DEFAULT_DMX_REFRESH_MS;
  cfg.sendFullPacket = DEFAULT_SEND_FULL_PACKET != 0;
  cfg.enableDmxInput = DEFAULT_ENABLE_DMX_INPUT != 0;
  cfg.enableDmx2Output = DEFAULT_ENABLE_DMX2_OUTPUT != 0;
  cfg.dmx2FilterStart = DEFAULT_DMX2_FILTER_START;
  cfg.dmx2FilterEnd = DEFAULT_DMX2_FILTER_END;
  cfg.artnetDebugMode = DEFAULT_ARTNET_DEBUG_MODE;
  cfg.artnetDebugEvery = DEFAULT_ARTNET_DEBUG_EVERY;
  cfg.artnetDebugChStart = DEFAULT_ARTNET_DEBUG_CH_START;
  cfg.artnetDebugChEnd = DEFAULT_ARTNET_DEBUG_CH_END;
  cfg.artnetDebugOnChange = DEFAULT_ARTNET_DEBUG_ON_CHANGE != 0;
}

static void normalizeArtnetDebugChannels() {
  if (cfg.artnetDebugChStart < 1) {
    cfg.artnetDebugChStart = 1;
  }
  if (cfg.artnetDebugChEnd > 512) {
    cfg.artnetDebugChEnd = 512;
  }
  if (cfg.artnetDebugChStart > cfg.artnetDebugChEnd) {
    uint16_t tmp = cfg.artnetDebugChStart;
    cfg.artnetDebugChStart = cfg.artnetDebugChEnd;
    cfg.artnetDebugChEnd = tmp;
  }
}

static void normalizeDmx2Filter() {
  if (cfg.dmx2FilterStart < 1) {
    cfg.dmx2FilterStart = 1;
  }
  if (cfg.dmx2FilterEnd > 512) {
    cfg.dmx2FilterEnd = 512;
  }
  if (cfg.dmx2FilterStart > cfg.dmx2FilterEnd) {
    uint16_t tmp = cfg.dmx2FilterStart;
    cfg.dmx2FilterStart = cfg.dmx2FilterEnd;
    cfg.dmx2FilterEnd = tmp;
  }
}

bool copyArgField(char *dest, size_t destSize, const char *value, const char *fieldName) {
  if (value == nullptr) {
    return true;
  }
  if (strlen(value) >= destSize) {
    appendLogf("Config rejected: %s too long", fieldName);
    return false;
  }
  strncpy(dest, value, destSize - 1);
  dest[destSize - 1] = '\0';
  return true;
}

String jsonEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '\\' || c == '"') {
      out += '\\';
    }
    out += c;
  }
  return out;
}

void saveConfigToNvs() {
  normalizeDmx2Filter();
  normalizeArtnetDebugChannels();

  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("wifi_ssid", cfg.wifiSsid);
  prefs.putString("wifi_pass", cfg.wifiPass);
  prefs.putString("hostname", cfg.hostname);
  prefs.putString("ota_pass", cfg.otaPassword);
  prefs.putString("web_pass", cfg.webPassword);
  prefs.putUShort("artnet_uni", cfg.artnetUniverse);
  prefs.putUInt("artnet_to", cfg.artnetTimeoutMs);
  prefs.putUInt("dmx_refresh", cfg.dmxRefreshMs);
  prefs.putBool("full_pkt", cfg.sendFullPacket);
  prefs.putBool("dmx_in", cfg.enableDmxInput);
  prefs.putBool("dmx2_out", cfg.enableDmx2Output);
  prefs.putUShort("dmx2_fstart", cfg.dmx2FilterStart);
  prefs.putUShort("dmx2_fend", cfg.dmx2FilterEnd);
  prefs.putUChar("artnet_dbg_mode", cfg.artnetDebugMode);
  prefs.putUShort("dbg_every", cfg.artnetDebugEvery);
  prefs.putUShort("dbg_ch_start", cfg.artnetDebugChStart);
  prefs.putUShort("dbg_ch_end", cfg.artnetDebugChEnd);
  prefs.putBool("dbg_on_chg", cfg.artnetDebugOnChange);
  prefs.end();
}

void loadConfigFromNvs() {
  applyConfigDefaults();

  prefs.begin(NVS_NAMESPACE, false);

  copyStringField(cfg.wifiSsid, sizeof(cfg.wifiSsid),
                  prefs.getString("wifi_ssid", cfg.wifiSsid));
  copyStringField(cfg.wifiPass, sizeof(cfg.wifiPass),
                  prefs.getString("wifi_pass", cfg.wifiPass));

  String host = prefs.getString("hostname", "");
  if (host.length() == 0) {
    host = prefs.getString("ota_host", cfg.hostname);
  }
  copyStringField(cfg.hostname, sizeof(cfg.hostname), host);

  copyStringField(cfg.otaPassword, sizeof(cfg.otaPassword),
                  prefs.getString("ota_pass", cfg.otaPassword));
  copyStringField(cfg.webPassword, sizeof(cfg.webPassword),
                  prefs.getString("web_pass", cfg.webPassword));

  cfg.artnetUniverse = prefs.getUShort("artnet_uni", cfg.artnetUniverse);
  cfg.artnetTimeoutMs = prefs.getUInt("artnet_to", cfg.artnetTimeoutMs);
  cfg.dmxRefreshMs = prefs.getUInt("dmx_refresh", cfg.dmxRefreshMs);
  cfg.sendFullPacket = prefs.getBool("full_pkt", cfg.sendFullPacket);
  cfg.enableDmxInput = prefs.getBool("dmx_in", cfg.enableDmxInput);
  cfg.enableDmx2Output = prefs.getBool("dmx2_out", cfg.enableDmx2Output);
  cfg.dmx2FilterStart = prefs.getUShort("dmx2_fstart", cfg.dmx2FilterStart);
  cfg.dmx2FilterEnd = prefs.getUShort("dmx2_fend", cfg.dmx2FilterEnd);

  if (prefs.isKey("artnet_dbg_mode")) {
    cfg.artnetDebugMode = prefs.getUChar("artnet_dbg_mode", cfg.artnetDebugMode);
  } else {
    cfg.artnetDebugMode = prefs.getBool("dbg_artnet", DEFAULT_DEBUG_ARTNET != 0) ? 1 : 0;
  }
  cfg.artnetDebugEvery = prefs.getUShort("dbg_every",
                                         prefs.getUShort("artnet_dbg_every",
                                                         cfg.artnetDebugEvery));
  if (cfg.artnetDebugEvery < 1) {
    cfg.artnetDebugEvery = 1;
  }
  if (prefs.isKey("dbg_ch_start")) {
    cfg.artnetDebugChStart = prefs.getUShort("dbg_ch_start", cfg.artnetDebugChStart);
    cfg.artnetDebugChEnd = prefs.getUShort("dbg_ch_end", cfg.artnetDebugChEnd);
  } else if (prefs.isKey("artnet_dbg_end")) {
    // Legacy keys: artnet_dbg_start was 16 chars and could not be stored in NVS.
    cfg.artnetDebugChStart = prefs.getUShort("artnet_dbg_start", cfg.artnetDebugChStart);
    cfg.artnetDebugChEnd = prefs.getUShort("artnet_dbg_end", cfg.artnetDebugChEnd);
  } else {
    cfg.artnetDebugChStart = cfg.dmx2FilterStart;
    cfg.artnetDebugChEnd = cfg.dmx2FilterEnd;
  }
  cfg.artnetDebugOnChange = prefs.getBool("dbg_on_chg", cfg.artnetDebugOnChange);

  prefs.end();
  normalizeDmx2Filter();
  normalizeArtnetDebugChannels();

  appendLogf("Config loaded: WiFi='%s', hostname='%s'", cfg.wifiSsid, cfg.hostname);
  appendLogf("ArtNet uni=%u, DMX refresh=%lu ms, DMX1 input=%s, DMX2 output=%s",
             cfg.artnetUniverse,
             (unsigned long)cfg.dmxRefreshMs,
             cfg.enableDmxInput ? "on" : "off",
             (!cfg.enableDmxInput && cfg.enableDmx2Output) ? "on" : "off");
}

void resetConfigToDefaults() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  loadConfigFromNvs();
  saveConfigToNvs();
  appendLog("Config reset to factory defaults");
}

void factoryResetAndRestart() {
  appendLog("Factory reset: clearing all settings");
  prefs.begin(NVS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  delay(250);
  ESP.restart();
}

void checkResetPin() {
  static uint32_t holdStartMs = 0;

  if (digitalRead(RESET_PIN) == LOW) {
    if (holdStartMs == 0) {
      holdStartMs = millis();
    } else if (millis() - holdStartMs >= RESET_HOLD_MS) {
      appendLog("Factory reset: GPIO held, clearing config");
      factoryResetAndRestart();
    }
  } else {
    holdStartMs = 0;
  }
}

String buildConfigJson() {
  String json = "{";
  json += "\"wifi_ssid\":\"" + jsonEscape(String(cfg.wifiSsid)) + "\",";
  json += "\"wifi_pass\":\"" + jsonEscape(String(cfg.wifiPass)) + "\",";
  json += "\"hostname\":\"" + jsonEscape(String(cfg.hostname)) + "\",";
  json += "\"ota_hostname\":\"" + jsonEscape(String(cfg.hostname)) + "\",";
  json += "\"ota_password\":\"" + jsonEscape(String(cfg.otaPassword)) + "\",";
  json += "\"web_password\":\"" + jsonEscape(String(cfg.webPassword)) + "\",";
  json += "\"artnet_universe\":" + String(cfg.artnetUniverse) + ",";
  json += "\"artnet_timeout_ms\":" + String(cfg.artnetTimeoutMs) + ",";
  json += "\"dmx_refresh_ms\":" + String(cfg.dmxRefreshMs) + ",";
  json += "\"send_full_packet\":" + String(cfg.sendFullPacket ? "true" : "false") + ",";
  json += "\"enable_dmx_input\":" + String(cfg.enableDmxInput ? "true" : "false") + ",";
  json += "\"enable_dmx2_output\":" + String(cfg.enableDmx2Output ? "true" : "false") + ",";
  json += "\"dmx2_filter_start\":" + String(cfg.dmx2FilterStart) + ",";
  json += "\"dmx2_filter_end\":" + String(cfg.dmx2FilterEnd) + ",";
  json += "\"artnet_debug_mode\":" + String(cfg.artnetDebugMode) + ",";
  json += "\"artnet_debug_every\":" + String(cfg.artnetDebugEvery) + ",";
  json += "\"artnet_debug_ch_start\":" + String(cfg.artnetDebugChStart) + ",";
  json += "\"artnet_debug_ch_end\":" + String(cfg.artnetDebugChEnd) + ",";
  json += "\"artnet_debug_on_change\":" + String(cfg.artnetDebugOnChange ? "true" : "false");
  json += "}";
  return json;
}

static String configTextQuote(const char *value) {
  String s(value);
  bool needQuote = false;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == ' ' || c == '\t' || c == '#' || c == '=' || c == '"' || c == '\r' || c == '\n') {
      needQuote = true;
      break;
    }
  }
  if (!needQuote) {
    return s;
  }

  String out = "\"";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"' || c == '\\') {
      out += '\\';
    }
    out += c;
  }
  out += "\"";
  return out;
}

static void appendConfigLine(String &out, const char *key, const char *value) {
  out += key;
  out += "=";
  out += configTextQuote(value);
  out += "\n";
}

static void appendConfigLine(String &out, const char *key, bool value) {
  out += key;
  out += "=";
  out += value ? "true" : "false";
  out += "\n";
}

static void appendConfigLine(String &out, const char *key, uint32_t value) {
  out += key;
  out += "=";
  out += String(value);
  out += "\n";
}

String exportConfigText() {
  String out;
  out.reserve(768);
  out += "# artdmx-bridge32 configuration\n";
  out += "# SPDX-License-Identifier: GPL-3.0-or-later\n";
  out += "# Copyright (C) 2026 Richard Smetana\n";
  out += "# License: GNU GPL v3 or later — full text in LICENSE file\n";
  out += "# Editable text format. Lines starting with # are comments.\n";
  out += "# Syntax: key=value  |  booleans: true/false  |  quote values with spaces\n";
  out += "# Required on upload: all setting keys below (wifi_*, hostname, etc.).\n";
  out += "# Optional metadata: format_version, device, firmware.\n\n";
  out += "format_version=1\n";
  out += "device=";
  out += DEVICE_NAME;
  out += "\n";
  out += "firmware=";
  out += VERSION;
  out += "\n\n";

  appendConfigLine(out, "wifi_ssid", cfg.wifiSsid);
  appendConfigLine(out, "wifi_pass", cfg.wifiPass);
  appendConfigLine(out, "hostname", cfg.hostname);
  appendConfigLine(out, "web_password", cfg.webPassword);
  appendConfigLine(out, "ota_password", cfg.otaPassword);
  appendConfigLine(out, "artnet_universe", (uint32_t)cfg.artnetUniverse);
  appendConfigLine(out, "artnet_timeout_ms", cfg.artnetTimeoutMs);
  appendConfigLine(out, "dmx_refresh_ms", cfg.dmxRefreshMs);
  appendConfigLine(out, "send_full_packet", cfg.sendFullPacket);
  appendConfigLine(out, "enable_dmx_input", cfg.enableDmxInput);
  appendConfigLine(out, "enable_dmx2_output", cfg.enableDmx2Output);
  appendConfigLine(out, "dmx2_filter_start", (uint32_t)cfg.dmx2FilterStart);
  appendConfigLine(out, "dmx2_filter_end", (uint32_t)cfg.dmx2FilterEnd);
  appendConfigLine(out, "artnet_debug_mode", (uint32_t)cfg.artnetDebugMode);
  appendConfigLine(out, "artnet_debug_every", (uint32_t)cfg.artnetDebugEvery);
  appendConfigLine(out, "artnet_debug_ch_start", (uint32_t)cfg.artnetDebugChStart);
  appendConfigLine(out, "artnet_debug_ch_end", (uint32_t)cfg.artnetDebugChEnd);
  appendConfigLine(out, "artnet_debug_on_change", cfg.artnetDebugOnChange);
  return out;
}

static bool parseConfigBool(const String &raw, bool &out) {
  String s = raw;
  s.trim();
  s.toLowerCase();
  if (s == "true" || s == "1" || s == "yes" || s == "on") {
    out = true;
    return true;
  }
  if (s == "false" || s == "0" || s == "no" || s == "off") {
    out = false;
    return true;
  }
  return false;
}

static bool parseConfigValue(const String &raw, String &out) {
  String s = raw;
  s.trim();
  if (s.length() >= 2 && s.charAt(0) == '"' && s.charAt(s.length() - 1) == '"') {
    String unquoted;
    unquoted.reserve(s.length());
    for (size_t i = 1; i < s.length() - 1; i++) {
      char c = s.charAt(i);
      if (c == '\\' && i + 1 < s.length() - 1) {
        char next = s.charAt(i + 1);
        if (next == '"' || next == '\\') {
          unquoted += next;
          i++;
          continue;
        }
      }
      unquoted += c;
    }
    out = unquoted;
    return true;
  }
  out = s;
  return true;
}

static bool parseConfigUint(const String &raw, uint32_t &out) {
  String s = raw;
  s.trim();
  if (s.length() == 0) {
    return false;
  }
  for (size_t i = 0; i < s.length(); i++) {
    if (!isDigit(s.charAt(i))) {
      return false;
    }
  }
  out = (uint32_t)s.toInt();
  return true;
}

static bool configStringFits(const char *value, size_t maxLen, String &error, const char *key) {
  if (value == nullptr) {
    return true;
  }
  if (strlen(value) > maxLen) {
    error = String(key) + " too long (max " + String(maxLen) + " chars)";
    return false;
  }
  return true;
}

static bool validateImportedConfig(const DeviceConfig &parsed, String &error) {
  if (strlen(parsed.wifiSsid) == 0) {
    error = "wifi_ssid must not be empty";
    return false;
  }
  if (strlen(parsed.hostname) == 0) {
    error = "hostname must not be empty";
    return false;
  }
  if (!configStringFits(parsed.wifiSsid, WIFI_SSID_MAX, error, "wifi_ssid") ||
      !configStringFits(parsed.wifiPass, WIFI_PASS_MAX, error, "wifi_pass") ||
      !configStringFits(parsed.hostname, HOSTNAME_MAX, error, "hostname") ||
      !configStringFits(parsed.webPassword, WEB_PASS_MAX, error, "web_password") ||
      !configStringFits(parsed.otaPassword, OTA_PASS_MAX, error, "ota_password")) {
    return false;
  }
  if (parsed.artnetUniverse > 32767) {
    error = "artnet_universe out of range (0-32767)";
    return false;
  }
  if (parsed.artnetTimeoutMs < 500 || parsed.artnetTimeoutMs > 60000) {
    error = "artnet_timeout_ms out of range (500-60000)";
    return false;
  }
  if (parsed.dmxRefreshMs < 10 || parsed.dmxRefreshMs > 1000) {
    error = "dmx_refresh_ms out of range (10-1000)";
    return false;
  }
  if (parsed.artnetDebugMode > 3) {
    error = "artnet_debug_mode out of range (0-3)";
    return false;
  }
  if (parsed.artnetDebugEvery < 1 || parsed.artnetDebugEvery > 1000) {
    error = "artnet_debug_every out of range (1-1000)";
    return false;
  }
  if (parsed.dmx2FilterStart < 1 || parsed.dmx2FilterStart > 512) {
    error = "dmx2_filter_start out of range (1-512)";
    return false;
  }
  if (parsed.dmx2FilterEnd < 1 || parsed.dmx2FilterEnd > 512) {
    error = "dmx2_filter_end out of range (1-512)";
    return false;
  }
  if (parsed.artnetDebugChStart < 1 || parsed.artnetDebugChStart > 512) {
    error = "artnet_debug_ch_start out of range (1-512)";
    return false;
  }
  if (parsed.artnetDebugChEnd < 1 || parsed.artnetDebugChEnd > 512) {
    error = "artnet_debug_ch_end out of range (1-512)";
    return false;
  }
  if (parsed.enableDmxInput && parsed.enableDmx2Output) {
    error = "enable_dmx_input and enable_dmx2_output cannot both be true";
    return false;
  }
  return true;
}

static bool markConfigKey(ConfigKeyFlags &flags, const char *key, String &error) {
  auto rejectDuplicate = [&](bool &seen) -> bool {
    if (seen) {
      error = String("Duplicate key: ") + key;
      return false;
    }
    seen = true;
    return true;
  };

  if (strcmp(key, "wifi_ssid") == 0) {
    return rejectDuplicate(flags.wifiSsid);
  }
  if (strcmp(key, "wifi_pass") == 0) {
    return rejectDuplicate(flags.wifiPass);
  }
  if (strcmp(key, "hostname") == 0) {
    return rejectDuplicate(flags.hostname);
  }
  if (strcmp(key, "web_password") == 0) {
    return rejectDuplicate(flags.webPassword);
  }
  if (strcmp(key, "ota_password") == 0) {
    return rejectDuplicate(flags.otaPassword);
  }
  if (strcmp(key, "artnet_universe") == 0) {
    return rejectDuplicate(flags.artnetUniverse);
  }
  if (strcmp(key, "artnet_timeout_ms") == 0) {
    return rejectDuplicate(flags.artnetTimeoutMs);
  }
  if (strcmp(key, "dmx_refresh_ms") == 0) {
    return rejectDuplicate(flags.dmxRefreshMs);
  }
  if (strcmp(key, "send_full_packet") == 0) {
    return rejectDuplicate(flags.sendFullPacket);
  }
  if (strcmp(key, "enable_dmx_input") == 0) {
    return rejectDuplicate(flags.enableDmxInput);
  }
  if (strcmp(key, "enable_dmx2_output") == 0) {
    return rejectDuplicate(flags.enableDmx2Output);
  }
  if (strcmp(key, "dmx2_filter_start") == 0) {
    return rejectDuplicate(flags.dmx2FilterStart);
  }
  if (strcmp(key, "dmx2_filter_end") == 0) {
    return rejectDuplicate(flags.dmx2FilterEnd);
  }
  if (strcmp(key, "artnet_debug_mode") == 0) {
    return rejectDuplicate(flags.artnetDebugMode);
  }
  if (strcmp(key, "artnet_debug_every") == 0) {
    return rejectDuplicate(flags.artnetDebugEvery);
  }
  if (strcmp(key, "artnet_debug_ch_start") == 0) {
    return rejectDuplicate(flags.artnetDebugChStart);
  }
  if (strcmp(key, "artnet_debug_ch_end") == 0) {
    return rejectDuplicate(flags.artnetDebugChEnd);
  }
  if (strcmp(key, "artnet_debug_on_change") == 0) {
    return rejectDuplicate(flags.artnetDebugOnChange);
  }
  return true;
}

static bool assignConfigKey(DeviceConfig &parsed,
                            const char *key,
                            const String &value,
                            String &error,
                            uint16_t lineNo) {
  String parsedValue;
  if (!parseConfigValue(value, parsedValue)) {
    error = "Line " + String(lineNo) + ": invalid quoted value for " + String(key);
    return false;
  }

  if (strcmp(key, "format_version") == 0) {
    uint32_t version = 0;
    if (!parseConfigUint(parsedValue, version) || version != 1) {
      error = "Unsupported format_version (expected 1)";
      return false;
    }
    return true;
  }
  if (strcmp(key, "device") == 0 || strcmp(key, "firmware") == 0) {
    return true;
  }

  if (strcmp(key, "wifi_ssid") == 0) {
    if (!copyArgField(parsed.wifiSsid, sizeof(parsed.wifiSsid), parsedValue.c_str(), key)) {
      error = String(key) + " too long";
      return false;
    }
    return true;
  }
  if (strcmp(key, "wifi_pass") == 0) {
    if (!copyArgField(parsed.wifiPass, sizeof(parsed.wifiPass), parsedValue.c_str(), key)) {
      error = String(key) + " too long";
      return false;
    }
    return true;
  }
  if (strcmp(key, "hostname") == 0) {
    if (!copyArgField(parsed.hostname, sizeof(parsed.hostname), parsedValue.c_str(), key)) {
      error = String(key) + " too long";
      return false;
    }
    return true;
  }
  if (strcmp(key, "web_password") == 0) {
    if (!copyArgField(parsed.webPassword, sizeof(parsed.webPassword), parsedValue.c_str(), key)) {
      error = String(key) + " too long";
      return false;
    }
    return true;
  }
  if (strcmp(key, "ota_password") == 0) {
    if (!copyArgField(parsed.otaPassword, sizeof(parsed.otaPassword), parsedValue.c_str(), key)) {
      error = String(key) + " too long";
      return false;
    }
    return true;
  }

  uint32_t number = 0;
  bool boolean = false;

  if (strcmp(key, "artnet_universe") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_universe";
      return false;
    }
    parsed.artnetUniverse = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "artnet_timeout_ms") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_timeout_ms";
      return false;
    }
    parsed.artnetTimeoutMs = number;
    return true;
  }
  if (strcmp(key, "dmx_refresh_ms") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for dmx_refresh_ms";
      return false;
    }
    parsed.dmxRefreshMs = number;
    return true;
  }
  if (strcmp(key, "dmx2_filter_start") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for dmx2_filter_start";
      return false;
    }
    parsed.dmx2FilterStart = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "dmx2_filter_end") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for dmx2_filter_end";
      return false;
    }
    parsed.dmx2FilterEnd = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "artnet_debug_mode") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_debug_mode";
      return false;
    }
    parsed.artnetDebugMode = (uint8_t)number;
    return true;
  }
  if (strcmp(key, "artnet_debug_every") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_debug_every";
      return false;
    }
    parsed.artnetDebugEvery = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "artnet_debug_ch_start") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_debug_ch_start";
      return false;
    }
    parsed.artnetDebugChStart = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "artnet_debug_ch_end") == 0) {
    if (!parseConfigUint(parsedValue, number)) {
      error = "Invalid integer for artnet_debug_ch_end";
      return false;
    }
    parsed.artnetDebugChEnd = (uint16_t)number;
    return true;
  }
  if (strcmp(key, "send_full_packet") == 0) {
    if (!parseConfigBool(parsedValue, boolean)) {
      error = "Invalid boolean for send_full_packet";
      return false;
    }
    parsed.sendFullPacket = boolean;
    return true;
  }
  if (strcmp(key, "enable_dmx_input") == 0) {
    if (!parseConfigBool(parsedValue, boolean)) {
      error = "Invalid boolean for enable_dmx_input";
      return false;
    }
    parsed.enableDmxInput = boolean;
    return true;
  }
  if (strcmp(key, "enable_dmx2_output") == 0) {
    if (!parseConfigBool(parsedValue, boolean)) {
      error = "Invalid boolean for enable_dmx2_output";
      return false;
    }
    parsed.enableDmx2Output = boolean;
    return true;
  }
  if (strcmp(key, "artnet_debug_on_change") == 0) {
    if (!parseConfigBool(parsedValue, boolean)) {
      error = "Invalid boolean for artnet_debug_on_change";
      return false;
    }
    parsed.artnetDebugOnChange = boolean;
    return true;
  }

  error = "Line " + String(lineNo) + ": unknown key '" + String(key) + "'";
  return false;
}

static bool configKeysComplete(const ConfigKeyFlags &flags, String &error) {
  for (size_t i = 0; i < CONFIG_REQUIRED_KEY_COUNT; i++) {
    const char *key = CONFIG_REQUIRED_KEYS[i];
    bool present = false;

    if (strcmp(key, "wifi_ssid") == 0) {
      present = flags.wifiSsid;
    } else if (strcmp(key, "wifi_pass") == 0) {
      present = flags.wifiPass;
    } else if (strcmp(key, "hostname") == 0) {
      present = flags.hostname;
    } else if (strcmp(key, "web_password") == 0) {
      present = flags.webPassword;
    } else if (strcmp(key, "ota_password") == 0) {
      present = flags.otaPassword;
    } else if (strcmp(key, "artnet_universe") == 0) {
      present = flags.artnetUniverse;
    } else if (strcmp(key, "artnet_timeout_ms") == 0) {
      present = flags.artnetTimeoutMs;
    } else if (strcmp(key, "dmx_refresh_ms") == 0) {
      present = flags.dmxRefreshMs;
    } else if (strcmp(key, "send_full_packet") == 0) {
      present = flags.sendFullPacket;
    } else if (strcmp(key, "enable_dmx_input") == 0) {
      present = flags.enableDmxInput;
    } else if (strcmp(key, "enable_dmx2_output") == 0) {
      present = flags.enableDmx2Output;
    } else if (strcmp(key, "dmx2_filter_start") == 0) {
      present = flags.dmx2FilterStart;
    } else if (strcmp(key, "dmx2_filter_end") == 0) {
      present = flags.dmx2FilterEnd;
    } else if (strcmp(key, "artnet_debug_mode") == 0) {
      present = flags.artnetDebugMode;
    } else if (strcmp(key, "artnet_debug_every") == 0) {
      present = flags.artnetDebugEvery;
    } else if (strcmp(key, "artnet_debug_ch_start") == 0) {
      present = flags.artnetDebugChStart;
    } else if (strcmp(key, "artnet_debug_ch_end") == 0) {
      present = flags.artnetDebugChEnd;
    } else if (strcmp(key, "artnet_debug_on_change") == 0) {
      present = flags.artnetDebugOnChange;
    }

    if (!present) {
      error = String("Missing required key: ") + key;
      return false;
    }
  }
  return true;
}

bool importConfigText(const String &text, String &error) {
  DeviceConfig parsed;
  memset(&parsed, 0, sizeof(parsed));
  ConfigKeyFlags flags;
  memset(&flags, 0, sizeof(flags));

  uint16_t lineNo = 0;
  int lineStart = 0;

  while (lineStart <= (int)text.length()) {
    lineNo++;
    int lineEnd = text.indexOf('\n', lineStart);
    if (lineEnd < 0) {
      lineEnd = text.length();
    }

    String line = text.substring(lineStart, lineEnd);
    if (line.endsWith("\r")) {
      line.remove(line.length() - 1);
    }
    line.trim();

    if (line.length() > 0 && line.charAt(0) != '#') {
      int eq = line.indexOf('=');
      if (eq <= 0) {
        error = "Line " + String(lineNo) + ": expected key=value";
        return false;
      }

      String key = line.substring(0, eq);
      String value = line.substring(eq + 1);
      key.trim();
      value.trim();

      if (key.length() == 0) {
        error = "Line " + String(lineNo) + ": empty key";
        return false;
      }

      if (strcmp(key.c_str(), "format_version") != 0 &&
          strcmp(key.c_str(), "device") != 0 &&
          strcmp(key.c_str(), "firmware") != 0) {
        if (!markConfigKey(flags, key.c_str(), error)) {
          return false;
        }
      }

      if (!assignConfigKey(parsed, key.c_str(), value, error, lineNo)) {
        return false;
      }
    }

    if (lineEnd >= (int)text.length()) {
      break;
    }
    lineStart = lineEnd + 1;
  }

  if (!configKeysComplete(flags, error)) {
    return false;
  }

  if (!validateImportedConfig(parsed, error)) {
    return false;
  }

  cfg = parsed;
  if (cfg.enableDmxInput) {
    cfg.enableDmx2Output = false;
  }
  normalizeDmx2Filter();
  normalizeArtnetDebugChannels();
  saveConfigToNvs();
  appendLogf("Config imported from file: WiFi='%s', hostname='%s'", cfg.wifiSsid, cfg.hostname);
  return true;
}
