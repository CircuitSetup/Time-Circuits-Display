/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Keypad handling
 *
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#include "tc_global.h"

#include <Arduino.h>

#include "input.h"
#include "tc_menus.h"
#include "tc_audio.h"
#include "tc_time.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_settings.h"
#include "tc_time.h"
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

static const char keys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

#ifdef GTE_KEYPAD
static const uint8_t rowPins[4] = {5, 0, 1, 3};
static const uint8_t colPins[3] = {4, 6, 2};
#else
static const uint8_t rowPins[4] = {1, 6, 5, 3};
static const uint8_t colPins[3] = {2, 0, 4};
#endif

static Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, KEYPAD_ADDR);

bool isEnterKeyPressed = false;
bool isEnterKeyHeld    = false;
static bool enterWasPressed = false;

static bool rcModeDepTime = false;

#ifdef EXTERNAL_TIMETRAVEL_IN
bool                 isEttKeyPressed = false;
bool                 isEttKeyHeld = false;
static unsigned long ettNow = 0;
static bool          ettDelayed = false;
static unsigned long ettDelay = 0; // ms
static bool          ettLong = DEF_ETT_LONG;
#endif

static unsigned long timeNow = 0;

static unsigned long lastKeyPressed = 0;

#define DATELEN_ALL   12   // mmddyyyyHHMM  dt: month, day, year, hour, min
#define DATELEN_DATE   8   // mmddyyyy      dt: month, day, year
#define DATELEN_QALM   6   // 11HHMM/888xxx 11, hour, min (alarm-set shortcut); 888xxx (mp)
#define DATELEN_INT    5   // xxxxx         reset
#define DATELEN_TIME   4   // HHMM          dt: hour, minute
#define DATELEN_CODE   3   // xxx           special codes
#define DATELEN_ALSH   2   // 11            show alarm time/wd
#define DATELEN_CMIN   DATELEN_ALSH
#define DATELEN_CMAX   DATELEN_QALM

static char dateBuffer[DATELEN_ALL + 2];
char        timeBuffer[8];

static int dateIndex = 0;
static int timeIndex = 0;
static int yearIndex = 0;

static bool doKey = false;

static unsigned long enterDelay = 0;

static TCButton enterKey = TCButton(ENTER_BUTTON_PIN,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor
);

#ifdef EXTERNAL_TIMETRAVEL_IN
static TCButton ettKey = TCButton(EXTERNAL_TIMETRAVEL_IN_PIN,
    true,     // Button is active LOW
    true      // Enable internal pull-up resistor
);
#endif

static void keypadEvent(char key, KeyState kstate);
static void recordKey(char key);
static void recordSetTimeKey(char key);
static void recordSetYearKey(char key);

static void enterKeyPressed();
static void enterKeyHeld();

#ifdef EXTERNAL_TIMETRAVEL_IN
static void ettKeyPressed();
static void ettKeyHeld();
#endif

static void mykpddelay(unsigned int mydel);

/*
 * keypad_setup()
 */
