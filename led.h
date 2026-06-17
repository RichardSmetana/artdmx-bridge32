#pragma once
// artdmx-bridge32 | Copyright (C) 2026 Richard Smetana | GPL-3.0-or-later

void initLeds();
void initDmx2Led();
void updateStatusLed();
void disableDmxTrafficLeds();
void blinkDmx1OutLed();
void blinkDmx2OutLed();
void blinkDmx1InLed();
void touchDmx1OutTraffic();
void touchDmx2OutTraffic();
void touchDmx1InTraffic();
void updateLeds();
bool isDmx1OutTrafficActive();
bool isDmx2OutTrafficActive();
bool isDmx1InTrafficActive();
bool isDmx1OutTrafficForWeb();
bool isDmx2OutTrafficForWeb();
