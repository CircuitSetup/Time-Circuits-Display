/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Keypad Menu handling
 *
 * Based on code by John Monaco, Marmoset Electronics
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

#ifndef _TC_MENUS_H
#define _TC_MENUS_H

extern bool isSetUpdate;
extern bool isYearUpdate;

extern uint8_t        autoInterval;
extern const uint8_t  autoTimeIntervals[6];

extern bool    alarmOnOff;
extern uint8_t alarmHour;
extern uint8_t alarmMinute;
extern uint8_t alarmWeekday;

void menu_setup();
void enter_menu();

bool loadAlarm();
void saveAlarm();

void alarmOff();
bool alarmOn();
bool loadAutoInterval();

void waitAudioDone();

void animate();
void allLampTest();
void allOff();

void start_file_copy();
void file_copy_progress();
void file_copy_done();
void file_copy_error();

void mydelay(unsigned long mydel);
void enterkeyScan();

#endif
