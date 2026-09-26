/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Sound handling
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

#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

// By default, use the volume knob
#define DEFAULT_VOLUME 255
#define VOL_LEVELS 21

#define PA_CHECKNM 0x0000001
#define PA_INTRMUS 0x0000002
#define PA_ALLOWSD 0x0000004
#define PA_DYNVOL  0x0000008
#define PA_DOID3TS 0x0000010
#define PA_DOOR    0x0000020
#define PA_LINEOUT 0x0000040
#define PA_INTSPKR 0x0000000
#define PA_ISWAV   0x0000080
// upper 8 bits all taken for key ID
#define PA_KEYMASK 0x001ff00
#define PA_DOORL   0x0000100
#define PA_DOORR   0x0000200
#define PA_LOOP    0x0020000
#define PA_TCSEGS  0x0040000
#define PA_SIGNAL  0x0100000
#define PA_ALARM   0x0200000
#define PA_REM     0x0400000
#define PA_TMR     0x0800000
#define PA_MUSIC   0x1000000
#define PA_SIGMASK (PA_SIGNAL|PA_ALARM|PA_REM|PA_TMR)

void  audio_setup();
void  audio_loopup_files();
void  audio_loop();
void  audio_loop_quick();

//void     append_file(const char *audio_file, uint32_t flags, float volumeFactor = 1.0f);

void     play_file(const char *audio_file, uint32_t flags, float volumeFactor = 1.0f);
uint32_t play_keypad_sound(char key);
void     play_hour_sound(int hour);
void     play_beep();
void     play_key(int k, uint32_t preDTMFkp);
void     play_door_snd(int doorNum, int state, uint32_t doorFlags);
void     play_ts_snd(int16_t *s);

bool     say_time(int pbt, int whichone, int gh, int gm);

void     setBeepLevel(unsigned int levelIdx);

bool     check_file_SD(const char *audio_file);
void     checkForTCC();

int      getSWVolFromHWVol();

bool     checkAudioDone();
bool     checkAudioFree();
bool     checkMP3Running();
uint32_t isSignalPlaying();
bool     isUISignalPlaying();

void  stopAudio();
void  stop_key();
void  stopAlarm(bool force);

void  mp_init(bool isSetup = false);
void  mp_play(bool forcePlay = true);
bool  mp_stop(bool forceStatus = false);
void  mp_next(bool forcePlay = false);
void  mp_prev(bool forcePlay = false);
int   mp_gotonum(int num, bool force = false);
void  mp_makeShuffle(bool enable);
int   mp_checkForFolder(int num);
int   mp_get_currently_playing();
#ifdef HAVE_MQTT
void  mp_sendStatus(int force = 0);
#endif

typedef struct {
    int state;
    int curVolume;
    int curTrack;
    int maxMusic;
    int mpShuffle;
} Aud_State;
extern Aud_State aud_state;

extern int  volumePin;

extern bool audioInitDone;
extern bool muteBeep;

extern bool mpActive;

extern int8_t mfstatus[];

extern char id3artist[];
extern char id3track[]; 

extern bool haveLineOut;
extern bool useLineOut;

#define TCC_VER 2
extern bool haveTCC;

extern unsigned int beepLvlIdx;

#endif
