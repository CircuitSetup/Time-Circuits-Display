/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * http://tcd.backtothefutu.re
 *
 * Sound handling
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

#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

extern bool audioInitDone;
extern bool audioMute;

extern bool muteBeep;

extern uint8_t curVolume;

extern bool haveMusic;
extern bool mpActive;
extern bool haveId3;
extern char id3[];

void audio_setup();
void play_keypad_sound(char key);
void play_hour_sound(int hour);
void play_beep();
void audio_loop();
void play_file(const char *audio_file, uint16_t flags, float volumeFactor = 1.0);
bool check_file_SD(const char *audio_file);
bool checkAudioDone();
bool checkMP3Done();
void stopAudio();
void decodeID3(char *artist, char *track);

void mp_init(bool isSetup = false);
void mp_play(bool forcePlay = true);
bool mp_stop();
void mp_next(bool forcePlay = false);
void mp_prev(bool forcePlay = false);
int  mp_gotonum(int num, bool force = false);
void mp_makeShuffle(bool enable);
int  mp_checkForFolder(int num);
int  mp_get_currently_playing();

// By default, use the volume knob
#define DEFAULT_VOLUME 255

#define PA_CHECKNM 0x0001
#define PA_INTRMUS 0x0002
#define PA_ALLOWSD 0x0004
#define PA_DYNVOL  0x0008
#define PA_NOID3TS 0x0010
#define PA_LOOPNOW 0x0020

#endif
