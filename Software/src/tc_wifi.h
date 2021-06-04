/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
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

extern void wifi_setup();
extern void wifi_loop();

void saveParamsCallback();

extern const char* getNTPServer();
extern const char* getBrightness(const char* display);
extern String getParam(String name);

#endif