void keypad_setup()
{
    // Set up the keypad
    keypad.begin();

    keypad.addEventListener(keypadEvent);

    // Set custom delay function
    // Called between i2c key scan iterations
    // (calls audio_loop() while waiting)
    keypad.setCustomDelayFunc(mykpddelay);

    keypad.setScanInterval(20);
    keypad.setHoldTime(ENTER_HOLD_TIME);

    // Set up pin for white LED
    pinMode(WHITE_LED_PIN, OUTPUT);
    digitalWrite(WHITE_LED_PIN, LOW);

    // Set up Enter button
    enterKey.setPressTicks(ENTER_PRESS_TIME);
    enterKey.setLongPressTicks(ENTER_HOLD_TIME);
    enterKey.setDebounceTicks(ENTER_DEBOUNCE);
    enterKey.attachPress(enterKeyPressed);
    enterKey.attachLongPressStart(enterKeyHeld);

#ifdef EXTERNAL_TIMETRAVEL_IN
    // Set up External time travel button
    ettKey.setPressTicks(ETT_PRESS_TIME);
    ettKey.setLongPressTicks(ETT_HOLD_TIME);
    ettKey.setDebounceTicks(ETT_DEBOUNCE);
    ettKey.attachPress(ettKeyPressed);
    ettKey.attachLongPressStart(ettKeyHeld);

    ettDelay = (int)atoi(settings.ettDelay);
    if(ettDelay > ETT_MAX_DEL) ettDelay = ETT_MAX_DEL;

    ettLong = ((int)atoi(settings.ettLong) > 0);
#endif

    dateBuffer[0] = '\0';
    timeBuffer[0] = '\0';
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
    
    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP0 || timeTravelP1)
        return;

    pwrNeedFullNow();

    switch(kstate) {
    case TCKS_PRESSED:
        if(key != '#' && key != '*') {
            play_keypad_sound(key);
            doKey = true;
        }
        break;
        
    case TCKS_HOLD:
        if(isSetUpdate) break;    // Don't do anything while in menu

        switch(key) {
        case '0':    // "0" held down -> time travel
            doKey = false;
            // Complete timeTravel, with speedo
            timeTravel(true, true);
            break;
        case '9':    // "9" held down -> return from time travel
            doKey = false;
            resetPresentTime();
            break;
        case '1':    // "1" held down -> toggle alarm on/off
            doKey = false;
            switch(toggleAlarm()) {
            case -1:
                playBad = true;
                break;
            case 0:
                play_file("/alarmoff.mp3", 1.0, true, false);
                break;
            case 1:
                play_file("/alarmon.mp3", 1.0, true, false);
                break;
            }
            break;
        case '4':    // "4" held down -> toggle night-mode on/off
            doKey = false;
            if(toggleNightMode()) {
                manualNightMode = 1;
                play_file("/nmon.mp3", 1.0, false, false);
            } else {
                manualNightMode = 0;
                play_file("/nmoff.mp3", 1.0, false, false);
            }
            manualNMNow = millis();
            break;
        case '3':    // "3" held down -> play audio file "key3.mp3"
            doKey = false;
            play_file("/key3.mp3", 1.0, true, true);
            break;
        case '6':    // "6" held down -> play audio file "key6.mp3"
            doKey = false;
            play_file("/key6.mp3", 1.0, true, true);
            break;
        case '7':    // "7" held down -> re-enable WiFi if in PowerSave mode
            doKey = false;
            if(wifiIsOn()) {
                play_file("/ping.mp3", 1.0, true, false);
            } else {
                if(haveMusic) mpWasActive = mp_stop();
                play_file("/ping.mp3", 1.0, true, true);
                waitAudioDone();
            }
            // Enable WiFi / even if in AP mode / with CP
            wifiOn(0, true, false);
            // Restart mp if it was active before
            if(mpWasActive) mp_play();   
            break;
        case '2':    // "2" held down -> musicplayer prev
            doKey = false;
            if(haveMusic) {
                mp_prev(mpActive);
            } else playBad = true;
            break;
        case '5':    // "5" held down -> musicplayer start/stop
            doKey = false;
            if(haveMusic) {
                if(mpActive) {
                    mp_stop();
                } else {
                    mp_play();
                }
            } else playBad = true;
            break;
        case '8':   // "8" held down -> musicplayer next
            doKey = false;
            if(haveMusic) {
                mp_next(mpActive);
            } else playBad = true;
            break;
        }
        if(playBad) {
            play_file("/baddate.mp3", 1.0, true, false);
        }
        break;
        
    case TCKS_RELEASED:
        if(doKey) {
            if(isSetUpdate) {
                if(isYearUpdate) {
                    recordSetYearKey(key);
                } else {
                    recordSetTimeKey(key);
                }
            } else {
                recordKey(key);
            }
        }
        break;
    }
}

static void enterKeyPressed()
{
    isEnterKeyPressed = true;
    pwrNeedFullNow();
}

static void enterKeyHeld()
{
    isEnterKeyHeld = true;
    pwrNeedFullNow();
}

#ifdef EXTERNAL_TIMETRAVEL_IN
static void ettKeyPressed()
{
    isEttKeyPressed = true;
    pwrNeedFullNow();
}

