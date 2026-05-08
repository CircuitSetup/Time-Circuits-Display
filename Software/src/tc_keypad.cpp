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


#include "tc_global.h"

#include <Arduino.h>

#include "input.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"
#include "tc_keypad.h"
#include "tc_settings.h"
#include "tc_wifi.h"

#define KEYPAD_ADDR     0x20    // I2C address of the PCF8574 port expander (keypad)

#define ENTER_DEBOUNCE    50    // enter button debounce time in ms
#define ENTER_PRESS_TIME 200    // enter button will register a short press
#define ENTER_HOLD_TIME 2000    // time in ms holding the enter button will count as a long press

#define ETT_DEBOUNCE      50    // external time travel button debounce time in ms
#define ETT_PRESS_TIME   200    // external time travel button will register a short press
#define ETT_HOLD_TIME   3000    // external time travel button will register a long press

// When ENTER button is pressed, turn off display for this many ms
// Must be sync'd to the sound file used (enter.mp3)
#define BADDATE_DELAY 400
#ifdef CS_EDITION
#define ENTER_DELAY   600
#else
#define ENTER_DELAY   500
#endif

#define SPEC_DELAY   3000
#define TMR_DELAY    5000

#define EE1_DELAY2   3000
#define EE1_DELAY3   2000
#define EE2_DELAY     600
#define EE3_DELAY     500
#define EE4_DELAY    7500
#define EE4_DELAY2    500
#define EE4_DELAY3    500
#define EE4_DELAY4    500

#ifndef TC_JULIAN_CAL
#define EEXSP1 70667637
#define EEXSP2 59572453
#define EEXSP3 97681642
#define EEXSP4 59769333
#define EEXSP5 8765917
#elif !defined(JSWITCH_1582)
#define EEXSP1 119882773
#define EEXSP2 125110405
#define EEXSP3 105941666
#define EEXSP4 124258709
#define EEXSP5 1753125
#else
#define EEXSP1 119882773
#define EEXSP2 125110677
#define EEXSP3 105941666
#define EEXSP4 124258437
#define EEXSP5 1753125
#endif

// BTTFN keystates
enum {
    BTTFN_KP_KS_PRESSED,
    BTTFN_KP_KS_HOLD,
};

static const char keys[4*3] = {
     '1', '2', '3',
     '4', '5', '6',
     '7', '8', '9',
     '*', '0', '#'
};

#ifdef GTE_KEYPAD
static const uint8_t rowPins[4] = {5, 0, 1, 3};
static const uint8_t colPins[3] = {4, 6, 2};
#else
static const uint8_t rowPins[4] = {1, 6, 5, 3};
static const uint8_t colPins[3] = {2, 0, 4};
#endif

static const char *weekDays[7] = {
      "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
};
#ifdef IS_ACAR_DISPLAY
static const char spTxtS1[]  = { 207, 254, 206, 255, 206, 247, 206, 247, 199, 247, 207, 247 };
static const char spTxtS5A[] = { 0x81, 95, 95, 95, 95, 45, 95, 45, 95, 45, 95, 45, 0};
static const char spTxtS5B[] = { 0x81, 95, 95, 95, 45, 95, 45, 95, 45, 95, 45, 95, 0};
#else
static const char spTxtS1[]  = { 181, 244, 186, 138, 187, 138, 179, 138, 179, 131, 179, 139, 179 };
static const char spTxtS5A[] = { 0x81, 95, 0x82, 95, 95, 95, 45, 95, 45, 95, 45, 95, 45, 0};
static const char spTxtS5B[] = { 0x81, 95, 0x82, 95, 95, 45, 95, 45, 95, 45, 95, 45, 95, 0};
#endif
static const char spTxtS2[]  = { 181, 224, 179, 231, 199, 140, 197, 129, 197, 140, 194, 133 };
static const char spTxtS4[]  = { 175, 253, 178, 246, 163, 224, 180, 148, 218, 149, 193 };
static const char spTxtS5[]  = { 190, 253, 169, 252, 189, 241, 189, 228 };
static const char spTxtS6[]  = { 185, 235, 174, 235 };

static char snoozeString[14];

static Keypad_I2C keypad((char *)keys, rowPins, colPins, 4, 3, KEYPAD_ADDR);

int         keypadMode = 0;

uint32_t    eef = 0;

static int  needDepTime = 0;

#ifndef IS_ACAR_DISPLAY
bool        p3anim = false;
#endif

static unsigned long ettNow = 0;
static unsigned long ettDelay = 0; // ms

static unsigned long enterTimerNow = 0;

static unsigned long lastKeyPressed = 0;

#define DATELEN_STPR  14   // 99mmddyyyyHHMM  exh mode: month, day, year, hour, min; also 91/92 for storing dest/last
#define DATELEN_ALL   12   // mmddyyyyHHMM  dt: month, day, year, hour, min
#define DATELEN_REM   10   // 77mmddHHMM    set reminder
#define DATELEN_DATE   8   // mmddyyyy      dt: month, day, year
#define DATELEN_ECMD   7   // xyyyyyy       6-digit command for FC, SID, DG, VSR, REMOTE, AUX
#define DATELEN_QALM   6   // 11HHMM/888xxx 11, hour, min (alarm-set shortcut); 888xxx (mp)
#define DATELEN_INT    5   // xxxxx         reset etc
#define DATELEN_TIME   4   // HHMM          dt: hour, minute; 4xxx timer & servo speedo; 3-9xxx BTTFN commands
#define DATELEN_CODE   3   // xxx           special codes
#define DATELEN_ALSH   2   // 11            show alarm time/wd, etc.
#define DATELEN_CMIN   DATELEN_ALSH     // min length of code entry
#define DATELEN_CMAX   DATELEN_QALM     // max length of code entry
#define DATELEN_MAX    DATELEN_STPR

static char dateBuffer[DATELEN_MAX + 2];
static char binBuf[DATELEN_MAX + 2];
char        timeBuffer[8];
char        keypadKeyPressed = 0;

static int   dateIndex = 0;
static int   timeIndex = 0;
unsigned int timeBufferSize = 2;

static char injectBuffer[DATELEN_MAX + 2];

static bool     doKey = false;
static uint32_t preDTMFkeyplayed = 0;

static unsigned long enterDelay = 0;

static char mp3Track[16];
static char mp3Artist[16];

static TCButton enterKey = TCButton(ENTER_BUTTON_PIN,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor
);

static TCButton ettKey = TCButton(EXTERNAL_TIMETRAVEL_IN_PIN,
    true,     // Button is active LOW
    true      // Enable internal pull-up resistor
);

// File copy progress
static bool          fcprog = false;
static unsigned long fcstart = 0;

static void keypadEvent(char key, KeyState kstate);

static void enterKeyPushedDown();
static void enterKeyPressed();
static void enterKeyHeld();

static void ettKeyPressed();
static void ettKeyHeld();

static void resetEnterAnim();

static void enterPressedPrepare();

static void resetDisplayMode(bool setDep = false);
static void setupWCMode();
static void displayTmrOff();
static void displayRemString(char *buf);
static void displayRemOffString();
static void displayStalePTStatus();
static void displayAlarmOff(bool snooze);
static void doAnddisplayMPNext(int num, char *buf);

static void waitAudioDone();

static int toggleAlarm();
static int getAlarm();

// Short-cuts from tc_menus
extern void dt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR);
extern void pt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR);
extern void lt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR);
extern void dt_off();
extern void pt_off();
extern void lt_off();
extern void dt_on();
extern void pt_on();
extern void lt_on();

static void handleTTrefusal(int reason)
{
    switch(reason) {
    case 1:
        bttfnSendRemCmd(REM_BRAKE);
        break;
    /*
    case 2:
        // Alarm is active - no reaction
        break;
    */
    }
}

static void prepareSpecText(const char *p, char *d, int l)
{            
    d[l] = 0;
    for(int i = l-1; i >= 0; i--) {
        d[i] = p[i] ^ (i == 0 ? 0xff : p[i-1]);
    }
}

void s5(bool b)
{
    dt_showTextDirect(b ? spTxtS5A : spTxtS5B, CDT_CLEAR);
}

/*
 * keypad_setup()
 */
