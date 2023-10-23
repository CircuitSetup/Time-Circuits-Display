/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.backtothefutu.re
 *
 * Keypad handling
 *
 * -------------------------------------------------------------------
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _TC_KEYPAD_H
#define _TC_KEYPAD_H

extern bool isEnterKeyPressed;
extern bool isEnterKeyHeld;
#ifdef EXTERNAL_TIMETRAVEL_IN
extern bool isEttKeyPressed;
extern bool isEttKeyHeld;
extern bool isEttKeyImmediate;
#endif
extern bool menuActive;

extern char timeBuffer[];

void keypad_setup();
bool scanKeypad();

void resetKeypadState();

void keypad_loop();

void resetTimebufIndices();
void cancelEnterAnim(bool reenableDT = true);
void cancelETTAnim();

bool keypadIsIdle();

void setBeepMode(int mode);
void startBeepTimer();

void nightModeOn();
void nightModeOff();
bool toggleNightMode();

void leds_on();
void leds_off();

void enterkeyScan();

#endif