static void ettKeyHeld()
{
    isEttKeyHeld = true;
    pwrNeedFullNow();
}
#endif

static void recordKey(char key)
{
    dateBuffer[dateIndex++] = key;
    dateBuffer[dateIndex] = '\0';
    // Don't wrap around, overwrite end of date instead
    if(dateIndex >= DATELEN_ALL) dateIndex = DATELEN_ALL - 1;  
    lastKeyPressed = millis();
}

static void recordSetTimeKey(char key)
{
    timeBuffer[timeIndex++] = key;
    timeBuffer[timeIndex] = '\0';
    timeIndex &= 0x1;
}

static void recordSetYearKey(char key)
{
    timeBuffer[yearIndex++] = key;
    timeBuffer[yearIndex] = '\0';
    yearIndex &= 0x3;
}

void resetTimebufIndices()
{
    timeIndex = yearIndex = 0;
    // Do NOT clear the timeBuffer, might be pre-set
}

void enterkeyScan()
{
    enterKey.scan();  // scan the enter button

#ifdef EXTERNAL_TIMETRAVEL_IN
    ettKey.scan();    // scan the ext. time travel button
#endif
}

/*
 * keypad_loop()
 */
void keypad_loop()
{
    char spTxt[16];
    #define EE1_KL2 12
    char spTxtS2[EE1_KL2] = { 181, 224, 179, 231, 199, 140, 197, 129, 197, 140, 194, 133 };
    const char *tmr = "TIMER   ";

    enterkeyScan();

    // Discard keypad input after 2 minutes of inactivity
    if(millis() - lastKeyPressed >= 2*60*1000) {
        dateBuffer[0] = '\0';
        dateIndex = 0;
    }

    // Bail out if sequence played or device is fake-"off"
    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP0 || timeTravelP1) {

        isEnterKeyHeld = false;
        isEnterKeyPressed = false;
        #ifdef EXTERNAL_TIMETRAVEL_IN
        isEttKeyPressed = false;
        isEttKeyHeld = false;
        #endif

        return;

    }

#ifdef EXTERNAL_TIMETRAVEL_IN
    if(isEttKeyHeld) {
        resetPresentTime();
        isEttKeyPressed = isEttKeyHeld = false;
    } else if(isEttKeyPressed) {
        if(!ettDelay) {
            timeTravel(ettLong, true);
            ettDelayed = false;
        } else {
            ettNow = millis();
            ettDelayed = true;
        }
        isEttKeyPressed = isEttKeyHeld = false;
    }
    if(ettDelayed) {
        if(millis() - ettNow >= ettDelay) {
            timeTravel(ettLong, true);
            ettDelayed = false;
        }
    }
