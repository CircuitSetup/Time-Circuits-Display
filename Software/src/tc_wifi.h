/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * WiFi and Config Portal handling
 *
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

#include <WiFiManager.h>

extern bool wifiIsOff;
extern bool wifiAPIsOff;
extern bool wifiInAPMode;

void wifi_setup();
void wifi_loop();
void wifiOff();
void wifiOn(unsigned long newDelay = 0, bool alsoInAPMode = false, bool deferConfigPortal = false);
void wifiStartCP();

void updateConfigPortalValues();

int  wifi_getStatus();
bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d);

#endif
