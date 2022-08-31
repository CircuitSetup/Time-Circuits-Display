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

#ifndef _TC_KEYPAD_H
#define _TC_KEYPAD_H

#include <Arduino.h>
#include <Keypad.h>
#include <OneButton.h>

#include "tc_global.h"
#include "tc_keypadi2c.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"

//#define GTE_KEYPAD // uncomment if using real GTE/TRW keypad control board

#define KEYPAD_ADDR 0x20        // I2C address of the PCF8574 port expander (keypad)

#define ENTER_DEBOUNCE    50    // enter button debounce time in ms
#define ENTER_CLICK_TIME 200    // enter button will register a click 
#define ENTER_HOLD_TIME 2000    // time in ms holding the enter button will count as a hold

#define ETT_DEBOUNCE      50    // external time travel button debounce time in ms
#define ETT_CLICK_TIME   250    // external time travel button will register a click (unused)
#define ETT_HOLD_TIME    200    // external time travel button will register a press (that's the one)

// When ENTER button is pressed, turn off display for this many ms
// Must be sync'd to the sound file used (enter.mp3)
#define BADDATE_DELAY 400
#ifdef TWSOUND
#define ENTER_DELAY   500       // For TW sound files
#else
#define ENTER_DELAY   600         
#endif

#define EE1_DELAY2   3000
#define EE1_DELAY3   2000
#define EE2_DELAY     600
#define EE3_DELAY     500
#define EE4_DELAY    3000

void keypadEvent(KeypadEvent key);
extern void keypad_setup();
extern char get_key();
extern void recordKey(char key);
extern void recordSetTimeKey(char key);
extern void recordSetYearKey(char key);
extern void resetTimebufIndices();
extern void cancelEnterAnim();
extern void cancelETTAnim();

void nightModeOn();
void nightModeOff();

extern char timeBuffer[]; 

void enterKeyPressed();
void enterKeyHeld();
void enterkeytick();

extern bool isEnterKeyPressed;
extern bool isEnterKeyHeld;

#ifdef EXTERNAL_TIMETRAVEL_IN
void ettKeyPressed();
#endif

extern void keypad_loop();

#endif
