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

#ifndef _TC_KEYPAD_H
#define _TC_KEYPAD_H

#include <Arduino.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <OneButton.h>

#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"

#define KEYPAD_ADDR 0x20
#define WHITE_LED 17 //GPIO that white led is connected to
#define ENTER_BUTTON 16 //GPIO that enter key is connected to
#define ENTER_DELAY 400 //when enter key is pressed, turn off display for this many ms
#define DEBOUNCE 50 //button debounce time in ms
#define ENTER_HOLD_TIME 2000 //time in ms holding the enter key will count as a hold
#define ENTER_DOUBLE_TIME 200 //enter key will register a double click if pressed twice within this time

void keypadEvent(KeypadEvent key);
extern void keypad_setup();
extern char get_key();
extern void recordKey(char key);
extern void recordSetTimeKey(char key);
extern void recordSetYearKey(char key);

void enterKeyPressed();
void enterKeyHeld();
void enterKeyDouble();
extern bool isEnterKeyPressed;
extern bool isEnterKeyHeld;
extern bool isEnterKeyDouble;
extern void keypadLoop();
//extern byte hold;
extern char key;
extern bool keyPressed;
extern bool menuFlag;

#endif