#endif

    // If enter key is held, go into keypad menu
    if(isEnterKeyHeld) {

        isEnterKeyHeld = false;
        isEnterKeyPressed = false;
        cancelEnterAnim();
        cancelETTAnim();

        timeIndex = yearIndex = 0;
        timeBuffer[0] = '\0';

        enter_menu();

        isEnterKeyHeld = false;
        isEnterKeyPressed = false;

        #ifdef EXTERNAL_TIMETRAVEL_IN
        // No external tt while in menu mode,
        // so reset flag upon menu exit
        isEttKeyPressed = isEttKeyHeld = false;
        #endif

    }

    // if enter key is merely pressed, copy dateBuffer to destination time (if valid)
    if(isEnterKeyPressed) {

        int  strLen = strlen(dateBuffer);
        bool invalidEntry = false;
        bool validEntry = false;
        bool enterInterruptsMusic = false;

        isEnterKeyPressed = false;
        enterWasPressed = true;

        cancelETTAnim();

        // Turn on white LED
        digitalWrite(WHITE_LED_PIN, HIGH);

        // Turn off destination time
        destinationTime.off();

        timeNow = millis();

        if(strLen != DATELEN_ALL  &&
           strLen != DATELEN_DATE &&
           (strLen < DATELEN_CMIN ||
            strLen > DATELEN_CMAX) ) {

            invalidEntry = true;

        } else if(strLen == DATELEN_ALSH) {

            char atxt[16];
            
            if(dateBuffer[0] == '1' && dateBuffer[1] == '1') {

                int al = getAlarm();
                if(al >= 0) {
                    const char *alwd = getAlWD(alarmWeekday);
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%-7s %02d%02d", alwd, al >> 8, al & 0xff);
                    #else
                    sprintf(atxt, "%-8s %02d%02d", alwd, al >> 8, al & 0xff);
                    #endif
                    destinationTime.showTextDirect(atxt);
                    validEntry = true;
                } else {
                    #ifdef IS_ACAR_DISPLAY
                    destinationTime.showTextDirect("ALARM  UNSET");
                    #else
                    destinationTime.showTextDirect("ALARM   UNSET");
                    #endif
                    invalidEntry = true;
                }
                specDisp = 10;

            } else if(dateBuffer[0] == '4' && dateBuffer[1] == '4') {

                if(!ctDown) {
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%s OFF", tmr);
                    #else
                    sprintf(atxt, "%s  OFF", tmr);
                    #endif
                    invalidEntry = true;
                } else {
                    unsigned long el = ctDown - (millis()-ctDownNow);
                    uint8_t mins = el/(1000*60);
                    uint8_t secs = (el-(mins*1000*60))/1000;
                    if((long)el < 0) mins = secs = 0;
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%s%02d%02d", tmr, mins, secs);
                    #else
                    sprintf(atxt, "%s %02d%02d", tmr, mins, secs);
                    #endif
                    validEntry = true;
                }
                destinationTime.showTextDirect(atxt);
                specDisp = 10;

            } else 
                invalidEntry = true;

        } else if(strLen == DATELEN_CODE) {

            uint16_t code = atoi(dateBuffer);
            char atxt[16];
            
            switch(code) {
            #ifdef TC_HAVETEMP
            case 111:               // 111+ENTER: Toggle rc-mode
                if(haveRcMode) {
                    toggleRcMode();
                    if(tempSens.haveHum()) {
                        departedTime.off();
                        rcModeDepTime = true;
                    }
                    validEntry = true;
                } else {
                    invalidEntry = true;
                }
                break;
            #endif
            case 222:               // 222+ENTER: Turn shuffle off
            case 555:               // 555+ENTER: Turn shuffle on
                if(haveMusic) {
                    mp_makeShuffle((code == 555));
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "SHUFFLE  %s", (code == 555) ? " ON" : "OFF");
                    #else
                    sprintf(atxt, "SHUFFLE   %s", (code == 555) ? " ON" : "OFF");
                    #endif
                    destinationTime.showTextDirect(atxt);
                    specDisp = 10;
                    validEntry = true;
                } else {
                    invalidEntry = true;
                }
                break;
            case 888:               // 888+ENTER: Goto song #0
                if(haveMusic) {
                    mp_gotonum(0, mpActive);
                    #ifdef IS_ACAR_DISPLAY
                    strcpy(atxt, "NEXT     000");
                    #else
                    strcpy(atxt, "NEXT      000");
                    #endif
                    destinationTime.showTextDirect(atxt);
                    specDisp = 10;
                    validEntry = true;
                } else {
                    invalidEntry = true;
                }
                break;
            case 000:
                muteBeep = !muteBeep;
                break;
            default:
                invalidEntry = true;
            }

        } else if(strLen == DATELEN_INT) {

            if(!(strncmp(dateBuffer, "64738", 5))) {
                mp_stop();
                stopAudio();
                allOff();
                #ifdef TC_HAVESPEEDO
                if(useSpeedo) speedo.off();
                #endif
                destinationTime.resetBrightness();
                destinationTime.showTextDirect("REBOOTING");
                destinationTime.on();
                delay(ENTER_DELAY);
                digitalWrite(WHITE_LED_PIN, LOW);
                esp_restart();
            }

            invalidEntry = true;

        } else if(strLen == DATELEN_QALM) {

            char atxt[16];
            uint8_t aHour, aMin;
            uint16_t num = 0;

            if(dateBuffer[0] == '1' && dateBuffer[1] == '1') {
                aHour = ((dateBuffer[2] - '0') * 10) + (dateBuffer[3] - '0');
                aMin  = ((dateBuffer[4] - '0') * 10) + (dateBuffer[5] - '0');
                if(aHour <= 23 && aMin <= 59) {
                    const char *alwd = getAlWD(alarmWeekday);
                    if( (alarmHour != aHour)  ||
                        (alarmMinute != aMin) ||
                        !alarmOnOff ) {
                        alarmHour = aHour;
                        alarmMinute = aMin;
                        alarmOnOff = true;
                        saveAlarm();
                    }
                    #ifdef IS_ACAR_DISPLAY
                    sprintf(atxt, "%-7s %02d%02d", alwd, alarmHour, alarmMinute);
                    #else
                    sprintf(atxt, "%-8s %02d%02d", alwd, alarmHour, alarmMinute);
                    #endif
                    destinationTime.showTextDirect(atxt);
                    specDisp = 10;
                    validEntry = true;
                } else {
                    invalidEntry = true;
                }
            } else if(haveMusic && dateBuffer[0] == '8' && dateBuffer[1] == '8' && dateBuffer[2] == '8') {
                num = ((dateBuffer[3] - '0') * 100) + ((dateBuffer[4] - '0') * 10) + (dateBuffer[5] - '0');
                num = mp_gotonum(num, mpActive);
                #ifdef IS_ACAR_DISPLAY
                sprintf(atxt, "NEXT     %03d", num);
                #else
                sprintf(atxt, "NEXT      %03d", num);
                #endif
                destinationTime.showTextDirect(atxt);
                specDisp = 10;
                validEntry = true;
            } else {
                invalidEntry = true;
            }

        } else if(strLen == DATELEN_TIME && 
                  dateBuffer[0] == '4' && dateBuffer[1] == '4') {

            char atxt[16];
            uint8_t mins;
            
            mins = ((dateBuffer[2] - '0') * 10) + (dateBuffer[3] - '0');
            if(!mins) {
                #ifdef IS_ACAR_DISPLAY
                sprintf(atxt, "%s OFF", tmr);
                #else
                sprintf(atxt, "%s  OFF", tmr);
                #endif
                ctDown = 0;
            } else {
                #ifdef IS_ACAR_DISPLAY
                sprintf(atxt, "%s%02d00", tmr, mins);
                #else
                sprintf(atxt, "%s %02d00", tmr, mins);
                #endif
                ctDown = mins * 60 * 1000;
                ctDownNow = millis();
            }

            destinationTime.showTextDirect(atxt);
            specDisp = 10;
            validEntry = true;

        } else {

            int _setMonth = -1, _setDay = -1, _setYear = -1;
            int _setHour = -1, _setMin = -1, temp1, temp2;
            int special = 0;
            uint32_t spTmp;
            #ifdef IS_ACAR_DISPLAY
            #define EE1_KL1 12
            char spTxtS1[EE1_KL1] = { 207, 254, 206, 255, 206, 247, 206, 247, 199, 247, 207, 247 };
            #else
            #define EE1_KL1 13
            char spTxtS1[EE1_KL1] = { 181, 244, 186, 138, 187, 138, 179, 138, 179, 131, 179, 139, 179 };
            #endif

            #ifdef TC_DBG
            Serial.print(F("date entered: ["));
            Serial.print(dateBuffer);
            Serial.println(F("]"));
            #endif

            for(int i = 0; i < strLen; i++) dateBuffer[i] -= '0';

            temp1 = (dateBuffer[0] * 10) + dateBuffer[1];
            temp2 = (dateBuffer[2] * 10) + dateBuffer[3];
            
            // Convert dateBuffer to date
            if(strLen == DATELEN_TIME) {
                _setHour  = temp1;
                _setMin   = temp2;
            } else {
                _setMonth = temp1;
                _setDay   = temp2;
                _setYear  = (dateBuffer[4] * 1000) + (dateBuffer[5] * 100) + 
                            (dateBuffer[6] *   10) +  dateBuffer[7];
                if(strLen == DATELEN_ALL) {
                    _setHour = (dateBuffer[8]  * 10) + dateBuffer[9];
                    _setMin  = (dateBuffer[10] * 10) + dateBuffer[11];
                }

                // Fix month
                if (_setMonth < 1)  _setMonth = 1;
                if (_setMonth > 12) _setMonth = 12;

                // Check if day makes sense for the month entered
                if(_setDay < 1)     _setDay = 1;
                if(_setDay > daysInMonth(_setMonth, _setYear)) {
                    //set to max day in that month
                    _setDay = daysInMonth(_setMonth, _setYear); 
                }

                // year: 1-9999 allowed. There is no year "0", for crying out loud.
                // Having said that, we allow it anyway, let the people have
                // the full movie experience.
                //if(_setYear < 1) _setYear = 1;

                spTmp = (uint32_t)_setYear << 16 | _setMonth << 8 | _setDay;
                if((spTmp ^ getHrs1KYrs(7)) == 70667637) {
                    special = 1;
                    spTxt[EE1_KL1] = '\0';
                    for(int i = EE1_KL1-1; i >= 0; i--) {
                        spTxt[i] = spTxtS1[i] ^ (i == 0 ? 0xff : spTxtS1[i-1]);
                    }
                } else if((spTmp ^ getHrs1KYrs(8)) == 59572453)  {
                    if(_setHour >= 9 && _setHour <= 11) {
                        special = 2;
                    }
                } else if((spTmp ^ getHrs1KYrs(6)) == 97681642)  {
                    special = 3;
                } else if((spTmp ^ getHrs1KYrs(8)) == 65998071)  {
                    special = 4;
                }
            }

            // hour and min are checked in clockdisplay

            // Normal date/time: ENTER-sound interrupts musicplayer
            enterInterruptsMusic = true;

            switch(special) {
            case 1:
                destinationTime.showTextDirect(spTxt);
                specDisp = 1;
                validEntry = true;
                break;
            case 2:
                play_file("/ee2.mp3", 1.0, true, true, false);
                enterDelay = EE2_DELAY;
                break;
            case 3:
                play_file("/ee3.mp3", 1.0, true, true, false);
                enterDelay = EE3_DELAY;
                break;
            case 4:
                play_file("/ee4.mp3", 1.0, true, true, false);
                enterDelay = EE4_DELAY;
                break;
            default:
                validEntry = true;
            }

            // Copy date to destination time
            if(_setYear >= 0) destinationTime.setYear(_setYear);   // ny0: >
            if(_setMonth > 0) destinationTime.setMonth(_setMonth);
            if(_setDay > 0)   destinationTime.setDay(_setDay);
            if(_setHour >= 0) destinationTime.setHour(_setHour);
            if(_setMin >= 0)  destinationTime.setMinute(_setMin);

            // We only save the new time to NVM if user wants persistence.
            // Might not be preferred; first, this messes with the user's custom
            // times. Secondly, it wears the flash memory.
            if(timetravelPersistent) {
                destinationTime.save();
            }

            // Pause autoInterval-cycling so user can play undisturbed
            pauseAuto();

            // Disable rc mode
            enableRcMode(false);
        }

        if(validEntry) {
            play_file("/enter.mp3", 1.0, true, enterInterruptsMusic);
            enterDelay = ENTER_DELAY;
        } else if(invalidEntry) {
            play_file("/baddate.mp3", 1.0, true, enterInterruptsMusic);
            enterDelay = BADDATE_DELAY;
        }

        // Prepare for next input
        dateIndex = 0;
        dateBuffer[0] = '\0';
    }

    // Turn everything back on after entering date
    // (might happen in next iteration of loop)

    if(enterWasPressed && (millis() - timeNow > enterDelay)) {

        if(specDisp) {

            switch(specDisp++) {
            case 2:
            case 10:
                if(specDisp == 3) destinationTime.onCond();
                else              { destinationTime.resetBrightness(); destinationTime.on(); }
                digitalWrite(WHITE_LED_PIN, LOW);
                timeNow = millis();
                enterWasPressed = true;
                enterDelay = (specDisp == 3) ? EE1_DELAY2 : ENTER_DELAY*5;
                break;
            case 3:
                spTxt[EE1_KL2] = '\0';
                for(int i = EE1_KL2-1; i >= 0; i--) {
                    spTxt[i] = spTxtS2[i] ^ (i == 0 ? 0xff : spTxtS2[i-1]);
                }
                destinationTime.showTextDirect(spTxt);
                timeNow = millis();
                enterWasPressed = true;
                enterDelay = EE1_DELAY3;
                play_file("/ee1.mp3", 1.0, true, true, false);
                break;
            case 4:
            case 11:
                specDisp = 0;
                break;
            }

        }

        if(!specDisp) {

            #ifdef TC_HAVETEMP
            if(isRcMode()) {

                destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit, true);
                if(rcModeDepTime) {
                    departedTime.showHumDirect(tempSens.readHum(), true);
                }
                mydelay(80);                      // Wait 80ms
                destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
                if(rcModeDepTime) {
                    departedTime.showHumDirect(tempSens.readHum());
                }
              
            } else {
            #endif

                destinationTime.showAnimate1();   // Show all but month
                #ifdef TC_HAVETEMP
                if(rcModeDepTime) {
                    departedTime.showAnimate1();
                }
                #endif
                mydelay(80);                      // Wait 80ms
                destinationTime.showAnimate2();   // turn on month
                #ifdef TC_HAVETEMP
                if(rcModeDepTime) {
                    departedTime.showAnimate2();
                }
                #endif

            #ifdef TC_HAVETEMP
            }
            #endif

            digitalWrite(WHITE_LED_PIN, LOW); // turn off white LED

            enterWasPressed = false;          // reset flags

            #ifdef TC_HAVETEMP
            rcModeDepTime = false;
            #endif

        }

    }
}

