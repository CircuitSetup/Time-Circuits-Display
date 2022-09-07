/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us 
 * (C) 2022 Thomas Winischhofer (A10001986)
 * 
 * Clockdisplay and keypad menu code based on code by John Monaco
 * Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * -------------------------------------------------------------------
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

#ifndef _TC_WIFI_H
#define _TC_WIFI_H

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#include "tc_global.h"
#include "clockdisplay.h"
#include "tc_menus.h"
#include "tc_time.h"
#include "tc_settings.h"

extern void wifi_setup();
extern void wifi_loop();

void saveParamsCallback();
void saveConfigCallback();
void preSaveConfigCallback();

void setupStaticIP();

extern void updateConfigPortalValues();

extern int wifi_getStatus();
extern bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d);

bool isIp(char *str);
void ipToString(char *str, IPAddress ip);
IPAddress stringToIp(char *str);

void getParam(String name, char *destBuf, size_t length);

#endif
