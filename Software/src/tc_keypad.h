/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Keypad handling
 *
 * -------------------------------------------------------------------
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
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

void keypad_setup();
bool scanKeypad();

void resetKeypadState();
void discardKeypadInput();

#ifdef TC_HAVEMQTT
bool injectInput(const char *src);
#endif

#ifdef TC_HAVE_REMOTE
void injectKeypadKey(char key, int kaction);
#endif

void keypad_loop();

void resetTimebufIndices();
void cancelEnterAnim(bool reenableDT = true);
void cancelETTAnim();

bool keypadIsIdle();

void setBeepMode(int mode);
void startBeepTimer();

void allNightmode(bool nm);
void nightModeOn();
void nightModeOff();
int  toggleNightMode();

void alarmOff();
int  alarmOn();

void leds_on();
void leds_off();

void enterkeyScan();

void displayTmrString();
void s5(bool b);

void doCopyAudioFiles();
void start_file_copy();
void file_copy_progress();
void file_copy_done(int err);

void prepareReboot();

extern bool p3anim;

extern int  keypadMode;

extern uint32_t eef;
#define EEF_EnterPressed    0x0001
#define EEF_EnterHeld       0x0002
#define EEF_EttPressed      0x0010
#define EEF_EttHeld         0x0020
#define EEF_EttImmediate    0x0040  // Ignore configured ett delay (also, do lead on speedo-less time travel)
#define EEF_InputInjected   0x0100

extern char timeBuffer[];
extern unsigned int timeBufferSize;

extern char keypadKeyPressed;

#endif
