/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
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
 * 
 */

#ifndef _TC_MENUS_H
#define _TC_MENUS_H

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#include "tc_global.h"
#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"

#define MODE_DEST 0
#define MODE_PRES 1
#define MODE_DEPT 2
#define MODE_ALRM 3
#define MODE_AINT 4
#define MODE_BRI  5
#define MODE_NET  6
#define MODE_END  7
#define MODE_MIN  MODE_DEST
#define MODE_MAX  MODE_END

#define FIELD_MONTH   0 
#define FIELD_DAY     1 
#define FIELD_YEAR    2 
#define FIELD_HOUR    3
#define FIELD_MINUTE  4

extern void menu_setup();
extern void enter_menu();
void menuSelect(int& number);
void menuShow(int& number);
void setUpdate(uint16_t& number, int field);
void setField(uint16_t& number, uint8_t field, int year, int month);
extern bool loadAlarm();
extern void saveAlarm();
void doSetAlarm();
extern void alarmOff();
extern bool alarmOn();
bool loadAutoInterval();
void saveAutoInterval();
void doSetAutoInterval();
void doSetBrightness(clockDisplay* displaySet);
void doShowNetInfo();
void waitForEnterRelease();
void prepareInput(uint16_t& number);
void waitAudioDone();

extern void animate();
extern void allLampTest();
extern void allOff();

extern bool isSetUpdate;
extern bool isYearUpdate;
extern uint8_t timeout;

extern void mydelay(int mydel);
extern void mysdelay(int mydel);
extern void enterkeytick();
extern void myloop();

#endif
