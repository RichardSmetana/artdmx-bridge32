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
#include "web_server.h"
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "device_config.h"
#include "device_log.h"
#include "globals.h"
#include "wifi_manager.h"
#include "led.h"
#include "dmx_test.h"
#include "logo_data.h"
#include "web_config_i18n.h"

WebServer server(80);

static const char PAGE_STYLE[] = R"rawliteral(
    * { box-sizing: border-box; }
    body {
      font-family: system-ui, sans-serif;
      max-width: 720px;
      margin: 0 auto;
      padding: 1rem;
      background: #0f172a;
      color: #e2e8f0;
    }
    h1 { margin: 0 0 0.25rem; font-size: 1.5rem; }
    .site-header {
      text-align: center;
      margin-bottom: 1.25rem;
    }
    .site-logo {
      display: block;
      max-width: min(100%, 420px);
      width: 100%;
      height: auto;
      margin: 0 auto 0.75rem;
    }
    .site-hostname {
      margin: 0;
      text-align: center;
      font-size: 1.5rem;
      word-break: break-word;
    }
    .sub { color: #94a3b8; margin-bottom: 1rem; font-size: 0.9rem; }
    nav { margin-bottom: 1.25rem; display: flex; gap: 0.5rem; flex-wrap: wrap; }
    nav a {
      color: #93c5fd;
      text-decoration: none;
      padding: 0.35rem 0.7rem;
      border-radius: 8px;
      background: #1e293b;
      font-size: 0.9rem;
    }
    nav a:hover { background: #334155; }
    nav a.active { background: #2563eb; color: white; }
    section {
      background: #1e293b;
      border-radius: 10px;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    h2 { margin: 0 0 0.75rem; font-size: 1rem; color: #cbd5e1; }
    .grid { display: grid; gap: 0.75rem; }
    .gpio-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 0.6rem 0.75rem;
      background: #334155;
      border-radius: 8px;
    }
    .gpio-label { font-weight: 600; }
    .badge {
      font-size: 0.75rem;
      padding: 0.15rem 0.5rem;
      border-radius: 999px;
      margin-left: 0.5rem;
    }
    .badge.on { background: #166534; color: #bbf7d0; }
    .badge.off { background: #475569; color: #cbd5e1; }
    button, .btn {
      border: none;
      border-radius: 8px;
      padding: 0.45rem 0.9rem;
      cursor: pointer;
      font-weight: 600;
      background: #2563eb;
      color: white;
      font-size: 0.9rem;
    }
    button:hover { background: #1d4ed8; }
    button.off-btn, .btn-secondary { background: #64748b; }
    button.off-btn:hover, .btn-secondary:hover { background: #475569; }
    button.warn-btn, .btn-warn { background: #b45309; }
    button.warn-btn:hover, .btn-warn:hover { background: #92400e; }
    button.danger-btn, .btn-danger { background: #dc2626; }
    button.danger-btn:hover, .btn-danger:hover { background: #b91c1c; }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 0.5rem;
    }
    .stat {
      background: #334155;
      border-radius: 8px;
      padding: 0.6rem 0.75rem;
    }
    .stat-label { font-size: 0.75rem; color: #94a3b8; }
    .stat-value { font-size: 1rem; font-weight: 600; margin-top: 0.15rem; word-break: break-all; }
    #log {
      font-family: ui-monospace, monospace;
      font-size: 0.8rem;
      background: #0b1220;
      border-radius: 8px;
      padding: 0.75rem;
      max-height: 220px;
      overflow-y: auto;
      white-space: pre-wrap;
      color: #a5f3fc;
    }
    .footer { color: #64748b; font-size: 0.8rem; text-align: center; }
    .field { display: grid; gap: 0.35rem; margin-bottom: 0.75rem; }
    .field label { font-size: 0.8rem; color: #94a3b8; }
    input[type="number"], input[type="text"], input[type="password"], select {
      width: 100%;
      border: 1px solid #475569;
      border-radius: 8px;
      padding: 0.55rem 0.75rem;
      background: #0b1220;
      color: #e2e8f0;
      font-size: 1rem;
    }
    .hint { font-size: 0.75rem; color: #64748b; margin-top: 0.5rem; }
    .action-row { display: flex; align-items: center; gap: 0.75rem; flex-wrap: wrap; }
    .msg { font-size: 0.85rem; min-height: 1.2rem; }
    .msg.ok { color: #86efac; }
    .msg.err { color: #fca5a5; }
    .msg.warn { color: #fcd34d; }
    fieldset {
      border: 1px solid #334155;
      border-radius: 8px;
      padding: 0.75rem;
      margin: 0 0 1rem;
    }
    legend { color: #94a3b8; font-size: 0.85rem; padding: 0 0.25rem; }
    .banner {
      background: #422006;
      border: 1px solid #b45309;
      border-radius: 10px;
      padding: 0.85rem 1rem;
      margin-bottom: 1rem;
      color: #fde68a;
      font-size: 0.9rem;
    }
    .banner strong { color: #fef3c7; }
    .banner.test-mode {
      background: #312e81;
      border: 1px solid #6366f1;
      color: #c7d2fe;
    }
    .banner.test-mode strong { color: #e0e7ff; }
    .banner.test-mode a { color: #a5b4fc; }
    .traffic-row {
      display: flex;
      gap: 1rem;
      flex-wrap: wrap;
      margin-bottom: 0.75rem;
    }
    .traffic-item {
      display: flex;
      align-items: center;
      gap: 0.6rem;
      background: #334155;
      border-radius: 8px;
      padding: 0.65rem 0.9rem;
      min-width: 180px;
    }
    .traffic-item.active {
      background: #3f4f63;
      outline: 1px solid #64748b;
    }
    .traffic-dot {
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: #475569;
      box-shadow: inset 0 0 0 1px #64748b;
      flex-shrink: 0;
      transition: background 0.15s, box-shadow 0.15s;
    }
    .traffic-dot.dmx1.on {
      background: #22c55e;
      box-shadow: 0 0 10px #22c55e;
    }
    .traffic-dot.on {
      background: #22c55e;
      box-shadow: 0 0 10px #22c55e;
    }
    .traffic-dot.dmx2.on {
      background: #38bdf8;
      box-shadow: 0 0 10px #38bdf8;
    }
    .traffic-dot.dmx1in.on {
      background: #f59e0b;
      box-shadow: 0 0 10px #f59e0b;
    }
    .traffic-label { font-weight: 600; }
    .traffic-state { font-size: 0.8rem; color: #94a3b8; }
    .slider-row {
      display: grid;
      grid-template-columns: 72px 1fr 48px;
      gap: 0.5rem;
      align-items: center;
      margin-bottom: 0.5rem;
      font-size: 0.85rem;
    }
    .slider-row input[type="range"] { width: 100%; }
    .slider-val { text-align: right; color: #94a3b8; font-variant-numeric: tabular-nums; }
    textarea {
      width: 100%;
      min-height: 120px;
      border: 1px solid #475569;
      border-radius: 8px;
      padding: 0.55rem 0.75rem;
      background: #0b1220;
      color: #a5f3fc;
      font-family: ui-monospace, monospace;
      font-size: 0.75rem;
      resize: vertical;
    }
    label.check-row {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      font-size: 0.9rem;
      margin-bottom: 0.75rem;
    }
)rawliteral";

static bool webAuthEnabled() {
  return strlen(cfg.webPassword) > 0;
}

static bool authenticateWeb() {
  if (!webAuthEnabled()) {
    return true;
  }
  if (!server.authenticate(cfg.hostname, cfg.webPassword)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

static void sendConfigNav(const char *activePage) {
  server.sendContent(F("<nav>"));
  server.sendContent(F("<a href=\"/\""));
  if (strcmp(activePage, "home") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(" data-i18n=\"nav_dashboard\">Dashboard</a><a href=\"/config\""));
  if (strcmp(activePage, "config") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(" data-i18n=\"nav_config\">Config</a><a href=\"/dmx-test\""));
  if (strcmp(activePage, "dmx-test") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(" data-i18n=\"nav_dmx_test\">DMX Test</a></nav>"));
}

static void sendNav(const char *activePage) {
  server.sendContent(F("<nav>"));
  server.sendContent(F("<a href=\"/\""));
  if (strcmp(activePage, "home") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(">Dashboard</a><a href=\"/config\""));
  if (strcmp(activePage, "config") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(">Config</a><a href=\"/dmx-test\""));
  if (strcmp(activePage, "dmx-test") == 0) {
    server.sendContent(F(" class=\"active\""));
  }
  server.sendContent(F(">DMX Test</a></nav>"));
}

static void sendPageHead(const char *titleSuffix) {
  server.sendContent(F("<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"));
  server.sendContent(F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
  server.sendContent(F("<title>"));
  server.sendContent(String(cfg.hostname));
  if (titleSuffix != nullptr && titleSuffix[0] != '\0') {
    server.sendContent(F(" · "));
    server.sendContent(titleSuffix);
  }
  server.sendContent(F("</title><style>"));
  server.sendContent(PAGE_STYLE);
  server.sendContent(F("</style></head><body><header class=\"site-header\">"));
  server.sendContent(F("<img class=\"site-logo\" src=\"/artdmx-bridge32-logo.png\" alt=\"artdmx-bridge32 logo\">"));
  server.sendContent(F("<h1 class=\"site-hostname\">"));
  server.sendContent(String(cfg.hostname));
  server.sendContent(F("</h1></header>"));
}

static void sendCopyrightFooter() {
  server.sendContent(F("<p class=\"footer\">"));
  server.sendContent(String(DEVICE_NAME));
  server.sendContent(F(" &middot; "));
  server.sendContent(COPYRIGHT_LINE);
  server.sendContent(F(" &middot; <a href=\""));
  server.sendContent(LICENSE_URL);
  server.sendContent(F("\" rel=\"license\">"));
  server.sendContent(LICENSE_LINE);
  server.sendContent(F("</a></p>"));
}

static void handleLogo() {
  if (!authenticateWeb()) {
    return;
  }
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.send_P(200, PSTR("image/png"),
                reinterpret_cast<PGM_P>(LOGO_PNG_DATA), LOGO_PNG_LEN);
}

static void handleRoot() {
  if (!authenticateWeb()) {
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendPageHead(nullptr);
  server.sendContent(F("<p class=\"sub\">ArtNet DMX Gateway · status · log</p>"));
  sendNav("home");

  server.sendContent(F(R"page(
  <div id="ap-banner" class="banner" style="display:none"></div>
  <div id="dmx-test-banner" class="banner test-mode" style="display:none"></div>
  <section>
    <h2>DMX traffic</h2>
    <div class="traffic-row">
      <div class="traffic-item" id="dmx1-out-item">
        <div class="traffic-dot dmx1" id="dmx1-out-dot"></div>
        <div>
          <div class="traffic-label">DMX1 output</div>
          <div class="traffic-state" id="dmx1-out-traffic">Idle</div>
        </div>
      </div>
      <div class="traffic-item" id="dmx2-out-item">
        <div class="traffic-dot dmx2" id="dmx2-out-dot"></div>
        <div>
          <div class="traffic-label">DMX2 output</div>
          <div class="traffic-state" id="dmx2-out-traffic">Idle</div>
        </div>
      </div>
      <div class="traffic-item" id="dmx1-in-item" style="display:none">
        <div class="traffic-dot dmx1in" id="dmx1-in-dot"></div>
        <div>
          <div class="traffic-label">DMX1 input</div>
          <div class="traffic-state" id="dmx1-in-traffic">Idle</div>
        </div>
      </div>
    </div>
    <p class="hint">Green = DMX1 out, blue = DMX2 out, amber = DMX1 in (when enabled). Active for 2s after last packet.</p>
  </section>
  <section>
    <h2>Status</h2>
    <div class="status-grid">
      <div class="stat"><div class="stat-label">Hostname</div><div class="stat-value" id="hostname">-</div></div>
      <div class="stat"><div class="stat-label">URL</div><div class="stat-value" id="url">-</div></div>
      <div class="stat"><div class="stat-label">IP</div><div class="stat-value" id="ip">-</div></div>
      <div class="stat"><div class="stat-label">RSSI</div><div class="stat-value" id="rssi">-</div></div>
      <div class="stat"><div class="stat-label">Uptime</div><div class="stat-value" id="uptime">-</div></div>
      <div class="stat"><div class="stat-label">Free heap</div><div class="stat-value" id="heap">-</div></div>
      <div class="stat"><div class="stat-label">OTA</div><div class="stat-value" id="ota">-</div></div>
      <div class="stat"><div class="stat-label">ArtNet</div><div class="stat-value" id="artnet">-</div></div>
      <div class="stat"><div class="stat-label">DMX test</div><div class="stat-value" id="dmx-test">-</div></div>
      <div class="stat"><div class="stat-label">Universe</div><div class="stat-value" id="artnet-uni">-</div></div>
      <div class="stat"><div class="stat-label">DMX refresh</div><div class="stat-value" id="dmx-refresh">-</div></div>
      <div class="stat"><div class="stat-label">DMX1 input</div><div class="stat-value" id="dmx1-in">-</div></div>
      <div class="stat"><div class="stat-label">DMX2 output</div><div class="stat-value" id="dmx2-out">-</div></div>
      <div class="stat"><div class="stat-label">DMX1 in filter</div><div class="stat-value" id="dmx1-filter">-</div></div>
    </div>
    <p class="hint" style="margin-top:0.75rem">GPIO 13 = DMX1 out, GPIO 14 = DMX2 out / DMX1 in, GPIO 22 = WiFi status (steady / single / double / triple blink during OTA / breathing during DMX test). Factory reset: GPIO 15 (3 s) or Config page.</p>
  </section>
  <section>
    <h2>Serial log</h2>
    <div id="log">Loading...</div>
  </section>
  <p class="footer">Refresh every 500ms · <a href="/config" style="color:#93c5fd">Edit config</a></p>
  <script>
    var lastLogSeq = -1;
    var logStickToBottom = true;
    function formatUptime(ms) {
      var s = Math.floor(ms / 1000);
      var h = Math.floor(s / 3600);
      var m = Math.floor((s % 3600) / 60);
      var sec = s % 60;
      return h + 'h ' + m + 'm ' + sec + 's';
    }
    function setTraffic(dotId, textId, itemId, active, onColor) {
      var dot = document.getElementById(dotId);
      var text = document.getElementById(textId);
      var item = itemId ? document.getElementById(itemId) : null;
      if (!dot || !text) return;
      var on = active === true || active === 'true' || active === 1;
      if (on) {
        dot.classList.add('on');
        dot.style.backgroundColor = onColor;
        dot.style.boxShadow = '0 0 12px ' + onColor;
        text.textContent = 'Traffic';
        text.style.color = onColor;
        if (item) item.classList.add('active');
      } else {
        dot.classList.remove('on');
        dot.style.backgroundColor = '#475569';
        dot.style.boxShadow = 'inset 0 0 0 1px #64748b';
        text.textContent = 'Idle';
        text.style.color = '#94a3b8';
        if (item) item.classList.remove('active');
      }
    }
    function updateLogPanel(log) {
      var logEl = document.getElementById('log');
      if (!logEl) return;
      var text = (log.lines && log.lines.length) ? log.lines.join('\n') : '(empty)';
      if (log.seq !== lastLogSeq || logEl.textContent !== text) {
        logEl.textContent = text;
        lastLogSeq = log.seq;
      }
      if (logStickToBottom) {
        logEl.scrollTop = logEl.scrollHeight;
      }
    }
    (function() {
      var logEl = document.getElementById('log');
      if (!logEl) return;
      logEl.addEventListener('scroll', function() {
        logStickToBottom = logEl.scrollHeight - logEl.scrollTop - logEl.clientHeight < 12;
      });
    })();
    async function refresh() {
      try {
        var statusRes = await fetch('/api/status?ts=' + Date.now(), { cache: 'no-store' });
        var logRes = await fetch('/api/log?ts=' + Date.now(), { cache: 'no-store' });
        var status = await statusRes.json();
        var log = await logRes.json();
        document.getElementById('hostname').textContent = status.hostname;
        document.getElementById('url').textContent = status.wifi_connected ? status.mdns_url : ('http://' + status.ip);
        document.getElementById('ip').textContent = status.ip;
        document.getElementById('rssi').textContent = status.wifi_connected ? (status.rssi + ' dBm') : 'AP mode';
        document.getElementById('uptime').textContent = formatUptime(status.uptime_ms);
        document.getElementById('heap').textContent = status.free_heap + ' B';
        document.getElementById('ota').textContent = status.ota_ready ? 'Ready' : 'Waiting';
        var testOn = status.dmx_test_mode === true;
        document.getElementById('dmx-test').textContent = testOn ? 'Active' : 'Off';
        document.getElementById('dmx-test').style.color = testOn ? '#a78bfa' : '';
        document.getElementById('artnet').textContent =
          testOn ? 'Ignored' : (status.artnet_active ? 'Active' : 'Idle');
        document.getElementById('artnet').style.color = testOn ? '#a78bfa' : '';
        var testBanner = document.getElementById('dmx-test-banner');
        if (testOn) {
          testBanner.style.display = 'block';
          testBanner.innerHTML =
            '<strong>DMX test mode active</strong> — Art-Net is ignored. Slider values from the ' +
            '<a href="/dmx-test">DMX Test page</a> are sent to DMX1. Status LED breathes (GPIO 22).';
        } else {
          testBanner.style.display = 'none';
        }
        setTraffic('dmx1-out-dot', 'dmx1-out-traffic', 'dmx1-out-item', status.dmx1_out_traffic, '#22c55e');
        document.getElementById('dmx2-out-item').style.display =
          (!status.enable_dmx_input && status.enable_dmx2_output) ? 'flex' : 'none';
        document.getElementById('dmx1-in-item').style.display = status.enable_dmx_input ? 'flex' : 'none';
        if (!status.enable_dmx_input && status.enable_dmx2_output) {
          setTraffic('dmx2-out-dot', 'dmx2-out-traffic', 'dmx2-out-item', status.dmx2_out_traffic, '#38bdf8');
        } else {
          setTraffic('dmx2-out-dot', 'dmx2-out-traffic', 'dmx2-out-item', false, '#38bdf8');
        }
        if (status.enable_dmx_input) {
          setTraffic('dmx1-in-dot', 'dmx1-in-traffic', 'dmx1-in-item', status.dmx1_in_traffic, '#f59e0b');
        } else {
          setTraffic('dmx1-in-dot', 'dmx1-in-traffic', 'dmx1-in-item', false, '#f59e0b');
        }
        document.getElementById('artnet-uni').textContent = status.artnet_universe;
        document.getElementById('dmx-refresh').textContent = status.dmx_refresh_ms + ' ms';
        document.getElementById('dmx1-in').textContent = status.enable_dmx_input ? 'On' : 'Off';
        document.getElementById('dmx2-out').textContent =
          status.enable_dmx_input ? 'Off' : (status.enable_dmx2_output ? 'On' : 'Off');
        document.getElementById('dmx1-filter').textContent = status.dmx2_filter_start + '–' + status.dmx2_filter_end;
        updateLogPanel(log);
        var banner = document.getElementById('ap-banner');
        if (status.ap_mode) {
          banner.style.display = 'block';
          banner.innerHTML = '<strong>Setup mode:</strong> WiFi unavailable. Connect to <strong>' +
            status.ap_ssid + '</strong>, then open <strong>http://' + status.ip + '</strong> to configure WiFi.';
        } else {
          banner.style.display = 'none';
        }
      } catch (e) {
        document.getElementById('log').textContent = 'Connection error';
      }
    }
    refresh();
    setInterval(refresh, 500);
  </script>
  )page"));
  sendCopyrightFooter();
  server.sendContent(F("</body></html>"));
  server.sendContent("");
}

static void handleConfigPage() {
  if (!authenticateWeb()) {
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendPageHead("Config");
  server.sendContent(F("<p class=\"sub\" data-i18n=\"page_sub\">Settings stored in flash · reboot after DMX1 input mode changes</p>"));
  sendConfigNav("config");

  server.sendContent(F(R"page(
  <section>
    <form id="config-form" onsubmit="return false;">
      <fieldset>
        <legend data-i18n="section_device">Device</legend>
        <div class="field">
          <label for="ui-lang" data-i18n="label_ui_lang">Interface language</label>
          <select id="ui-lang">
            <option value="0" data-i18n="lang_auto">Autodetect</option>
            <option value="1" data-i18n="lang_en">English</option>
            <option value="2" data-i18n="lang_de">German</option>
          </select>
        </div>
        <div class="field">
          <label for="hostname" data-i18n="label_hostname">Hostname (mDNS, OTA, web login name)</label>
          <input type="text" id="hostname" maxlength="31" required>
        </div>
        <div class="field">
          <label for="web-pass" data-i18n="label_web_pass">Web UI password (leave empty to disable)</label>
          <input type="password" id="web-pass" maxlength="31">
        </div>
      </fieldset>
      <fieldset>
        <legend data-i18n="section_wifi">WiFi</legend>
        <div class="field">
          <label for="wifi-ssid" data-i18n="label_wifi_ssid">SSID</label>
          <input type="text" id="wifi-ssid" maxlength="32" required>
        </div>
        <div class="field">
          <label for="wifi-pass" data-i18n="label_wifi_pass">Password</label>
          <input type="password" id="wifi-pass" maxlength="63">
        </div>
      </fieldset>
      <fieldset>
        <legend data-i18n="section_ota">OTA</legend>
        <div class="field">
          <label for="ota-pass" data-i18n="label_ota_pass">OTA password (leave empty to disable)</label>
          <input type="password" id="ota-pass" maxlength="31">
        </div>
      </fieldset>
      <fieldset>
        <legend data-i18n="section_artnet">ArtNet / DMX</legend>
        <div class="field">
          <label for="artnet-universe" data-i18n="label_artnet_universe">ArtNet universe</label>
          <input type="number" id="artnet-universe" min="0" max="32767" step="1" required>
        </div>
        <div class="field">
          <label for="artnet-timeout" data-i18n="label_artnet_timeout">ArtNet timeout (ms)</label>
          <input type="number" id="artnet-timeout" min="500" max="60000" step="100" required>
        </div>
        <div class="field">
          <label for="dmx-refresh" data-i18n="label_dmx_refresh">DMX refresh interval (ms)</label>
          <input type="number" id="dmx-refresh" min="10" max="1000" step="1" required>
        </div>
        <div class="field">
          <label><input type="checkbox" id="send-full-packet"> <span data-i18n="field_send_full_packet">Send full 512-channel DMX packets</span></label>
        </div>
        <div class="field">
          <label><input type="checkbox" id="enable-dmx2-output"> <span data-i18n="field_enable_dmx2">Enable DMX2 output (mirrors ArtNet to GPIO 19/18/21)</span></label>
        </div>
        <div class="field">
          <label><input type="checkbox" id="enable-dmx-input"> <span data-i18n="field_enable_dmx_input">Enable DMX1 wired input (disables DMX2 output)</span></label>
        </div>
        <div class="field">
          <label for="dmx2-filter-start" data-i18n="label_filter_start">Channel filter start (1–512)</label>
          <input type="number" id="dmx2-filter-start" min="1" max="512" step="1" required>
        </div>
        <div class="field">
          <label for="dmx2-filter-end" data-i18n="label_filter_end">Channel filter end (1–512)</label>
          <input type="number" id="dmx2-filter-end" min="1" max="512" step="1" required>
        </div>
        <p class="hint" data-i18n="hint_port2">DMX1 input: forwards only this range to DMX1 out. DMX2 output: sends only this range, and only when a value in the range changes.</p>
        <div class="field">
          <label for="artnet-debug-mode" data-i18n="label_artnet_debug_mode">ArtNet console log</label>
          <select id="artnet-debug-mode">
            <option value="0" data-i18n="debug_off">Off</option>
            <option value="1" data-i18n="debug_matched">Matching universe only</option>
            <option value="2" data-i18n="debug_ignored">Other universes only (ignored)</option>
            <option value="3" data-i18n="debug_all">All universes</option>
          </select>
        </div>
        <div class="field">
          <label for="artnet-debug-ch-start" data-i18n="label_debug_ch_start">Debug log channel start (1–512)</label>
          <input type="number" id="artnet-debug-ch-start" min="1" max="512" step="1" value="1">
        </div>
        <div class="field">
          <label for="artnet-debug-ch-end" data-i18n="label_debug_ch_end">Debug log channel end (1–512)</label>
          <input type="number" id="artnet-debug-ch-end" min="1" max="512" step="1" value="4">
        </div>
        <div class="field">
          <label for="artnet-debug-every" data-i18n="label_debug_every">Log every Nth packet (1 = every packet)</label>
          <input type="number" id="artnet-debug-every" min="1" max="1000" step="1" value="1">
        </div>
        <div class="field">
          <label><input type="checkbox" id="artnet-debug-on-change"> <span data-i18n="field_debug_on_change">Log only when debug channel values change</span></label>
        </div>
        <p class="hint" data-i18n="hint_debug_log">Logged lines show DMX values for the debug channel range (e.g. 3–7). With “values change” enabled, identical frames are not logged.</p>
      </fieldset>
      <fieldset>
        <legend data-i18n="section_backup">Backup / restore</legend>
        <p class="hint" data-i18n="hint_backup">Download all permanent settings as a text file (artdmx-bridge32.conf), edit in any text editor, then upload. Syntax errors or illegal values are rejected and existing settings are left unchanged.</p>
        <div class="action-row">
          <button type="button" class="btn-secondary" onclick="downloadConfig()" data-i18n="btn_download">Download .conf</button>
          <input type="file" id="config-file" accept=".conf,.txt,text/plain">
        </div>
        <div class="field">
          <label for="config-upload-text" data-i18n="label_upload_text">Or paste configuration text</label>
          <textarea id="config-upload-text" rows="10" placeholder="# Paste artdmx-bridge32.conf contents here"></textarea>
        </div>
        <div class="action-row">
          <button type="button" onclick="uploadConfig(false)" data-i18n="btn_upload">Upload</button>
          <button type="button" class="warn-btn" onclick="uploadConfig(true)" data-i18n="btn_upload_reboot">Upload and Reboot</button>
        </div>
      </fieldset>
      <fieldset>
        <legend data-i18n="section_danger">Danger zone</legend>
        <p class="hint" data-i18n="hint_danger">Erases all saved settings (WiFi, hostname, DMX options) and reboots with factory defaults.</p>
        <button type="button" class="danger-btn" onclick="factoryReset()" data-i18n="btn_factory_reset">Factory reset</button>
      </fieldset>
      <div class="action-row">
        <button type="button" onclick='saveConfig(false)' data-i18n="btn_save">Save</button>
        <button type="button" class="warn-btn" onclick='saveConfig(true)' data-i18n="btn_save_reboot">Save and Reboot</button>
        <button type="button" class="btn-secondary" onclick='rebootDevice()' data-i18n="btn_reboot">Reboot</button>
        <span id="cfg-msg" class="msg"></span>
      </div>
      <p class="hint"><span data-i18n="hint_footer_before">WiFi and hostname changes take full effect after reboot.</span><strong id="ap-hint-ssid">hostname-setup</strong><span data-i18n="hint_footer_mid"> access point and open the device IP. When on your network, use </span><strong id="mdns-hint">http://hostname.local</strong><span data-i18n="hint_footer_after">.</span></p>
    </form>
  </section>
  <script>
)page"));
  server.sendContent(FPSTR(CONFIG_PAGE_I18N_SCRIPT));
  server.sendContent(F(R"page(
    function setMsg(text, kind) {
      var el = document.getElementById('cfg-msg');
      el.textContent = text;
      el.className = 'msg ' + (kind || '');
    }
    function syncPort2Options() {
      var inputOn = document.getElementById('enable-dmx-input').checked;
      var dmx2 = document.getElementById('enable-dmx2-output');
      if (inputOn) {
        dmx2.checked = false;
        dmx2.disabled = true;
      } else {
        dmx2.disabled = false;
      }
    }
    function fillForm(data) {
      document.getElementById('ui-lang').value = String(data.ui_lang != null ? data.ui_lang : 0);
      setLangFromPref(parseInt(document.getElementById('ui-lang').value, 10), true);
      document.getElementById('hostname').value = data.hostname || data.ota_hostname || '';
      document.getElementById('web-pass').value = data.web_password || '';
      document.getElementById('wifi-ssid').value = data.wifi_ssid || '';
      document.getElementById('wifi-pass').value = data.wifi_pass || '';
      document.getElementById('ota-pass').value = data.ota_password || '';
      document.getElementById('artnet-universe').value = data.artnet_universe;
      document.getElementById('artnet-timeout').value = data.artnet_timeout_ms;
      document.getElementById('dmx-refresh').value = data.dmx_refresh_ms;
      document.getElementById('send-full-packet').checked = !!data.send_full_packet;
      document.getElementById('enable-dmx2-output').checked = !!data.enable_dmx2_output;
      document.getElementById('enable-dmx-input').checked = !!data.enable_dmx_input;
      syncPort2Options();
      document.getElementById('dmx2-filter-start').value = data.dmx2_filter_start;
      document.getElementById('dmx2-filter-end').value = data.dmx2_filter_end;
      document.getElementById('artnet-debug-mode').value =
        String(data.artnet_debug_mode != null ? data.artnet_debug_mode :
          (data.debug_artnet ? 1 : 0));
      document.getElementById('artnet-debug-ch-start').value =
        data.artnet_debug_ch_start != null ? data.artnet_debug_ch_start : 1;
      document.getElementById('artnet-debug-ch-end').value =
        data.artnet_debug_ch_end != null ? data.artnet_debug_ch_end : 4;
      document.getElementById('artnet-debug-every').value =
        data.artnet_debug_every != null ? data.artnet_debug_every : 1;
      document.getElementById('artnet-debug-on-change').checked = !!data.artnet_debug_on_change;
      var host = data.hostname || data.ota_hostname || 'hostname';
      document.getElementById('ap-hint-ssid').textContent = host + '-setup';
      document.getElementById('mdns-hint').textContent = 'http://' + host + '.local';
      document.title = host + ' · ' + t('page_title');
    }
    async function loadConfig() {
      var res = await fetch('/api/config');
      fillForm(await res.json());
    }
    function downloadConfig() {
      window.location.href = '/api/config/download?ts=' + Date.now();
    }
    async function uploadConfig(reboot) {
      var text = document.getElementById('config-upload-text').value;
      var fileInput = document.getElementById('config-file');
      if (fileInput.files && fileInput.files[0]) {
        try {
          text = await fileInput.files[0].text();
        } catch (e) {
          setMsg(t('msg_could_not_read'), 'err');
          return;
        }
      }
      if (!text || !text.trim()) {
        setMsg(t('msg_choose_file'), 'err');
        return;
      }
      try {
        var url = '/api/config/upload?ts=' + Date.now();
        if (reboot) {
          url += '&reboot=1';
        }
        var res = await fetch(url, {
          method: 'POST',
          headers: { 'Content-Type': 'text/plain; charset=utf-8' },
          body: text
        });
        var data = await res.json();
        if (!res.ok) {
          setMsg(data.error || t('msg_upload_failed'), 'err');
          return;
        }
        setMsg(data.reboot ? t('msg_rebooting') : t('msg_uploaded'), data.reboot ? 'warn' : 'ok');
        await loadConfig();
        if (data.reboot) {
          setTimeout(function() { setMsg(t('msg_rebooting'), 'warn'); }, 400);
        }
      } catch (e) {
        setMsg(t('msg_conn_error'), 'err');
      }
    }
    async function saveConfig(reboot) {
      var params = new URLSearchParams();
      params.set('ui_lang', document.getElementById('ui-lang').value);
      params.set('hostname', document.getElementById('hostname').value);
      params.set('ota_hostname', document.getElementById('hostname').value);
      params.set('web_password', document.getElementById('web-pass').value);
      params.set('wifi_ssid', document.getElementById('wifi-ssid').value);
      params.set('wifi_pass', document.getElementById('wifi-pass').value);
      params.set('ota_password', document.getElementById('ota-pass').value);
      params.set('artnet_universe', document.getElementById('artnet-universe').value);
      params.set('artnet_timeout_ms', document.getElementById('artnet-timeout').value);
      params.set('dmx_refresh_ms', document.getElementById('dmx-refresh').value);
      params.set('send_full_packet', document.getElementById('send-full-packet').checked ? '1' : '0');
      params.set('enable_dmx2_output', document.getElementById('enable-dmx2-output').checked ? '1' : '0');
      params.set('enable_dmx_input', document.getElementById('enable-dmx-input').checked ? '1' : '0');
      params.set('dmx2_filter_start', document.getElementById('dmx2-filter-start').value);
      params.set('dmx2_filter_end', document.getElementById('dmx2-filter-end').value);
      params.set('artnet_debug_mode', document.getElementById('artnet-debug-mode').value);
      params.set('artnet_debug_ch_start', document.getElementById('artnet-debug-ch-start').value);
      params.set('artnet_debug_ch_end', document.getElementById('artnet-debug-ch-end').value);
      params.set('artnet_debug_every', document.getElementById('artnet-debug-every').value);
      params.set('artnet_debug_on_change', document.getElementById('artnet-debug-on-change').checked ? '1' : '0');
      params.set('reboot', reboot ? '1' : '0');
      try {
        var res = await fetch('/api/config', { method: 'POST', body: params });
        var data = await res.json();
        if (!res.ok) {
          setMsg(data.error || t('msg_save_failed'), 'err');
          return;
        }
        if (data.reboot) {
          setMsg(t('msg_saved_reboot'), 'warn');
          setTimeout(function() { setMsg(t('msg_rebooting'), 'warn'); }, 400);
        } else if (data.message && data.message.indexOf('Reboot recommended') >= 0) {
          setMsg(t('msg_saved_reboot_hint'), 'ok');
        } else {
          setMsg(t('msg_saved'), 'ok');
        }
      } catch (e) {
        setMsg(t('msg_conn_error'), 'err');
      }
    }
    async function rebootDevice() {
      setMsg(t('msg_rebooting'), 'warn');
      await fetch('/api/reboot', { method: 'POST' });
    }
    async function factoryReset() {
      if (!confirm(t('msg_factory_confirm'))) {
        return;
      }
      setMsg(t('msg_factory_reset'), 'warn');
      try {
        await fetch('/api/factory-reset', { method: 'POST' });
      } catch (e) {
        setMsg(t('msg_device_rebooting'), 'warn');
      }
    }
    document.getElementById('enable-dmx-input').addEventListener('change', syncPort2Options);
    document.getElementById('ui-lang').addEventListener('change', function() {
      setLangFromPref(parseInt(this.value, 10), true);
    });
    document.getElementById('hostname').addEventListener('input', function() {
      var host = this.value || 'hostname';
      document.getElementById('ap-hint-ssid').textContent = host + '-setup';
      document.getElementById('mdns-hint').textContent = 'http://' + host + '.local';
      document.title = host + ' · ' + t('page_title');
    });
    setLangFromPref(0, true);
    loadConfig();
  </script>
  )page"));
  sendCopyrightFooter();
  server.sendContent(F("</body></html>"));
  server.sendContent("");
}

static void handleConfigGet() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "application/json", buildConfigJson());
}

static void handleConfigDownload() {
  if (!authenticateWeb()) {
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=\"artdmx-bridge32.conf\"");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain; charset=utf-8", exportConfigText());
}

static void handleConfigUpload() {
  if (!authenticateWeb()) {
    return;
  }

  DeviceConfig previous = cfg;
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
  } else if (server.hasArg("config")) {
    body = server.arg("config");
  } else {
    server.send(400, "application/json", "{\"error\":\"Empty upload body\"}");
    return;
  }

  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Empty configuration file\"}");
    return;
  }

  String error;
  if (!importConfigText(body, error)) {
    server.send(400, "application/json", "{\"error\":\"" + jsonEscape(error) + "\"}");
    return;
  }

  bool networkChanged = strcmp(previous.wifiSsid, cfg.wifiSsid) != 0 ||
                        strcmp(previous.wifiPass, cfg.wifiPass) != 0 ||
                        strcmp(previous.hostname, cfg.hostname) != 0 ||
                        strcmp(previous.otaPassword, cfg.otaPassword) != 0 ||
                        strcmp(previous.webPassword, cfg.webPassword) != 0;
  bool dmxInputChanged = previous.enableDmxInput != cfg.enableDmxInput;
  bool dmx2OutputChanged = previous.enableDmx2Output != cfg.enableDmx2Output;
  bool rebootRequested = server.hasArg("reboot") && server.arg("reboot").toInt() != 0;

  if (rebootRequested) {
    server.send(200, "application/json",
                 "{\"message\":\"Configuration imported, rebooting\",\"reboot\":true}");
    server.client().flush();
    delay(250);
    ESP.restart();
    return;
  }

  String message = (networkChanged || dmxInputChanged || dmx2OutputChanged)
                     ? "Configuration imported. Reboot recommended for WiFi/hostname or port 2 mode changes."
                     : "Configuration imported.";
  server.send(200, "application/json",
               "{\"message\":\"" + jsonEscape(message) + "\",\"reboot\":false}");
}

static void handleConfigPost() {
  if (!authenticateWeb()) {
    return;
  }

  DeviceConfig previous = cfg;

  if (!server.hasArg("wifi_ssid") || server.arg("wifi_ssid").length() == 0) {
    server.send(400, "application/json", "{\"error\":\"wifi_ssid required\"}");
    return;
  }

  String hostArg = server.hasArg("hostname") ? server.arg("hostname") : server.arg("ota_hostname");
  if (hostArg.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"hostname required\"}");
    return;
  }

  if (!copyArgField(cfg.wifiSsid, sizeof(cfg.wifiSsid), server.arg("wifi_ssid").c_str(), "wifi_ssid") ||
      !copyArgField(cfg.wifiPass, sizeof(cfg.wifiPass), server.arg("wifi_pass").c_str(), "wifi_pass") ||
      !copyArgField(cfg.hostname, sizeof(cfg.hostname), hostArg.c_str(), "hostname") ||
      !copyArgField(cfg.otaPassword, sizeof(cfg.otaPassword), server.arg("ota_password").c_str(), "ota_password") ||
      !copyArgField(cfg.webPassword, sizeof(cfg.webPassword), server.arg("web_password").c_str(), "web_password")) {
    server.send(400, "application/json", "{\"error\":\"field too long\"}");
    return;
  }

  if (server.hasArg("artnet_universe")) {
    int uni = server.arg("artnet_universe").toInt();
    if (uni < 0 || uni > 32767) {
      server.send(400, "application/json", "{\"error\":\"artnet_universe out of range\"}");
      return;
    }
    cfg.artnetUniverse = (uint16_t)uni;
  }

  if (server.hasArg("artnet_timeout_ms")) {
    uint32_t timeoutMs = (uint32_t)server.arg("artnet_timeout_ms").toInt();
    if (timeoutMs < 500 || timeoutMs > 60000) {
      server.send(400, "application/json", "{\"error\":\"artnet_timeout_ms out of range\"}");
      return;
    }
    cfg.artnetTimeoutMs = timeoutMs;
  }

  if (server.hasArg("dmx_refresh_ms")) {
    uint32_t refreshMs = (uint32_t)server.arg("dmx_refresh_ms").toInt();
    if (refreshMs < 10 || refreshMs > 1000) {
      server.send(400, "application/json", "{\"error\":\"dmx_refresh_ms out of range\"}");
      return;
    }
    cfg.dmxRefreshMs = refreshMs;
  }

  if (server.hasArg("send_full_packet")) {
    cfg.sendFullPacket = server.arg("send_full_packet").toInt() != 0;
  }

  if (server.hasArg("enable_dmx_input")) {
    cfg.enableDmxInput = server.arg("enable_dmx_input").toInt() != 0;
  }

  if (server.hasArg("enable_dmx2_output")) {
    cfg.enableDmx2Output = server.arg("enable_dmx2_output").toInt() != 0;
  }

  if (cfg.enableDmxInput) {
    cfg.enableDmx2Output = false;
  }

  if (server.hasArg("dmx2_filter_start")) {
    int start = server.arg("dmx2_filter_start").toInt();
    if (start < 1 || start > 512) {
      server.send(400, "application/json", "{\"error\":\"dmx2_filter_start out of range\"}");
      return;
    }
    cfg.dmx2FilterStart = (uint16_t)start;
  }

  if (server.hasArg("dmx2_filter_end")) {
    int end = server.arg("dmx2_filter_end").toInt();
    if (end < 1 || end > 512) {
      server.send(400, "application/json", "{\"error\":\"dmx2_filter_end out of range\"}");
      return;
    }
    cfg.dmx2FilterEnd = (uint16_t)end;
  }

  if (server.hasArg("artnet_debug_mode")) {
    int mode = server.arg("artnet_debug_mode").toInt();
    if (mode < 0 || mode > 3) {
      server.send(400, "application/json", "{\"error\":\"artnet_debug_mode out of range\"}");
      return;
    }
    cfg.artnetDebugMode = (uint8_t)mode;
  } else if (server.hasArg("debug_artnet")) {
    cfg.artnetDebugMode = server.arg("debug_artnet").toInt() != 0 ? 1 : 0;
  }

  if (server.hasArg("artnet_debug_every")) {
    int every = server.arg("artnet_debug_every").toInt();
    if (every < 1 || every > 1000) {
      server.send(400, "application/json", "{\"error\":\"artnet_debug_every out of range\"}");
      return;
    }
    cfg.artnetDebugEvery = (uint16_t)every;
  }

  if (server.hasArg("artnet_debug_ch_start")) {
    int start = server.arg("artnet_debug_ch_start").toInt();
    if (start < 1 || start > 512) {
      server.send(400, "application/json", "{\"error\":\"artnet_debug_ch_start out of range\"}");
      return;
    }
    cfg.artnetDebugChStart = (uint16_t)start;
  }

  if (server.hasArg("artnet_debug_ch_end")) {
    int end = server.arg("artnet_debug_ch_end").toInt();
    if (end < 1 || end > 512) {
      server.send(400, "application/json", "{\"error\":\"artnet_debug_ch_end out of range\"}");
      return;
    }
    cfg.artnetDebugChEnd = (uint16_t)end;
  }

  if (server.hasArg("artnet_debug_on_change")) {
    cfg.artnetDebugOnChange = server.arg("artnet_debug_on_change").toInt() != 0;
  }

  if (server.hasArg("ui_lang")) {
    int lang = server.arg("ui_lang").toInt();
    if (lang < UI_LANG_AUTO || lang > UI_LANG_DE) {
      server.send(400, "application/json", "{\"error\":\"ui_lang out of range\"}");
      return;
    }
    cfg.uiLang = (uint8_t)lang;
  }

  saveConfigToNvs();
  appendLogf("Config saved: WiFi='%s', hostname='%s', ArtNet uni=%u",
             cfg.wifiSsid, cfg.hostname, cfg.artnetUniverse);

  bool networkChanged = strcmp(previous.wifiSsid, cfg.wifiSsid) != 0 ||
                        strcmp(previous.wifiPass, cfg.wifiPass) != 0 ||
                        strcmp(previous.hostname, cfg.hostname) != 0 ||
                        strcmp(previous.otaPassword, cfg.otaPassword) != 0 ||
                        strcmp(previous.webPassword, cfg.webPassword) != 0;
  bool dmxInputChanged = previous.enableDmxInput != cfg.enableDmxInput;
  bool dmx2OutputChanged = previous.enableDmx2Output != cfg.enableDmx2Output;
  bool rebootRequested = server.hasArg("reboot") && server.arg("reboot").toInt() != 0;

  if (rebootRequested) {
    server.send(200, "application/json",
                 "{\"message\":\"Saved, rebooting\",\"reboot\":true}");
    server.client().flush();
    delay(250);
    ESP.restart();
    return;
  }

  String message = (networkChanged || dmxInputChanged || dmx2OutputChanged)
                     ? "Saved. Reboot recommended for WiFi/hostname or port 2 mode changes."
                     : "Saved.";
  server.send(200, "application/json",
               "{\"message\":\"" + jsonEscape(message) + "\",\"reboot\":false}");
}

static void handleFactoryReset() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "application/json", "{\"message\":\"Factory reset, rebooting\"}");
  server.client().flush();
  delay(250);
  factoryResetAndRestart();
}

static void handleReboot() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "application/json", "{\"message\":\"Rebooting\"}");
  appendLog("Reboot requested via web");
  server.client().flush();
  delay(250);
  ESP.restart();
}

static void handleStatus() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "application/json", buildStatusJson());
}

static void handleLog() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "application/json", buildLogJson());
}

String buildStatusJson() {
  String json = "{";
  json += "\"hostname\":\"" + jsonEscape(String(cfg.hostname)) + "\",";
  const char *dhcpHost = WiFi.getHostname();
  json += "\"dhcp_hostname\":\"" +
          jsonEscape(dhcpHost != nullptr ? String(dhcpHost) : String("")) + "\",";
  json += "\"mdns_url\":\"http://" + jsonEscape(String(cfg.hostname)) + ".local\",";
  json += "\"ip\":\"" + (isWifiConnected()
                            ? WiFi.localIP().toString()
                            : WiFi.softAPIP().toString()) + "\",";
  json += "\"rssi\":" + String(isWifiConnected() ? WiFi.RSSI() : 0) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"wifi_status\":" + String(WiFi.status()) + ",";
  json += "\"wifi_connected\":" + String(isWifiConnected() ? "true" : "false") + ",";
  json += "\"ap_mode\":" + String(isApModeActive() ? "true" : "false") + ",";
  json += "\"ap_ssid\":\"" + jsonEscape(apSsid()) + "\",";
  json += "\"web_auth\":" + String(webAuthEnabled() ? "true" : "false") + ",";
  json += "\"ota_ready\":" + String(isOtaReady() ? "true" : "false") + ",";
  json += "\"artnet_active\":" + String(artnetActive ? "true" : "false") + ",";
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
  json += "\"dmx_test_mode\":" + String(dmxTestModeActive() ? "true" : "false") + ",";
  json += "\"dmx1_out_traffic\":" + String(isDmx1OutTrafficForWeb() ? "true" : "false") + ",";
  json += "\"dmx2_out_traffic\":" + String(isDmx2OutTrafficForWeb() ? "true" : "false") + ",";
  json += "\"dmx1_in_traffic\":" + String(isDmx1InTrafficActive() ? "true" : "false");
  json += "}";
  return json;
}

static String buildDmxTestJson() {
  uint16_t start = dmxTestStartChannel();
  uint16_t end = start + DMX_TEST_SLIDER_COUNT - 1;

  String json = "{";
  json += "\"test_mode\":" + String(dmxTestModeActive() ? "true" : "false") + ",";
  json += "\"send_on_change_only\":" + String(dmxTestSendOnChangeOnly() ? "true" : "false") + ",";
  json += "\"live_slider_sync\":" + String(dmxTestLiveSliderSync() ? "true" : "false") + ",";
  json += "\"start_channel\":" + String(start) + ",";
  json += "\"sliders\":[";
  for (uint8_t i = 0; i < DMX_TEST_SLIDER_COUNT; i++) {
    if (i > 0) {
      json += ",";
    }
    json += String(dmxTestSliderValue(i));
  }
  json += "],\"range_dump\":\"" + jsonEscape(dmxTestBufferDump(start, end)) + "\"";
  json += "}";
  return json;
}

static void handleDmxTestPage() {
  if (!authenticateWeb()) {
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendPageHead("DMX Test");
  server.sendContent(F("<p class=\"sub\">Manual DMX output · Art-Net ignored while test mode is on</p>"));
  sendNav("dmx-test");

  server.sendContent(F(R"page(
  <section>
    <label class="check-row">
      <input type="checkbox" id="test-mode">
      <span><strong>DMX test mode</strong> — send slider values to DMX1 (Art-Net ignored)</span>
    </label>
    <label class="check-row">
      <input type="checkbox" id="send-on-change" checked>
      <span>Send DMX only when a slider is moved (test mode only)</span>
    </label>
    <label class="check-row">
      <input type="checkbox" id="live-slider-sync">
      <span>Update sliders from Art-Net when test mode is off</span>
    </label>
    <div class="field">
      <label for="start-channel">Start channel (sliders control 16 channels from here)</label>
      <input type="number" id="start-channel" min="1" max="497" step="1" value="1">
    </div>
    <div id="sliders"></div>
    <p class="hint">Sliders load from the last Art-Net frame on page open. With “slider only” checked, DMX sends when you move a slider. Uncheck it to refresh DMX continuously. Enable live sync to mirror incoming Art-Net on the sliders while test mode is off.</p>
    <div class="field">
      <label for="range-dump">Slider range buffer</label>
      <textarea id="range-dump" readonly></textarea>
    </div>
    <div class="field">
      <label for="full-dump">Full DMX buffer (ch 1–512)</label>
      <textarea id="full-dump" readonly></textarea>
    </div>
    <div class="action-row">
      <button type="button" onclick="refreshDump()">Refresh buffer dump</button>
      <span id="test-msg" class="msg"></span>
    </div>
  </section>
  <script>
    var sliderCount = 16;
    var sliderSendTimer = null;
    var liveSyncTimer = null;
    function setMsg(text, kind) {
      var el = document.getElementById('test-msg');
      el.textContent = text;
      el.className = 'msg ' + (kind || '');
    }
    function updateSliderValues(values) {
      for (var i = 0; i < sliderCount; i++) {
        var input = document.querySelector('#sliders input[data-idx="' + i + '"]');
        if (!input || input === document.activeElement) {
          continue;
        }
        input.value = values[i] != null ? values[i] : 0;
        var valEl = document.getElementById('val-' + i);
        if (valEl) {
          valEl.textContent = input.value;
        }
      }
    }
    function buildSliders(startCh, values) {
      var box = document.getElementById('sliders');
      box.innerHTML = '';
      for (var i = 0; i < sliderCount; i++) {
        var ch = startCh + i;
        var row = document.createElement('div');
        row.className = 'slider-row';
        row.innerHTML =
          '<label>Ch ' + ch + '</label>' +
          '<input type="range" min="0" max="255" step="1" data-idx="' + i + '">' +
          '<span class="slider-val" id="val-' + i + '">0</span>';
        box.appendChild(row);
        var input = row.querySelector('input[type="range"]');
        input.value = values[i] != null ? values[i] : 0;
        document.getElementById('val-' + i).textContent = input.value;
        input.addEventListener('input', function(e) {
          var idx = parseInt(e.target.getAttribute('data-idx'), 10);
          document.getElementById('val-' + idx).textContent = e.target.value;
          if (!document.getElementById('test-mode').checked) {
            return;
          }
          clearTimeout(sliderSendTimer);
          sliderSendTimer = setTimeout(function() {
            sendSlider(idx, e.target.value, true);
          }, 40);
        });
        input.addEventListener('change', function(e) {
          if (!document.getElementById('test-mode').checked) {
            return;
          }
          sendSlider(parseInt(e.target.getAttribute('data-idx'), 10), e.target.value, true);
        });
      }
    }
    function appendTestMode(params) {
      params.set('test_mode', document.getElementById('test-mode').checked ? '1' : '0');
      params.set('send_on_change_only', document.getElementById('send-on-change').checked ? '1' : '0');
      params.set('live_slider_sync', document.getElementById('live-slider-sync').checked ? '1' : '0');
    }
    function syncLiveSliderPolling() {
      if (liveSyncTimer) {
        clearInterval(liveSyncTimer);
        liveSyncTimer = null;
      }
      var enabled = document.getElementById('live-slider-sync').checked;
      var testOff = !document.getElementById('test-mode').checked;
      if (!enabled || !testOff) {
        return;
      }
      liveSyncTimer = setInterval(async function() {
        if (document.getElementById('test-mode').checked || !document.getElementById('live-slider-sync').checked) {
          syncLiveSliderPolling();
          return;
        }
        try {
          var res = await fetch('/api/dmx-test?sync=1&ts=' + Date.now(), { cache: 'no-store' });
          var data = await res.json();
          updateSliderValues(data.sliders || []);
          document.getElementById('range-dump').value = data.range_dump || '';
        } catch (e) {}
      }, 500);
    }
    async function loadState() {
      var res = await fetch('/api/dmx-test?init=1&ts=' + Date.now(), { cache: 'no-store' });
      var data = await res.json();
      document.getElementById('test-mode').checked = data.test_mode === true;
      document.getElementById('send-on-change').checked = data.send_on_change_only === true;
      document.getElementById('live-slider-sync').checked = data.live_slider_sync === true;
      document.getElementById('start-channel').value = data.start_channel;
      buildSliders(data.start_channel, data.sliders || []);
      document.getElementById('range-dump').value = data.range_dump || '';
      syncLiveSliderPolling();
    }
    async function refreshDump() {
      var res = await fetch('/api/dmx-buffer');
      document.getElementById('full-dump').value = await res.text();
      var state = await (await fetch('/api/dmx-test')).json();
      document.getElementById('range-dump').value = state.range_dump || '';
    }
    function appendSendMode(params) {
      appendTestMode(params);
    }
    async function postTest(params) {
      try {
        var res = await fetch('/api/dmx-test', { method: 'POST', body: params });
        var data = await res.json();
        if (!res.ok) {
          setMsg(data.error || 'Update failed', 'err');
          return null;
        }
        document.getElementById('range-dump').value = data.range_dump || '';
        return data;
      } catch (e) {
        setMsg('Connection error', 'err');
        return null;
      }
    }
    async function sendSlider(idx, value, quiet) {
      if (!document.getElementById('test-mode').checked) {
        return;
      }
      var params = new URLSearchParams();
      params.set('slider', idx);
      params.set('value', value);
      appendTestMode(params);
      var data = await postTest(params);
      if (data && !quiet) {
        setMsg('Ch ' + (parseInt(document.getElementById('start-channel').value, 10) + idx) +
               ' = ' + value, 'ok');
      }
    }
    document.getElementById('test-mode').addEventListener('change', async function() {
      var params = new URLSearchParams();
      appendTestMode(params);
      var data = await postTest(params);
      if (data) {
        setMsg(this.checked ? 'Test mode on — Art-Net ignored' : 'Test mode off — Art-Net active', 'ok');
        syncLiveSliderPolling();
      }
    });
    document.getElementById('send-on-change').addEventListener('change', async function() {
      var params = new URLSearchParams();
      appendTestMode(params);
      var data = await postTest(params);
      if (data) {
        setMsg(this.checked ? 'DMX send on slider move only' : 'DMX refresh loop enabled', 'ok');
      }
    });
    document.getElementById('live-slider-sync').addEventListener('change', async function() {
      var params = new URLSearchParams();
      appendTestMode(params);
      var data = await postTest(params);
      if (data) {
        setMsg(this.checked ? 'Live slider sync enabled' : 'Live slider sync disabled', 'ok');
        syncLiveSliderPolling();
      }
    });
    document.getElementById('start-channel').addEventListener('change', async function() {
      var params = new URLSearchParams();
      params.set('start_channel', this.value);
      appendSendMode(params);
      var data = await postTest(params);
      if (data) {
        buildSliders(data.start_channel, data.sliders);
        setMsg('Start channel ' + data.start_channel, 'ok');
      }
    });
    loadState().then(refreshDump);
  </script>
  )page"));
  sendCopyrightFooter();
  server.sendContent(F("</body></html>"));
  server.sendContent("");
}

static void handleDmxTestGet() {
  if (!authenticateWeb()) {
    return;
  }
  if (server.hasArg("init") && server.arg("init").toInt() != 0) {
    dmxTestImportSlidersFromArtNet();
  } else if (server.hasArg("sync") && server.arg("sync").toInt() != 0) {
    if (dmxTestLiveSliderSync() && !dmxTestModeActive()) {
      dmxTestImportSlidersFromArtNet();
    }
  }
  server.send(200, "application/json", buildDmxTestJson());
}

static void handleDmxTestPost() {
  if (!authenticateWeb()) {
    return;
  }

  if (server.hasArg("send_on_change_only")) {
    dmxTestSetSendOnChangeOnly(server.arg("send_on_change_only").toInt() != 0);
  }

  if (server.hasArg("live_slider_sync")) {
    dmxTestSetLiveSliderSync(server.arg("live_slider_sync").toInt() != 0);
  }

  if (server.hasArg("test_mode")) {
    dmxTestSetActive(server.arg("test_mode").toInt() != 0);
    if (dmxTestModeActive()) {
      appendLog("DMX test mode enabled (Art-Net ignored)");
    } else {
      appendLog("DMX test mode disabled");
    }
  }

  if (server.hasArg("start_channel")) {
    int start = server.arg("start_channel").toInt();
    if (start < 1 || start > 512 - DMX_TEST_SLIDER_COUNT + 1) {
      server.send(400, "application/json", "{\"error\":\"start_channel out of range\"}");
      return;
    }
    dmxTestSetStartChannel((uint16_t)start);
  }

  if (server.hasArg("slider") && server.hasArg("value")) {
    int idx = server.arg("slider").toInt();
    int value = server.arg("value").toInt();
    if (idx < 0 || idx >= DMX_TEST_SLIDER_COUNT || value < 0 || value > 255) {
      server.send(400, "application/json", "{\"error\":\"slider/value out of range\"}");
      return;
    }
    dmxTestSetSlider((uint8_t)idx, (uint8_t)value);
  }

  server.send(200, "application/json", buildDmxTestJson());
}

static void handleDmxBufferDump() {
  if (!authenticateWeb()) {
    return;
  }
  server.send(200, "text/plain", dmxTestFullBufferDump());
}

void setupWebServer() {
  server.on("/artdmx-bridge32-logo.png", HTTP_GET, handleLogo);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/dmx-test", HTTP_GET, handleDmxTestPage);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/config/download", HTTP_GET, handleConfigDownload);
  server.on("/api/config/upload", HTTP_POST, handleConfigUpload);
  server.on("/api/dmx-test", HTTP_GET, handleDmxTestGet);
  server.on("/api/dmx-test", HTTP_POST, handleDmxTestPost);
  server.on("/api/dmx-buffer", HTTP_GET, handleDmxBufferDump);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/log", HTTP_GET, handleLog);

  server.onNotFound([]() {
    if (!authenticateWeb()) {
      return;
    }
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  appendLog("Web server started on port 80");
}