void keypad_setup()
{
    enterKey.begin();

    // Set up the keypad
    keypad.begin(20, ENTER_HOLD_TIME, myCustomDelay_KP);

    keypad.addEventListener(keypadEvent);

    // Set up pin for white LED
    pinMode(WHITE_LED_PIN, OUTPUT);
    digitalWrite(WHITE_LED_PIN, LOW);

    // Set up Enter button
    enterKey.setTiming(ENTER_DEBOUNCE, ENTER_PRESS_TIME, ENTER_HOLD_TIME);
    enterKey.attachPress(enterKeyPressed);
    enterKey.attachLongPressStart(enterKeyHeld);
    enterKey.attachPressStart(enterKeyPushedDown);

    // Set up External time travel button
    if(!ttinpin) {
        ettKey.begin();
        ettKey.setTiming(ETT_DEBOUNCE, ETT_PRESS_TIME, ETT_HOLD_TIME);
        ettKey.attachPress(ettKeyPressed);
        ettKey.attachLongPressStart(ettKeyHeld);
    }

    ettDelay = atoi(settings.ettDelay);
    if(ettDelay > ETT_MAX_DEL) ettDelay = ETT_MAX_DEL;

    dateBuffer[0] = 0;
    timeBuffer[0] = 0;

    #ifdef IS_ACAR_DISPLAY
    sprintf(snoozeString, "SNOOZE    %2s", settings.snoozeTime);
    #else
    sprintf(snoozeString, "SNOOZE     %2s", settings.snoozeTime); 
    #endif
}

/*
 * scanKeypad(): Scan keypad keys
 */
bool scanKeypad()
{
    return keypad.scanKeypad();
}

/*
 *  The keypad event handler
 */
static void keypadEvent(char key, KeyState kstate)
{
    bool mpWasActive = false;
    bool playBad = false;
    int  i;

    if(csf & (CSF_OFF|CSF_ST|CSF_P0|CSF_P1|CSF_RE|CSF_AL|CSF_AE)) {
        doKey = false;
        return;
    }

    pwrNeedFullNow();

    switch(kstate) {
    case TCKS_PRESSED:
        if(key != '#' && key != '*') {
            if(keypadMode != 2) preDTMFkeyplayed = play_keypad_sound(key);
            doKey = true;
        }
        break;
        
    case TCKS_HOLD:
        if(keypadMode) break;    // Don't do anything while in menu

        switch(key) {
        case '0':    // "0" held down -> time travel
            // Complete timeTravel, with speedo, with lead
            if((i = timeTravel(true, true, false))) {
                handleTTrefusal(i);
            }
            break;
        case '9':    // "9" held down -> return from time travel
            if(currentlyOnTimeTravel()) {
                resetPresentTime();
            } else {
                playBad = true;
            }
            break;
        case '1':    // "1" held down -> toggle alarm on/off
            if((i = toggleAlarm()) >= 0) {
                play_file(i ? "/alarmon.mp3" : "/alarmoff.mp3", PA_INTSPKR|PA_CHECKNM|PA_ALLOWSD|PA_DYNVOL);
            } else {
                playBad = true;
            }
            break;
        case '4':    // "4" held down -> toggle night-mode on/off
            manualNightMode = toggleNightMode();
            play_file(manualNightMode ? "/nmon.mp3" : "/nmoff.mp3", PA_INTSPKR|PA_ALLOWSD|PA_DYNVOL);
            manualNMNow = millis();
            break;
        case '3':    // "3" held down -> play audio file "key3.mp3"
        case '6':    // "6" held down -> play audio file "key6.mp3"
            play_key(key - '0', preDTMFkeyplayed);
            break;
        case '7':    // "7" held down -> re-enable/re-connect WiFi
            if(!wifiOnWillBlock()) {
                play_file("/ping.mp3", PA_INTSPKR|PA_CHECKNM|PA_ALLOWSD);
            } else {
                if(haveMusic) mpWasActive = mp_stop();
                play_file("/ping.mp3", PA_INTSPKR|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD);
                waitAudioDone();
            }
            // Enable WiFi / even if in AP mode / with CP
            wifiOn(0, true, false);
            syncTrigger = true;
            syncTriggerNow = millis();
            // Restart mp if it was active before
            if(mpWasActive) mp_play();   
            break;
        case '2':    // "2" held down -> musicplayer prev
            if(haveMusic) mp_prev(mpActive);
            else          playBad = true;
            break;
        case '5':    // "5" held down -> musicplayer start/stop
            if(haveMusic) {
                if(mpActive) {
                    mp_stop();
                } else {
                    mp_play();
                }
            } else playBad = true;
            break;
        case '8':   // "8" held down -> musicplayer next
            if(haveMusic) mp_next(mpActive);
            else          playBad = true;
            break;
        }

        if(playBad) {
            play_file("/baddate.mp3", PA_INTSPKR|PA_CHECKNM|PA_ALLOWSD);
        }

        // When holding a key, the input buffer is cleared
        doKey = false;
        discardKeypadInput();

        break;
        
    case TCKS_RELEASED:
        if(doKey) {
            switch(keypadMode) {
            case 0:
                dateBuffer[dateIndex++] = key;
                dateBuffer[dateIndex] = 0;
                // Don't wrap around, overwrite end of date instead
                if(dateIndex >= DATELEN_MAX) dateIndex = DATELEN_MAX - 1;  
                lastKeyPressed = millisNonZero();
                break;
            case 1:
                timeBuffer[timeIndex++] = key;
                timeBuffer[timeIndex] = 0;
                timeIndex &= (timeBufferSize - 1);
                break;
            case 2:
                keypadKeyPressed = key;
                break;
            }
        }
        break;
    }
}

void resetKeypadState()
{
    doKey = false;
}

void discardKeypadInput()
{
    *dateBuffer = 0;
    dateIndex = 0;
}

/*
 * Callbacks
 */

static void enterKeyPushedDown()
{
    if(csf & CSF_AL) csf |= CSF_AE;
    else             csf &= ~CSF_AE;
}

static void enterKeyPressed()
{
    eef |= EEF_EnterPressed;
    pwrNeedFullNow();
}

static void enterKeyHeld()
{
    eef |= EEF_EnterHeld;
    pwrNeedFullNow();
}

static void ettKeyPressed()
{
    eef |= EEF_EttPressed;
    // Do not touch EEF_EttImmediate here
    pwrNeedFullNow();
}

static void ettKeyHeld()
{
    eef |= EEF_EttHeld;
    pwrNeedFullNow();
}


void resetTimebufIndices()
{
    timeIndex = 0;
    // Do NOT clear the timeBuffer, might be pre-set
}

#ifdef TC_HAVEMQTT
bool injectInput(const char *s) 
{
    int i = 0;

    // No new command until the prev one is done
    if(eef & EEF_InputInjected) return false;

    // status flags (eg CSF_MA) checked in caller function
    
    for( ; *s; ++s) {
        if(*s >= '0' && *s <= '9') injectBuffer[i++] = *s;
        if(i >= DATELEN_MAX) break;
    }
    injectBuffer[i] = 0;
    
    if(!i) return false;

    eef |= EEF_InputInjected;
    return true;
}
#endif

void enterkeyScan()
{
    // scan the enter button
    enterKey.scan(); 

    // scan the ext. time travel button
    #ifdef SERVOSPEEDO
    if(!ttinpin)
    #endif
        ettKey.scan();
}

static int read2digs(unsigned int idx)
{
    return (binBuf[idx] * 10) + binBuf[idx+1];
}

#ifdef TC_HAVE_REMOTE
void injectKeypadKey(char key, int kaction)
{
    if(csf & CSF_MA) return;
    
    if(key == 'E') {
        switch(kaction) {
        case BTTFN_KP_KS_PRESSED:
            // Ignore if physical ENTER button pushed down during Alarm already
            if(!(csf & CSF_AE)) {
                enterKeyPushedDown();   // set/clr CSF_AE depending on CSF_AL
                eef |= EEF_EnterPressed;
                eef &= ~EEF_EnterHeld;
            }
            break;
        case BTTFN_KP_KS_HOLD:
            // We don't enter the keypad menu remotely,
            // but we allow stopping the alarm.
            // Ignore if physical ENTER button pushed down during Alarm already
            if(!(csf & CSF_AE)) {
                if(csf & CSF_AL) {
                    stopAlarm(true);
                    cancelSnooze();
                }
            }
            break;
        }
    } else if(key >= '0' && key <= '9') {
        bool b = doKey;
        doKey = false;
        switch(kaction) {
        case BTTFN_KP_KS_PRESSED:
            keypadEvent(key, TCKS_PRESSED);
            keypadEvent(key, TCKS_RELEASED);
            break;
        case BTTFN_KP_KS_HOLD:
            keypadEvent(key, TCKS_PRESSED);
            keypadEvent(key, TCKS_HOLD);
            keypadEvent(key, TCKS_RELEASED);
            break;
        }
        doKey = b;
    }
}
#endif

