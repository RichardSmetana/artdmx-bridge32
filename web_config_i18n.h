#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later
// Client-side i18n for the /config web page (English + German).

static const char CONFIG_PAGE_I18N_SCRIPT[] PROGMEM = R"js(
var I18N = {
  en: {
    page_title: "Config",
    page_sub: "Settings stored in flash · reboot after DMX1 input mode changes",
    nav_dashboard: "Dashboard",
    nav_config: "Config",
    nav_dmx_test: "DMX Test",
    section_device: "Device",
    section_wifi: "WiFi",
    section_ota: "OTA",
    section_artnet: "ArtNet / DMX",
    section_backup: "Backup / restore",
    section_danger: "Danger zone",
    label_ui_lang: "Interface language",
    lang_auto: "Autodetect",
    lang_en: "English",
    lang_de: "German",
    label_hostname: "Hostname (mDNS, OTA, web login name)",
    label_web_pass: "Web UI password (leave empty to disable)",
    label_wifi_ssid: "SSID",
    label_wifi_pass: "Password",
    label_ota_pass: "OTA password (leave empty to disable)",
    label_artnet_universe: "ArtNet universe",
    label_artnet_timeout: "ArtNet timeout (ms)",
    label_dmx_refresh: "DMX refresh interval (ms)",
    field_send_full_packet: "Send full 512-channel DMX packets",
    field_enable_dmx2: "Enable DMX2 output (mirrors ArtNet to GPIO 19/18/21)",
    field_enable_dmx_input: "Enable DMX1 wired input (disables DMX2 output)",
    label_filter_start: "Channel filter start (1–512)",
    label_filter_end: "Channel filter end (1–512)",
    hint_port2: "DMX1 input: forwards only this range to DMX1 out. DMX2 output: sends only this range, and only when a value in the range changes.",
    label_artnet_debug_mode: "ArtNet console log",
    debug_off: "Off",
    debug_matched: "Matching universe only",
    debug_ignored: "Other universes only (ignored)",
    debug_all: "All universes",
    label_debug_ch_start: "Debug log channel start (1–512)",
    label_debug_ch_end: "Debug log channel end (1–512)",
    label_debug_every: "Log every Nth packet (1 = every packet)",
    field_debug_on_change: "Log only when debug channel values change",
    hint_debug_log: "Logged lines show DMX values for the debug channel range (e.g. 3–7). With “values change” enabled, identical frames are not logged.",
    hint_backup: "Download all permanent settings as a text file (artdmx-bridge32.conf), edit in any text editor, then upload. Syntax errors or illegal values are rejected and existing settings are left unchanged.",
    btn_download: "Download .conf",
    label_upload_text: "Or paste configuration text",
    placeholder_upload: "# Paste artdmx-bridge32.conf contents here",
    btn_upload: "Upload",
    btn_upload_reboot: "Upload and Reboot",
    hint_danger: "Erases all saved settings (WiFi, hostname, DMX options) and reboots with factory defaults.",
    btn_factory_reset: "Factory reset",
    btn_save: "Save",
    btn_save_reboot: "Save and Reboot",
    btn_reboot: "Reboot",
    hint_footer_before: "WiFi and hostname changes take full effect after reboot. DMX2 output and DMX1 input mode changes also require reboot. DMX1 wired input requires the esp_dmx uart.c UART2 patch (see README). Hold GPIO 15 LOW for 3 seconds to erase all settings and restore defaults. If WiFi is unavailable, connect to the ",
    hint_footer_mid: " access point and open the device IP. When on your network, use ",
    hint_footer_after: ".",
    msg_could_not_read: "Could not read file",
    msg_choose_file: "Choose a file or paste configuration text",
    msg_upload_failed: "Upload failed",
    msg_uploaded: "Uploaded",
    msg_rebooting: "Rebooting...",
    msg_save_failed: "Save failed",
    msg_saved: "Saved",
    msg_saved_reboot: "Saved, rebooting",
    msg_saved_reboot_hint: "Saved. Reboot recommended for WiFi/hostname or port 2 mode changes.",
    msg_conn_error: "Connection error",
    msg_factory_reset: "Factory reset...",
    msg_factory_confirm: "Are you sure? This erases all saved settings and reboots the device.",
    msg_device_rebooting: "Device rebooting"
  },
  de: {
    page_title: "Konfiguration",
    page_sub: "Einstellungen im Flashspeicher · Neustart nach Änderung des DMX1-Eingangsmodus",
    nav_dashboard: "Dashboard",
    nav_config: "Konfiguration",
    nav_dmx_test: "DMX-Test",
    section_device: "Gerät",
    section_wifi: "WiFi",
    section_ota: "OTA",
    section_artnet: "ArtNet / DMX",
    section_backup: "Sicherung / Wiederherstellung",
    section_danger: "Gefahrenzone",
    label_ui_lang: "Oberflächensprache",
    lang_auto: "Automatisch",
    lang_en: "Englisch",
    lang_de: "Deutsch",
    label_hostname: "Hostname (mDNS, OTA, Web-Loginname)",
    label_web_pass: "Web-Passwort (leer lassen zum Deaktivieren)",
    label_wifi_ssid: "SSID",
    label_wifi_pass: "Passwort",
    label_ota_pass: "OTA-Passwort (leer lassen zum Deaktivieren)",
    label_artnet_universe: "ArtNet-Universum",
    label_artnet_timeout: "ArtNet-Timeout (ms)",
    label_dmx_refresh: "DMX-Refresh-Intervall (ms)",
    field_send_full_packet: "Volle 512-Kanal-DMX-Pakete senden",
    field_enable_dmx2: "DMX2-Ausgang aktivieren (spiegelt ArtNet auf GPIO 19/18/21)",
    field_enable_dmx_input: "Kabelgebundenen DMX1-Eingang aktivieren (deaktiviert DMX2-Ausgang)",
    label_filter_start: "Kanalfilter Start (1–512)",
    label_filter_end: "Kanalfilter Ende (1–512)",
    hint_port2: "DMX1-Eingang: leitet nur diesen Bereich an DMX1-Ausgang weiter. DMX2-Ausgang: sendet nur diesen Bereich und nur bei Wertänderung im Bereich.",
    label_artnet_debug_mode: "ArtNet-Konsolenlog",
    debug_off: "Aus",
    debug_matched: "Nur passendes Universum",
    debug_ignored: "Nur andere Universen (ignoriert)",
    debug_all: "Alle Universen",
    label_debug_ch_start: "Debug-Log Kanal Start (1–512)",
    label_debug_ch_end: "Debug-Log Kanal Ende (1–512)",
    label_debug_every: "Jedes N-te Paket loggen (1 = jedes Paket)",
    field_debug_on_change: "Nur bei Änderung der Debug-Kanalwerte loggen",
    hint_debug_log: "Logzeilen zeigen DMX-Werte für den Debug-Kanalbereich (z. B. 3–7). Mit „nur bei Änderung“ werden identische Frames nicht geloggt.",
    hint_backup: "Alle dauerhaften Einstellungen als Textdatei (artdmx-bridge32.conf) herunterladen, in einem Texteditor bearbeiten und hochladen. Syntaxfehler oder ungültige Werte werden abgelehnt; bestehende Einstellungen bleiben unverändert.",
    btn_download: ".conf herunterladen",
    label_upload_text: "Oder Konfigurationstext einfügen",
    placeholder_upload: "# artdmx-bridge32.conf hier einfügen",
    btn_upload: "Hochladen",
    btn_upload_reboot: "Hochladen und neu starten",
    btn_factory_reset: "Werkseinstellungen",
    hint_danger: "Löscht alle gespeicherten Einstellungen (WiFi, Hostname, DMX-Optionen) und startet mit Werkseinstellungen neu.",
    btn_save: "Speichern",
    btn_save_reboot: "Speichern und neu starten",
    btn_reboot: "Neustart",
    hint_footer_before: "WiFi- und Hostname-Änderungen werden erst nach Neustart wirksam. DMX2-Ausgang und DMX1-Eingangsmodus erfordern ebenfalls einen Neustart. Kabelgebundener DMX1-Eingang benötigt den esp_dmx uart.c UART2-Patch (siehe README). GPIO 15 für 3 Sekunden auf LOW halten, um alle Einstellungen zu löschen. Wenn WiFi nicht verfügbar ist, mit ",
    hint_footer_mid: " verbinden und Geräte-IP öffnen. Im Netzwerk: ",
    hint_footer_after: " verwenden.",
    msg_could_not_read: "Datei konnte nicht gelesen werden",
    msg_choose_file: "Datei wählen oder Konfigurationstext einfügen",
    msg_upload_failed: "Upload fehlgeschlagen",
    msg_uploaded: "Hochgeladen",
    msg_rebooting: "Neustart...",
    msg_save_failed: "Speichern fehlgeschlagen",
    msg_saved: "Gespeichert",
    msg_saved_reboot: "Gespeichert, Neustart",
    msg_saved_reboot_hint: "Gespeichert. Neustart empfohlen bei WiFi-/Hostname- oder Port-2-Änderungen.",
    msg_conn_error: "Verbindungsfehler",
    msg_factory_reset: "Werkseinstellungen...",
    msg_factory_confirm: "Wirklich fortfahren? Alle gespeicherten Einstellungen werden gelöscht und das Gerät neu gestartet.",
    msg_device_rebooting: "Gerät startet neu"
  }
};

var activeLang = "en";

function detectBrowserLang() {
  var nav = (navigator.language || navigator.userLanguage || "en").toLowerCase();
  return nav.indexOf("de") === 0 ? "de" : "en";
}

function resolveLang(pref) {
  if (pref === 1) return "en";
  if (pref === 2) return "de";
  return detectBrowserLang();
}

function t(key) {
  var pack = I18N[activeLang] || I18N.en;
  return pack[key] || I18N.en[key] || key;
}

function applyI18n(lang) {
  activeLang = lang;
  document.documentElement.lang = lang;
  document.querySelectorAll("[data-i18n]").forEach(function(el) {
    var key = el.getAttribute("data-i18n");
    if (key) el.textContent = t(key);
  });
  var upload = document.getElementById("config-upload-text");
  if (upload) upload.placeholder = t("placeholder_upload");
  var host = document.getElementById("hostname");
  var hostName = host && host.value ? host.value : "hostname";
  document.title = hostName + " · " + t("page_title");
}

function setLangFromPref(pref, applyNow) {
  activeLang = resolveLang(pref);
  if (applyNow) applyI18n(activeLang);
}
)js";