void cancelEnterAnim(bool reenableDT)
{
    if(enterWasPressed) {
        enterWasPressed = false;
        
        digitalWrite(WHITE_LED_PIN, LOW);
        
        if(reenableDT) {
            #ifdef TC_HAVETEMP
            if(isRcMode()) {
                destinationTime.showTempDirect(tempSens.readLastTemp(), tempUnit);
            } else
            #endif
                destinationTime.show();
            destinationTime.onCond();
            #ifdef TC_HAVETEMP
            if(rcModeDepTime) {
                if(isRcMode()) {
                    departedTime.showHumDirect(tempSens.readHum());
                } else {
                    departedTime.show();
                }
                departedTime.onCond();
            }
            #endif
        }
        
        #ifdef TC_HAVETEMP
        rcModeDepTime = false;
        #endif
        specDisp = 0;
    }
}

void cancelETTAnim()
{
    #ifdef EXTERNAL_TIMETRAVEL_IN
    ettDelayed = false;
    #endif
}

/*
 * Custom delay function for key scan in keypad_i2c
 */
static void mykpddelay(unsigned int mydel)
{
    unsigned long startNow = millis();
    audio_loop();
    while(millis() - startNow < mydel) {
        delay(1);
        ntp_short_loop();
        audio_loop();
    }
}

/*
 * Night mode
 */

static void setNightMode(bool nm)
{
    destinationTime.setNightMode(nm);
    presentTime.setNightMode(nm);
    departedTime.setNightMode(nm);
    #ifdef TC_HAVESPEEDO
    if(useSpeedo) speedo.setNightMode(nm);
    #endif
}

void nightModeOn()
{
    setNightMode(true);
    leds_off();
}

void nightModeOff()
{
    setNightMode(false);
    leds_on();
}

bool toggleNightMode()
{
    if(destinationTime.getNightMode()) {
        nightModeOff();
        return false;
    }
    nightModeOn();
    return true;
}

void leds_on()
{
    if(FPBUnitIsOn && !destinationTime.getNightMode()) {
        digitalWrite(LEDS_PIN, HIGH);
    }
}

void leds_off()
{
    digitalWrite(LEDS_PIN, LOW);
}