/*
 * keypad_loop()
 */
void keypad_loop()
{
    char *keyBuffer = dateBuffer;
    char spTxt[16];

    if(!(eef & EEF_InputInjected)) {
        enterkeyScan();
        if((csf & CSF_OFF) && (csf & CSF_AE)) {
            if(eef & EEF_EnterHeld) {
                stopAlarm(true);
                cancelSnooze();
                csf &= ~CSF_AE;
            } else if(eef & EEF_EnterPressed) {
                stopAlarm(true);
                startSnooze();
                csf &= ~CSF_AE;
            }
        }
    } else {
        keyBuffer = injectBuffer;
    }

    // Discard keypad input after 2 minutes of inactivity
    if(lastKeyPressed && (millis() - lastKeyPressed >= 2*60*1000)) {
        discardKeypadInput();
        lastKeyPressed = 0;
    }

    // Bail out if sequence played or device is fake-"off"
    if(csf & (CSF_OFF|CSF_ST|CSF_P0|CSF_P1|CSF_RE)) {

        if(eef & (EEF_EnterHeld|EEF_EnterPressed)) {
            eef &= ~(EEF_EnterHeld|EEF_EnterPressed);
            csf &= ~CSF_AE; // ?
        }
        eef &= ~(EEF_InputInjected|EEF_EttPressed|EEF_EttHeld|EEF_EttImmediate);

        cancelETTAnim();

        return;
    }

    if(eef & EEF_EttHeld) {
        resetPresentTime();
        eef &= ~(EEF_EttPressed|EEF_EttHeld|EEF_EttImmediate);
    } else if(eef & EEF_EttPressed) {
        int res = 0;
        bool temp = true;
        if(!ettDelay || (eef & EEF_EttImmediate)) {
            // 3rd parameter [forceNoLead]: MQTT/BTTFN-induced with lead; external button without lead
            if((res = timeTravel(true, true, (eef & EEF_EttImmediate) ? false : true))) {
                handleTTrefusal(res);
            }
            ettNow = 0;
        } else if((res = timeTravelProbe(true, temp, true))) {
            handleTTrefusal(res);
        } else {
            ettNow = millisNonZero();
            startBeepTimer();
            send_wakeup_msg();
        }
        eef &= ~(EEF_EttPressed|EEF_EttHeld|EEF_EttImmediate);
    }
    if(ettNow && (millis() - ettNow >= ettDelay)) {
        int res = 0;
        if((res = timeTravel(true, true, true))) {
            handleTTrefusal(res);
        }
        ettNow = 0;
    }

    // If enter key is held, go into keypad menu
    if((!(eef & EEF_InputInjected)) && (eef & EEF_EnterHeld)) {

        eef &= ~(EEF_EnterHeld|EEF_EnterPressed);
        cancelEnterAnim();
        cancelETTAnim();

        if(csf & CSF_AE) {

            csf &= ~CSF_AE;
            stopAlarm(true);
            cancelSnooze();

            // Simulate pressing ENTER
            // cancel ETT, LED on, dest off, init timer
            enterPressedPrepare();
            
            displayAlarmOff(false); // Sets specDisp = 10;
            
        } else {

            timeIndex = 0;
            *timeBuffer = 0;
    
            csf |= CSF_MA;
            bttfn_notify_info();
    
            enter_menu();

            // No enter handling and no external tt while in menu mode,
            // so reset flags upon menu exit
            eef &= ~(EEF_EnterHeld|EEF_EnterPressed|EEF_EttPressed|EEF_EttHeld|EEF_EttImmediate);
    
            csf &= ~CSF_MA;
            bttfn_notify_info();

        }

    }

    // if enter key is merely pressed, handle input
    if(eef & (EEF_EnterPressed|EEF_InputInjected)) {

        int strLen = strlen(keyBuffer);
        int validEntry = 2;
        
        uint16_t enterInterruptsMusic = 0;
        char atxt[16];

        strcpy(binBuf, keyBuffer);
        for(char *s = binBuf; *s ; ++s) {
            *s -= '0';
        }
        
        if(!(eef & EEF_InputInjected)) {
            eef &= ~EEF_EnterPressed;
        }

        // cancel ETT, LED on, dest off, init timer
        enterPressedPrepare();

        if((!(eef & EEF_InputInjected)) && (csf & CSF_AE)) {

            csf &= ~CSF_AE;
            stopAlarm(true);
            displayAlarmOff(startSnooze()); // Sets specDisp = 10;

        } else if(strLen == DATELEN_ALSH) {

            uint16_t flags = 0;
            uint8_t code = atoi(keyBuffer);
            
            if(code == 11) {

                int al = getAlarm();
                if(al >= 0) {
                    const char *alwd = getAlWD(alarmWeekday);
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%-7s %02d%02d", alwd, al >> 8, al & 0xff);
                    #else
                    sprintf(atxt, "%-8s %02d%02d", alwd, al >> 8, al & 0xff);
                    #endif
                    dt_showTextDirect(atxt, CDT_CLEAR|CDT_COLON);
                } else {
                    #ifdef IS_ACAR_DISPLAY
                    dt_showTextDirect("ALARM  UNSET");
                    #else
                    dt_showTextDirect("ALARM   UNSET");
                    #endif
                }
                
                specDisp = 10;
                validEntry = 1;

            } else if(code == 12) {

                if(snoozeRunning()) {
                    dt_showTextDirect("ALARM STOP", CDT_CLEAR);
                    cancelSnooze();
                    specDisp = 10;
                    validEntry = 1;
                }

            } else if(code == 44) {

                if(!ctDown) {
                    displayTmrOff();  // sets specDisp = 10
                } else {
                    displayTmrString();
                    specDisp = 30;
                }

                validEntry = 1;

            } else if(code == 77) {

                uint16_t flags = 0;

                if(!remDay) {
                    displayRemOffString();   // Sets specDisp = 10;
                } else {
                    displayRemString(atxt);  // Sets specDisp = 10;
                }

                validEntry = 1;

            } else if(code == 55) {

                if(haveMusic) {
                    specDisp = 10;
                    if(mpActive) {
                        #ifdef IS_ACAR_DISPLAY
                        sprintf(atxt, "PLAYING  %03d", mp_get_currently_playing());
                        #else
                        sprintf(atxt, "PLAYING   %03d", mp_get_currently_playing());
                        #endif
                        if(*id3artist || *id3track) {
                            strcpy(mp3Artist, *id3artist ? id3artist : "UNKNOWN");
                            strcpy(mp3Track, *id3track ? id3track : "UNKNOWN");
                            specDisp = 20;
                        }
                        dt_showTextDirect(atxt);
                    } else {
                        dt_showTextDirect("STOPPED");
                    }
                    validEntry = 1;
                }

            } else if(code == 33) {
              
                dt_showTextDirect(
                    weekDays[dayOfWeek(presentTime.getDay(), presentTime.getMonth(), presentTime.getYear())]
                );
                specDisp = 10;
                validEntry = 1;

            #ifdef SERVOSPEEDO
            } else if(code == 49) {
                int scorr, tcorr;
                loadServoCorr(scorr, tcorr);
                sprintf(atxt, "S%4d T%4d", scorr, tcorr);
                dt_showTextDirect(atxt);
                specDisp = 10;
                validEntry = 1;
            #endif

            }

        } else if(strLen == DATELEN_CODE) {

            uint16_t code = atoi(keyBuffer);
            bool rcModeState;
            uint16_t flags = 0;

            if((code == 113) && ((wcf & (WCF_HaveRCM|WCF_HaveWCM)) != (WCF_HaveRCM|WCF_HaveWCM))) {
                code = (wcf & WCF_HaveRCM) ? 111 : 112;
            }

            if((code >= 300 && code <= 319) || code == 399) {
                
                curVolume = (code == 399) ? 255 : (code - 300);

                // Re-set RotEnc for current level
                #ifdef TC_HAVE_RE
                re_vol_reset();
                #endif

                saveCurVolume();
                validEntry = 1;
              
            } else if(code >= 501 && code <= 509) {

                play_key(code - 500, 0xffff);

                validEntry = 0;   // Play no sound

            #ifdef TC_HAVEMQTT
            } else if(code >= 600 && code <= 609) {
            
                int idx = code - 600;
                if(settings.mqmt[idx] && settings.mqmm[idx]) {
                    mqttPublish(settings.mqmt[idx], settings.mqmm[idx], strlen(settings.mqmm[idx]));
                }
                
                validEntry = 0;   // Play no sound
            #endif

            } else {
            
                switch(code) {
                case 110:           // 110: Return to default display mode (disable RC, WC, Nav, mini)
                    if(isRcMode() || isWcMode() || isNavMode() || isMiniMode()) {
                        bool needDep = isMiniMode();
                        if(!needDep) {
                            if(isWcMode() && ((wcf & WCF_HaveTZ2) || isRcMode())) needDep = true;
                            #ifdef TC_HAVETEMP
                            else if(isRcMode() && tempSens.haveHum()) needDep = true;
                            #endif
                            #ifdef TC_HAVEGPS
                            else if(isNavMode()) needDep = true;
                            #endif
                        }
                        if(needDep) needDepTime = 1;
                        if(isMiniMode()) enableMiniMode(false);
                        #ifdef TC_HAVEGPS
                        else if(isNavMode()) enableNavMode(false);
                        #endif
                        #ifdef TC_HAVETEMP
                        if(isRcMode()) enableRcMode(false);
                        #endif
                        if(isWcMode()) {
                            enableWcMode(false);
                            setupWCMode();
                        }
                    }
                    validEntry = 1;
                    break;
                #ifdef TC_HAVETEMP
                case 111:               // 111+ENTER: Toggle rc-mode
                    if(wcf & WCF_HaveRCM) {
                        toggleRcMode();
                        if(tempSens.haveHum() || isWcMode()) {
                            needDepTime = 1;
                        }
                        validEntry = 1;
                    }
                    break;
                #endif
                case 112:               // 112+ENTER: Toggle wc-mode
                    if(wcf & WCF_HaveWCM) {
                        toggleWcMode();
                        if((wcf & WCF_HaveTZ2) || isRcMode()) {
                            needDepTime = 1;
                        }
                        setupWCMode();
                        destShowAlt = depShowAlt = 0; // Reset TZ-Name-Animation
                        validEntry = 1;
                    }
                    break;
                case 113:               // 113+ENTER: Toggle rc+wc mode
                    // Dep Time display needed in any case:
                    // Either for TZ2 or TEMP
                    rcModeState = toggleRcMode();
                    enableWcMode(rcModeState);
                    setupWCMode();
                    destShowAlt = depShowAlt = 0; // Reset TZ-Name-Animation
                    needDepTime = 1;
                    validEntry = 1;
                    break;
                #ifdef TC_HAVEGPS
                case 114:
                case 115:
                case 116:
                    if(haveNavMode()) {
                        int dmMode = gpsGetDM();
                        setNavDisplayMode(code - 114);
                        if( !isNavMode() ||
                            (code - 114 == dmMode) ) {
                            toggleNavMode();
                        }
                        needDepTime = 1;
                        validEntry = 1;
                    }
                    break;
                #endif
                case 117:
                    if(isWcMode()) {
                        enableWcMode(false);
                        setupWCMode();
                    }
                    toggleMiniMode();
                    needDepTime = 1;
                    validEntry = 1;
                    break;
                case 222:               // 222+ENTER: Turn shuffle off
                case 555:               // 555+ENTER: Turn shuffle on
                    mp_makeShuffle((code == 555));  // Make regardless of haveMusic to save requested setting
                    if(haveMusic) {
                        #ifdef IS_ACAR_DISPLAY
                        dt_showTextDirect((code == 555) ? "SHUFFLE   ON"  : "SHUFFLE  OFF");
                        #else
                        dt_showTextDirect((code == 555) ? "SHUFFLE    ON" : "SHUFFLE   OFF");
                        #endif
                        specDisp = 10;
                        validEntry = 1;
                    }
                    break;
                case 888:               // 888+ENTER: Goto song #0
                    if(haveMusic) {
                        doAnddisplayMPNext(0, atxt);  // Sets specDisp = 10
                        validEntry = 1;
                    }
                    break;
                case 350:
                case 351:
                    if(haveLineOut) {
                        rcModeState = useLineOut;
                        useLineOut = (code == 351);
                        if(rcModeState != useLineOut) {
                            saveLineOut();
                        }
                        validEntry = 1;
                    }
                    break;
                case 440:
                    ctDown = 0;
                    displayTmrOff();    // Sets specDisp = 10
                    validEntry = 1;
                    break;
                case 770:
                    remMonth = remDay = remHour = remMin = 0;
                    saveReminder();
                    displayRemOffString();    // Sets specDisp = 10
                    validEntry = 1;
                    break;
                case 777:
                    if(!remDay) {
                      
                        displayRemOffString();    // Sets specDisp = 10
                        
                    } else {
                      
                        DateTime dtu, dtl;
                        myrtcnow(dtu);
                        UTCtoLocal(dtu, dtl, 0);
                        int  ry = dtl.year(), rm = remMonth ? remMonth : dtl.month(), rd = remDay, rh = remHour, rmm = remMin;
                        LocalToUTC(ry, rm, rd, rh, rmm, 0);
                        
                        uint32_t locMins = mins2Date(dtu.year(), dtu.month(), dtu.day(), dtu.hour(), dtu.minute());
                        uint32_t tgtMins = mins2Date(ry, rm, rd, rh, rmm);
                        
                        if(tgtMins < locMins) {
                            if(remMonth) {
                                tgtMins = mins2Date(ry + 1, rm, rd, rh, rmm);
                                tgtMins += (365*24*60);
                                if(isLeapYear(ry)) tgtMins += 24*60;
                            } else {
                                if(dtu.month() == 12) {
                                    tgtMins = mins2Date(ry + 1, 1, rd, rh, rmm);
                                    tgtMins += (365*24*60);
                                    if(isLeapYear(ry)) tgtMins += 24*60;
                                } else {
                                    tgtMins = mins2Date(ry, rm + 1, rd, rh, rmm);
                                }
                            }
                        }
                        tgtMins -= locMins;
                        
                        int days = tgtMins / (24*60);
                        tgtMins -= (days*24*60);
                        int hours = tgtMins / 60;
                        int minutes = tgtMins - (hours*60);
    
                        #ifdef IS_ACAR_DISPLAY
                        sprintf(atxt, "    %3dd%2d%02d", days, hours, minutes);
                        #else
                        sprintf(atxt, "     %3dd%2d%02d", days, hours, minutes);
                        #endif
                        dt_showTextDirect(atxt, CDT_CLEAR|CDT_COLON);
                        specDisp = 10;
                    }
                    validEntry = 1;
                    break;
                case 0:
                case 1:
                case 2:
                case 3:
                    setBeepMode(code);
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "BEEP MODE  %1d", beepMode);
                    #else
                    sprintf(atxt, "BEEP MODE   %1d", beepMode);
                    #endif
                    dt_showTextDirect(atxt);
                    specDisp = 10;
                    validEntry = 0;   // Play no sound
                    break;
                case 9:
                    send_refill_msg();
                    validEntry = 0;   // Play no sound
                    break;
                case 900:
                case 901:
                    if(ETTOcommands) {
                        setTTOUTpin((code == 901) ? HIGH : LOW);
                        validEntry = 0;   // Play no sound
                    }
                    break;
                case 990:
                case 991:
                    if(!(eef & EEF_InputInjected)) {
                        rcModeState = carMode;
                        carMode = (code == 991);
                        if(rcModeState != carMode) {
                            saveCarMode();
                            prepareReboot();
                            delay(1000);
                            esp_restart();
                        }
                        validEntry = 1;
                    }
                    break;
                #ifdef TC_HAVE_REMOTE
                case 992:
                case 993:
                    if(!(eef & EEF_InputInjected)) {
                        rcModeState = remoteAllowed;
                        remoteAllowed = (code == 993);
                        if(rcModeState != remoteAllowed) {
                            saveRemoteAllowed();
                            if(!remoteAllowed) {
                                removeRemote();
                                bttfn_notify_info();
                            }
                        }
                        validEntry = 1;
                    }
                    break;
                case 994:
                case 995:
                    if(!(eef & EEF_InputInjected)) {
                        rcModeState = remoteKPAllowed;
                        remoteKPAllowed = (code == 995);
                        if(rcModeState != remoteKPAllowed) {
                            saveRemoteAllowed();
                            if(!remoteKPAllowed) {
                                removeKPRemote();
                                bttfn_notify_info();
                            }
                        }
                        validEntry = 1;
                    }
                    break;
                #endif  // TC_HAVE_REMOTE
                #ifdef TC_HAVEMQTT
                case 996:
                    if(!(eef & EEF_InputInjected)) {
                        mqttFakePowerControl(false);
                        validEntry = 1;
                    }
                    break;
                #endif  // TC_HAVEMQTT
                case 998:
                    pauseAuto();
                    enableRcMode(false);
                    enableWcMode(false);
                    #ifdef TC_HAVEGPS
                    enableNavMode(false);
                    #endif
                    enableMiniMode(false);
                    loadUserDLTimes();
                    backupDestTime();
                    backupLastTime();
                    departedTime.savePending();
                    destinationTime.save();
                    needDepTime = 1;
                    validEntry = 1;
                    break;
                case 999:
                    stalePresent = !stalePresent;
                    displayStalePTStatus();   // Sets specDisp = 10
                    saveStaleTime((void *)&stalePresentTime[0], stalePresent);
                    validEntry = 1;
                    break;
                }
            }

        } else if(strLen == DATELEN_INT) {

            if(!strncmp(keyBuffer, "64738", 5) && (!(eef & EEF_InputInjected))) {
                prepareReboot();
                delay(1000);
                esp_restart();
            } else if(!strncmp(keyBuffer, "53281", 5)) {
                showUpdAvail = !showUpdAvail;
                saveUpdAvail();
                validEntry = 1;
            }

        } else if(strLen == DATELEN_QALM) {

            int code = read2digs(0);

            if(code == 11) {

                int aHour = read2digs(2);
                int aMin = read2digs(4);

                if(aHour <= 23 && aMin <= 59) {
                    const char *alwd = getAlWD(alarmWeekday);
                    alarmHour = aHour;
                    alarmMinute = aMin;
                    alarmOnOff = true;
                    saveAlarm();
                    cancelSnooze();
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%-7s %02d%02d", alwd, alarmHour, alarmMinute);
                    #else
                    sprintf(atxt, "%-8s %02d%02d", alwd, alarmHour, alarmMinute);
                    #endif
                    dt_showTextDirect(atxt, CDT_COLON);
                    specDisp = 10;
                    validEntry = 1;
                }

            } else if(code == 77) {

                int sMon  = read2digs(2);
                int sDay  = read2digs(4);

                if((sMon <= 12) && (sDay >= 1 && sDay <= 31)) {

                    if(!sMon || sDay <= daysInMonth(sMon, 2000)) {

                        remMonth = sMon;
                        remDay = sDay;

                        // If current hr and min are zero
                        // assume unset, set default 9am.
                        if(!remHour && !remMin) remHour = 9;
                        
                        saveReminder();

                        displayRemString(atxt); // Sets specdisp = 10

                        validEntry = 1;
                    }
                }
            
            } else if(haveMusic && !strncmp(keyBuffer, "888", 3)) {

                int num = (binBuf[3] * 100) + read2digs(4);
                
                doAnddisplayMPNext(num, atxt); // Sets specDisp = 10

                validEntry = 1;

            }

        } else if(strLen == DATELEN_TIME && binBuf[0] == 4) {

            int mins;
            uint16_t flags = 0;
            #ifdef SERVOSPEEDO
            int sfact = 1;
            #endif
            
            switch(binBuf[1]) {
            case 4:
                mins = read2digs(2);
                if(!mins) {
                    ctDown = 0;
                    displayTmrOff();  // sets specDisp = 10
                } else {
                    ctDown = mins * 60 * 1000;
                    ctDownNow = millis();
                    displayTmrString();
                    specDisp = 30;
                }
                validEntry = 1;
                break;
            #ifdef SERVOSPEEDO    // Servo speedo calibration:
            case 5:               // 45xx: Set Tacho negative offset (in degrees times 10)
            case 7:               // 46xx: Set Tacho positive offset
                sfact = -1;       // 47xx: Set Speedo negative offset
                // fall through   // 48xx: Set Speedo positive offset
            case 6:
            case 8:
                if(sgf & SGF_USpeedoDisp) {
                    int val = read2digs(2) * sfact;
                    bool doSave;
                    if(binBuf[1] <= 6) {
                        doSave = speedo.setTCorr(val);
                    } else {
                        doSave = speedo.setSCorr(val);
                    }
                    if(doSave) {
                        int scorr, tcorr;
                        speedo.getCorr(scorr, tcorr);
                        saveServoCorr(scorr, tcorr);
                        validEntry = 1;
                    }
                }
                break;
            case 9:     // 49xx: Servo speedo calib mode. 4999 quits, 49xx (xx<=95) sets speed (mph)
                if(sgf & SGF_USpeedoDisp) {
                    int val = read2digs(2);
                    speedo.setCalib((val > 95) ? -1 : val);
                    validEntry = 1;
                }
                break;
            #endif
            }

        } else if((strLen == DATELEN_TIME || strLen == DATELEN_ECMD) && (binBuf[0] >= 3)) {
                      
            uint32_t cmd;
            if(strLen == DATELEN_TIME)
                cmd = (binBuf[1] * 100) + read2digs(2);
            else
                cmd = (read2digs(1) * 10000) + (read2digs(3) * 100) + read2digs(5);

            validEntry = 1;
            switch(binBuf[0]) {
            case 3:
                bttfnSendFluxCmd(cmd);
                break;
            case 5:
                bttfnSendAUXCmd(cmd);
                break;
            case 6:
                bttfnSendSIDCmd(cmd);
                break;
            case 7:
                bttfnSendRemCmd(cmd);
                break;
            case 8:
                bttfnSendVSRCmd(cmd);
                break;
            case 9:
                bttfnSendPCGCmd(cmd);
                break;
            default:
                validEntry = 2;
            }
        
        } else if(strLen == DATELEN_REM) {
           
            if(read2digs(0) == 77) {

                int sMon  = read2digs(2);
                int sDay  = read2digs(4);
                int sHour = read2digs(6);
                int sMin  = read2digs(8);

                if((sMon <= 12) && (sDay >= 1 && sDay <= 31) && (sHour <= 23) && (sMin <= 59)) {

                    if(!sMon || sDay <= daysInMonth(sMon, 2000)) {

                        remMonth = sMon;
                        remDay = sDay;
                        remHour = sHour;
                        remMin = sMin;
                    
                        saveReminder();

                        displayRemString(atxt);   // Sets specDisp = 10

                        validEntry = 1;
                    }
                } 
            } 

        } else if(strLen == DATELEN_STPR) {

            int code = read2digs(0);

            if(code == 91 || code == 92 || code == 99) {
                int tm = read2digs(2);
                int td = read2digs(4);
                int y = ((int)read2digs(6) * 100) + read2digs(8);
                int th, tmi;

                if(tm < 1) tm = 1;
                else if(tm > 12) tm = 12;
                if(td < 1) td = 1;
                else {
                    int tdd = daysInMonth(tm, y);
                    if(td > tdd) td = tdd;
                }
                
                #ifdef TC_JULIAN_CAL
                correctNonExistingDate(y, tm, td);
                #endif
                
                th = read2digs(10);
                if(th > 23) th = 23;
                
                tmi  = read2digs(12);
                if(tmi > 59) tmi = 59;

                switch(code) {
                case 91:
                    // Disable nav&rc&wc&mini modes
                    resetDisplayMode();
                    destinationTime.setFromParms(y, tm, td, th, tmi);
                    destinationTime.copyToUserTimes();
                    destinationTime.save();
                    backupDestTime();
                    pauseAuto();
                    break;
                case 92:
                    // Disable nav&rc&wc&mini modes
                    needDepTime = 1;
                    resetDisplayMode(true);
                    departedTime.setFromParms(y, tm, td, th, tmi);
                    departedTime.copyToUserTimes();
                    departedTime.save();
                    backupLastTime();
                    pauseAuto();
                    break;
                default:
                    stalePresentTime[0].year = y;
                    stalePresentTime[0].month = tm;
                    stalePresentTime[0].day = td;
                    stalePresentTime[0].hour = th;
                    stalePresentTime[0].minute = tmi;
                    memcpy((void *)&stalePresentTime[1], (void *)&stalePresentTime[0], sizeof(dateStruct));
                    stalePresent = true;
                    saveStaleTime((void *)&stalePresentTime[0], stalePresent);
                    displayStalePTStatus();   // Sets specDisp = 10
                }

                validEntry = 1;
            }

        } else if((strLen == DATELEN_TIME) || (strLen == DATELEN_DATE) || (strLen == DATELEN_ALL)) {

            int _setMonth = -1, _setDay = -1, _setYear = -1;
            int _setHour = -1, _setMin = -1, temp1, temp2;
            int special = 0;
            uint32_t spTmp;

            temp1 = read2digs(0);
            temp2 = read2digs(2);
            
            // Convert keyBuffer to date
            if(strLen == DATELEN_TIME) {
                _setHour  = temp1;
                _setMin   = temp2;
            } else {
                _setMonth = temp1;
                _setDay   = temp2;
                _setYear  = (read2digs(4) * 100) + read2digs(6);
                if(strLen == DATELEN_ALL) {
                    _setHour = read2digs(8);
                    _setMin  = read2digs(10);
                }

                // Check month
                if(_setMonth < 1) _setMonth = 1;
                else if(_setMonth > 12) _setMonth = 12;

                // Check day
                if(_setDay < 1) _setDay = 1;
                else if(_setDay > daysInMonth(_setMonth, _setYear)) {
                    // set to max day in that month
                    _setDay = daysInMonth(_setMonth, _setYear); 
                }

                #ifdef TC_JULIAN_CAL
                correctNonExistingDate(_setYear, _setMonth, _setDay);
                #endif

                // Year: There is no year "0", for crying out loud.
                // Having said that, we allow it anyway, let the people have
                // the full movie experience.
                //if(_setYear < 1) _setYear = 1;

                spTmp = (uint32_t)_setYear << 16 | _setMonth << 8 | _setDay;
                if((spTmp ^ getHrs1KYrs(7)) == EEXSP1) {
                    special = 1;
                    prepareSpecText(spTxtS1, spTxt, sizeof(spTxtS1));
                } else if((spTmp ^ getHrs1KYrs(8)) == EEXSP2)  {
                    if(_setHour >= 9 && _setHour <= 12) {
                        special = 2;
                    }
                } else if((spTmp ^ getHrs1KYrs(6)) == EEXSP3)  {
                    special = 3;
                } else if((spTmp ^ getHrs1KYrs(8)) == EEXSP4)  {
                    special = 4;
                }
            }

            // Hour and min are checked in tcddisplay

            // Normal date/time: ENTER-sound interrupts musicplayer
            enterInterruptsMusic = PA_INTRMUS;

            switch(special) {
            case 1:
                dt_showTextDirect(spTxt, CDT_CLEAR|CDT_COLON);
                specDisp = 1;
                validEntry = 1;
                break;
            case 2:
                play_file("/ee2.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS);
                enterDelay = EE2_DELAY;
                validEntry = 0;
                break;
            case 3:
                play_file("/ee3.mp3", PA_INTSPKR|PA_CHECKNM|PA_INTRMUS);
                enterDelay = EE3_DELAY;
                validEntry = 0;
                break;
            case 4:
                destinationTime.clearDisplay();
                destinationTime.onCond();
                play_file("/ee4.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS);
                enterDelay = EE4_DELAY;
                specDisp = 5;
                validEntry = 0;
                break;
            default:
                validEntry = 1;
            }

            // Copy date to destination time
            if(_setYear >= 0) destinationTime.setYear(_setYear);   // ny0: >
            if(_setMonth > 0) destinationTime.setMonth(_setMonth);
            if(_setDay > 0)   destinationTime.setDay(_setDay);
            if(_setHour >= 0) destinationTime.setHour(_setHour);
            if(_setMin >= 0)  destinationTime.setMinute(_setMin);

            backupDestTime();

            // We only save the new time if user wants persistence.
            // Might not be preferred; wears the flash memory.
            if(timetravelPersistent) {
                destinationTime.save();
            }

            // Disable nav&rc&wc&mini modes
            resetDisplayMode();

            // Pause autoInterval-cycling so user can play undisturbed
            pauseAuto();

            // Beep auto mode: Restart timer
            startBeepTimer();

            // Send "wakeup" to network clients
            send_wakeup_msg();

        }

        if(needDepTime) lt_off();

        if(validEntry == 1) {
            play_file("/enter.mp3", PA_INTSPKR|PA_CHECKNM|enterInterruptsMusic|PA_ALLOWSD);
        } else if(validEntry == 2) {
            enterDelay = BADDATE_DELAY;
            specDisp = 0;
            if((!enterInterruptsMusic && mpActive) || isUISignalPlaying()) {
                dt_showTextDirect("ERROR", CDT_CLEAR);
                specDisp = 10;
            } else {
                play_file("/baddate.mp3", PA_INTSPKR|PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD);
            }
        }

        // Prepare for next input
        if(!(eef & EEF_InputInjected)) {
            dateIndex = 0;
            keyBuffer[0] = 0;
        }

        eef &= ~EEF_InputInjected;
        
    }

    // Turn everything back on after entering date
    // (happens in later iteration of loop)

    if(enterTimerNow && (millis() - enterTimerNow > enterDelay)) {

        switch(specDisp) {
        case 0:
            break;
        case 1:
        case 5:
        case 10:
        case 20:
        case 30:
            specDisp++;
            digitalWrite(WHITE_LED_PIN, LOW);
            enterTimerNow = millisNonZero();
            enterDelay = SPEC_DELAY;
            switch(specDisp) {
            case 2:
                destinationTime.onCond();
                enterDelay = EE1_DELAY2;
                break;
            case 6:
                prepareSpecText(spTxtS4, spTxt, sizeof(spTxtS4));
                dt_showTextDirect(spTxt);
                enterDelay = EE4_DELAY2;
                break;
            case 31:
                displayTmrString();
                enterDelay = TMR_DELAY;
                // Fall through
            default:
                destinationTime.resetBrightness(); 
                dt_on();
            }
            break;
        case 2:
            specDisp++;
            prepareSpecText(spTxtS2, spTxt, sizeof(spTxtS2));            
            dt_showTextDirect(spTxt);
            play_file("/ee1.mp3", PA_LINEOUT|PA_CHECKNM|PA_INTRMUS);
            enterTimerNow = millisNonZero();
            enterDelay = EE1_DELAY3;
            break;
        case 6:
        case 7:
            if(specDisp == 6) {
                prepareSpecText(spTxtS5, spTxt, sizeof(spTxtS5));
                enterDelay = EE4_DELAY3;
            } else {
                prepareSpecText(spTxtS6, spTxt, sizeof(spTxtS6)); 
                enterDelay = EE4_DELAY4;
            }
            specDisp++;
            dt_showTextDirect(spTxt);
            enterTimerNow = millisNonZero();
            break;
        case 21:
        case 22:
            dt_showTextDirect(specDisp == 21 ? mp3Track : mp3Artist);
            specDisp++;
            enterTimerNow = millisNonZero();
            enterDelay = SPEC_DELAY;
            break;
        case 3:
        case 8:
        case 11:
        case 23:
        case 31:
            specDisp = 0;
            break;
        default:
            specDisp++;
        }

        if(!specDisp) {

            #ifdef TC_HAVEMQTT
            // We overwrite dest time display here, so restart 
            // MQTT message afterwards.
            if(mqttDisp & MQ_DISP_D) {
                mqttOldDisp &= ~MQ_DISP_D;
                mqttIdx[0] = 0;
            }
            if((mqttDisp & MQ_DISP_L) && needDepTime) {
                mqttOldDisp &= ~MQ_DISP_L;
                mqttIdx[2] = 0;
            }
            #endif

            // Animate display

            // Fill audio buffer, avoid a pause in the actual animation
            audio_loop();

            #ifdef TC_HAVEGPS
            if(isNavMode()) {
                char destDisp[16];
                char depDisp[16];
                gpsMakePos(destDisp, depDisp);
                #ifndef TC_NO_MONTH_ANIM // ------------------
                for(bool i : { true, false }) {
                    destinationTime.showNavDirect(destDisp, i);
                    if(needDepTime) {
                        departedTime.showNavDirect(depDisp, i);
                    }
                    if(i) mydelay(80);
                }
                #else // -------------------------------------
                destinationTime.showNavDirect(destDisp, false);
                if(needDepTime) {
                    departedTime.showNavDirect(depDisp, false);
                    departedTime.onCond();
                }
                destinationTime.onCond();
                #endif // ------------------------------------
            } else {
            #endif
                #ifdef TC_HAVETEMP
                if(isRcMode() && (!isWcMode() || (!(wcf & WCF_HaveTZ1)) || needDepTime)) {

                    #ifndef TC_NO_MONTH_ANIM // -----------------------------------
                    for(bool i : { true, false }) {
                        if(!isWcMode() || (!(wcf & WCF_HaveTZ1))) {
                            destinationTime.showTempDirect(tempSens.readLastTemp(), i);
                        } else {
                            destinationTime.showAnimate(i);
                        }
                        if(needDepTime) {
                            if(isWcMode() && (wcf & WCF_HaveTZ1)) {
                                departedTime.showTempDirect(tempSens.readLastTemp(), i);
                            } else if(!isWcMode() && tempSens.haveHum()) {
                                departedTime.showHumDirect(tempSens.readHum(), i);
                            } else {
                                departedTime.showAnimate(i);
                            }
                        }
                        if(i) mydelay(80);
                    }
                    #else // TC_NO_MONTH_ANIM -------------------------------------
                    if(!isWcMode() || (!(wcf & WCF_HaveTZ1))) {
                        destinationTime.showTempDirect(tempSens.readLastTemp(), false);
                    } else {
                        destinationTime.show();
                    }
                    if(needDepTime) {
                        if(isWcMode() && (wcf & WCF_HaveTZ1)) {
                            departedTime.showTempDirect(tempSens.readLastTemp(), false);
                        } else if(!isWcMode() && tempSens.haveHum()) {
                            departedTime.showHumDirect(tempSens.readHum(), false);
                        } else {
                            departedTime.show();
                        }
                        departedTime.onCond();
                    }
                    destinationTime.onCond();
                    #endif  // TC_NO_MONTH_ANIM -------------------------------------
                  
                } else {
                #endif  // TC_HAVETEMP

                    if(isMiniMode()) {
                        
                        destinationTime.clearDisplay();
                        if(needDepTime) {
                            departedTime.clearDisplay();
                            departedTime.onCond();
                        }
                        destinationTime.onCond();
                        
                    } else {
    
                        #ifndef IS_ACAR_DISPLAY
                        if(p3anim) {
                            for(int i = 0; i < 12; i++) {
                                if(!destinationTime.showAnimate3(i))
                                    break;
                                if(needDepTime) {
                                    departedTime.showAnimate3(i);
                                }
                                mydelay(5);
                            }
                        } else {
                        #endif    // IS_ACAR_DISPLAY
                            #ifndef TC_NO_MONTH_ANIM // ---------------------
                            for(bool i : { true, false }) {
                                destinationTime.showAnimate(i);
                                if(needDepTime) {
                                    departedTime.showAnimate(i);
                                }
                                if(i) mydelay(80);
                            }
                            #else // TC_NO_MONTH_ANIM -----------------------
                            destinationTime.show();
                            if(needDepTime) {
                                departedTime.show();
                                departedTime.onCond();
                            }
                            destinationTime.onCond();
                            #endif  // TC_NO_MONTH_ANIM ---------------------
                        #ifndef IS_ACAR_DISPLAY    
                        }
                        #endif  // IS_ACAR_DISPLAY

                    }
    
                #ifdef TC_HAVETEMP
                }
                #endif
            #ifdef TC_HAVEGPS
            }
            #endif

            // Turn off white LED, reset TZ-Name-Animation & flags
            resetEnterAnim();
        }

    }
}

void cancelEnterAnim(bool reenableDT)
{
    if(enterTimerNow) {

        if(reenableDT) {
            #ifdef TC_HAVEGPS
            char destDisp[16];
            char depDisp[16];
            if(isNavMode()) {
                gpsMakePos(destDisp, depDisp);
                destinationTime.showNavDirect(destDisp, false);
            } else
            #endif
            #ifdef TC_HAVETEMP
            if(isRcMode() && (!isWcMode() || (!(wcf & WCF_HaveTZ1)))) {
                destinationTime.showTempDirect(tempSens.readLastTemp());
            } else
            #endif
            if(isMiniMode())
                destinationTime.clearDisplay();
            else
                destinationTime.show();

            if(needDepTime) {
                #ifdef TC_HAVEGPS
                if(isNavMode()) {
                    departedTime.showNavDirect(depDisp, false);
                } else
                #endif
                #ifdef TC_HAVETEMP
                if(isRcMode()) {
                    if(isWcMode() && (wcf & WCF_HaveTZ1)) {
                        departedTime.showTempDirect(tempSens.readLastTemp());
                    } else if(!isWcMode() && tempSens.haveHum()) {
                        departedTime.showHumDirect(tempSens.readHum());
                    } else {
                        departedTime.show();
                    }
                } else
                #endif
                if(isMiniMode())
                    departedTime.clearDisplay();
                else
                    departedTime.show();

                departedTime.onCond();
            }

            destinationTime.onCond();
        }

        // Turn off white LED, reset TZ-Name-Animation & flags
        // Call this here, it resets needDepTime, still needed above
        resetEnterAnim();
        
        specDisp = 0;
    }
}

static void resetEnterAnim()
{
    enterTimerNow = 0;
    
    digitalWrite(WHITE_LED_PIN, LOW);

    // Reset TZ-Name-Animation
    destShowAlt = depShowAlt = 0;

    needDepTime = 0;
}

void cancelETTAnim()
{
    ettNow = 0;
}

static void enterPressedPrepare()
{        
    cancelETTAnim();

    // Turn on white LED
    digitalWrite(WHITE_LED_PIN, HIGH);

    // Turn off destination time
    dt_off();

    enterTimerNow = millisNonZero();

    enterDelay = ENTER_DELAY;
}

bool keypadIsIdle()
{
    return (!lastKeyPressed);
}

static void resetDisplayMode(bool setDep)
{
    // Reset the red display and disable nav&rc&wc&mini modes if they use it.
    // if setDep is set, reset the yellow display instead, and disable
    // rc/wc/nav/mini it they use it.

    // Mini & Nav use both displays, so disable regardless of display to set

    if(isMiniMode()) {
        enableMiniMode(false);
        needDepTime = 1;
    }

    #ifdef TC_HAVEGPS
    if(isNavMode()) {
        enableNavMode(false);
        needDepTime = 1;
    }
    #endif

    #ifdef TC_HAVETEMP
    bool rcUsesLT = (isRcMode() && (tempSens.haveHum() || isWcMode()));
    #endif

    if(!setDep) {
        // If setting the red display, disable RC mode, and
        // reset yellow display if used for RC or RC+WC mode.
        #ifdef TC_HAVETEMP
        if(rcUsesLT) needDepTime = 1;
        enableRcMode(false);
        #endif

        if(isWcMode() && (wcf & WCF_HaveTZ1)) {
            // If WC mode is enabled and we have a TZ for red display,
            // we need to disable WC mode in order to keep the new dest
            // time on display. When disabling WC mode we need to reset
            // the yellow display as well if we have a TZ for it.
            // We restore the yellow time to either the stored value or 
            // the current auto-int step, otherwise the current yellow WC 
            // time would remain but become stale, which is confusing.
            // If there is no TZ for red display, no need to disable WC
            // mode at this point.
            if(wcf & WCF_HaveTZ2) {
                // Restore backed up time
                restoreLastTime();
                needDepTime = 1;
            }
            enableWcMode(false);
        }
        
    } else {

        // If the caller explicitly wants to reset the yellow
        // display, he needs to set needDepTime to true beforehand.
      
        // If setting the yellow display, check if it used by RC
        // or RC+WC mode, and only disable either one if yes.
        #ifdef TC_HAVETEMP
        if(rcUsesLT) enableRcMode(false);
        #endif

        if(isWcMode() && (wcf & WCF_HaveTZ2)) {
            // As above but vice versa.
            if(wcf & WCF_HaveTZ1) restoreDestTime();
            enableWcMode(false);
        }
        
    }
}

static void setupWCMode()
{
    if(isWcMode()) {
        DateTime dtu;
        myrtcnow(dtu);
        setDatesTimesWC(dtu);
    } else {
        // Restore backed up times
        if(wcf & WCF_HaveTZ1) restoreDestTime();
        if(wcf & WCF_HaveTZ2) restoreLastTime();
    }
}

static void displayTmrOff()
{
    #ifdef IS_ACAR_DISPLAY
    dt_showTextDirect("TIMER    OFF");
    #else
    dt_showTextDirect("TIMER     OFF");
    #endif
    specDisp = 10;
}

void displayTmrString()
{
    char atxt[16];
    int mins = 0, secs = 0;
    unsigned long el = millis() - ctDownNow;
    if(ctDown > el) {
        unsigned long res = ((ctDown - el + 999U) / 1000);
        mins = res / 60;
        secs = res - (mins * 60);
    }
    #ifdef IS_ACAR_DISPLAY
    sprintf(atxt, "TIMER   %02d%02d", mins, secs);
    #else
    sprintf(atxt, "TIMER    %02d%02d", mins, secs);
    #endif
    dt_showTextDirect(atxt, CDT_CLEAR|CDT_COLON);
}

static void displayRemString(char *buf)
{   
    if(remMonth) {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "%02d%02d    %02d%02d", remMonth, remDay, remHour, remMin);
        #else
        sprintf(buf, "%3s%02d    %02d%02d", destinationTime.getMonthString(remMonth), 
                                         remDay, remHour, remMin);
        #endif
    } else {
        #ifdef IS_ACAR_DISPLAY
        sprintf(buf, "  %02d    %02d%02d", remDay, remHour, remMin);
        #else
        sprintf(buf, "   %02d    %02d%02d", remDay, remHour, remMin);
        #endif
    }
    dt_showTextDirect(buf, CDT_CLEAR|CDT_COLON);
    specDisp = 10;
}

