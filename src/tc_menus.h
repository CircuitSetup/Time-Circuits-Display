/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
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

#include <Arduino.h>
#include <Wire.h>

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"

#define buttonDown 16          // Button SET MENU'
#define buttonUp 17            // Button +
//#define ENTER_BUTTON 15           // Button -
#define maxTime 240            // maximum time in seconds before timout during setting and no button pressed, max 255 seconds

extern uint8_t timeout;

extern void menu_setup();
extern void enter_menu();
void displayHighlight(int& number);
void displaySelect(int& number);
void setUpdate(uint16_t& number, int field);
void brtButtonUpDown(clockDisplay* displaySet);
void enterNext(uint16_t& number, uint8_t field, int year, int month);
bool loadAutoInterval();
void saveAutoInterval();
void autoTimesButtonUpDown();
void doGetBrightness(clockDisplay* displaySet);
void waitForButtonSetRelease();

extern void animate();
extern void allLampTest();
extern void allOff();

#endif