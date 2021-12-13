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

#ifndef _TC_SETTINGS_H
#define _TC_SETTINGS_H

#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson
#include <FS.h>
#include <SPIFFS.h>

extern void settings_setup();

//default settings - change settings in the web interface 192.168.4.1
struct Settings{
    char ntpServer[32] = "pool.ntp.org";
    char gmtOffset[7] = "-18000"; //US EST
    char daylightOffset[7] = "3600";
    char autoRotateTimes[3] = "1";
    char destTimeBright[3] = "15";
    char presTimeBright[3] = "15";
    char lastTimeBright[3] = "15";
    char beepSound[3] = "0";
};

extern struct Settings settings;

#endif