static void displayRemOffString()
{
    #ifdef IS_ACAR_DISPLAY
    dt_showTextDirect("REMINDER OFF", CDT_CLEAR);
    #else
    dt_showTextDirect("REMINDER  OFF", CDT_CLEAR);
    #endif
    specDisp = 10;
}

static void displayStalePTStatus()
{
    #ifdef IS_ACAR_DISPLAY
    dt_showTextDirect(stalePresent ? "EXH MODE  ON" : "EXH MODE OFF");
    #else
    dt_showTextDirect(stalePresent ? "EXH MODE   ON" : "EXH MODE  OFF");
    #endif
    specDisp = 10;
}

static void displayAlarmOff(bool snooze)
{
    dt_showTextDirect(snooze ? snoozeString : "ALARM STOP", CDT_CLEAR);
    specDisp = 10;
}

static void doAnddisplayMPNext(int num, char *buf)
{
    num = mp_gotonum(num, mpActive);
    #ifdef IS_ACAR_DISPLAY
    sprintf(buf, "NEXT     %03d", num);
    #else
    sprintf(buf, "NEXT      %03d", num);
    #endif
    dt_showTextDirect(buf);
    specDisp = 10;
}

/*
 * Beep
 */

void setBeepMode(int mode)
{
    bool nb = (mode != beepMode);
    unsigned long now = millis();
    
    switch(mode) {
    case 0:
        muteBeep = true;
        beepMode = 0;
        beepTimer = false;
        break;
    case 1:
        muteBeep = false;
        beepMode = 1;
        beepTimer = false;
        break;
    case 2:
        if(beepMode == 1) {
            beepTimerNow = now;
            beepTimer = true;
        }
        beepMode = 2;
        beepTimeout = BEEPM2_SECS*1000;
        break;
    case 3:
        if(beepMode == 1) {
            beepTimerNow = now;
            beepTimer = true;
        }
        beepMode = 3;
        beepTimeout = BEEPM3_SECS*1000;
        break;
    }
    if(nb) {
        settings.beep[0] = beepMode + '0';
        saveBeepAutoInterval();
    }
}
         
/*
 * Un-mute beep and start beep timer
 */
void startBeepTimer()
{
    if(beepMode >= 2) {
        beepTimer = true;
        beepTimerNow = millis();
        muteBeep = false;
    }

    #if defined(TC_DBG_GEN) && defined(TC_DBGBEEP)
    Serial.printf("startBeepTimer: Beepmode %d BeepTimer %d, BTNow %d, now %d mute %d\n", 
        beepMode, beepTimer, beepTimerNow, millis(), muteBeep);
    #endif
}

/*
 * Night mode
 */

void allNightmode(bool nm)
{
    destinationTime.setNightMode(nm);
    presentTime.setNightMode(nm);
    departedTime.setNightMode(nm);
}

static void setNightMode(bool nm)
{
    allNightmode(nm);
    if(sgf & SGF_USpeedoDisp) speedo.setNightMode(nm);
    bttfn_notify_info();
}

void nightModeOn()
{
    csf |= CSF_NM;
    setNightMode(true);
    leds_off();
    // Expire beep timer
    if(beepMode >= 2) {
        beepTimer = false;
        muteBeep = true;
    }
}

void nightModeOff()
{
    csf &= ~CSF_NM;
    setNightMode(false);
    leds_on();
}

int toggleNightMode()
{
    if(csf & CSF_NM) {
        nightModeOff();
        return 0;
    }
    nightModeOn();
    return 1;
}

/*
 * Alarm
 *
 */

void alarmOff()
{
    alarmOnOff = false;
    cancelSnooze();
    saveAlarm();
}

int alarmOn()
{
    cancelSnooze();
    if(alarmHour <= 23 && alarmMinute <= 59) {
        alarmOnOff = true;
        saveAlarm();
        return 1;
    }
    
    return 0;
}

static int toggleAlarm()
{
    if(alarmOnOff) {
        alarmOff();
        return 0;
    }
    return alarmOn() ? 1 : -1;
}

static int getAlarm()
{
    if(alarmHour <= 23 && alarmMinute <= 59) {
        return (alarmHour << 8) | alarmMinute;
    }
    return -1;
}

/*
 * LEDs (TCD control board 1.3+)
 */
 
void leds_on()
{
    if(!(csf & (CSF_OFF|CSF_NM))) {
        digitalWrite(LEDS_PIN, HIGH);
    }
}

void leds_off()
{
    digitalWrite(LEDS_PIN, LOW);
}

/*
 * Other helpers
 *
 */

void doCopyAudioFiles()
{
    bool delIDfile = false;

    if((!copy_audio_files(delIDfile)) && !FlashROMode) {
        // If copy fails because of a write error, re-format flash FS
        lt_showTextDirect("FORMATTING");
        reInstallFlashFS();
        copy_audio_files(delIDfile);  // Retry copy
    }

    if(delIDfile) {
        delete_ID_file();
    } else {
        delay(3000);
    }
    
    delay(2000);

    prepareReboot();
    delay(1000);
    esp_restart();
}

void start_file_copy()
{
    mp_stop();
    stopAudio();
  
    dt_showTextDirect("INSTALLING");
    pt_showTextDirect("SOUND PACK");
    lt_showTextDirect("PLEASE");
    allOn();
    allresetBrightness();
    
    fcprog = false;
    fcstart = millis();
}

void file_copy_progress()
{
    if(millis() - fcstart >= 1000) {
        lt_showTextDirect(fcprog ? "PLEASE" : "WAIT");
        fcprog = !fcprog;
        fcstart = millis();
    }
}

void file_copy_done(int err)
{
    lt_showTextDirect(err ? "ERROR" : "DONE");
}


void prepareReboot()
{
    mp_stop();
    stopAudio();
    ettoPulseEnd();
    allOff();
    flushDelayedSave();
    if(sgf & SGF_USpeedoDisp) speedo.off();
    destinationTime.resetBrightness();
    dt_showTextDirect("REBOOTING");
    dt_on();
    digitalWrite(WHITE_LED_PIN, LOW);
    unmount_fs();
    delay(ENTER_DELAY + 600);
}

// Wait for audio to finish.
// This is only used before stuff that blocks.
// ATM only used before a (blocking) WiFi reconnect.
static void waitAudioDone()
{
    unsigned long startNow = millis();

    while(!checkAudioDone() && (millis() - startNow < 3000)) {
        mydelay(50);
        wifi_loop();
        audio_loop(); // audio after wifi is always a good idea
    }
}
