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

#include "tc_keypad.h"

const char keys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

#ifdef GTE_KEYPAD
byte rowPins[4] = {5, 0, 1, 3};
byte colPins[3] = {4, 6, 2};
#else
byte rowPins[4] = {1, 6, 5, 3}; // connect to the row pinouts of the keypad  2+64+32+8 = 0x6a  INPUT
byte colPins[3] = {2, 0, 4};    // connect to the column pinouts of the keypad  4+1+16 = 0x15  OUTPUT
#endif

Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, KEYPAD_ADDR, PCF8574);

bool isEnterKeyPressed = false;
bool isEnterKeyHeld = false;
bool enterWasPressed = false;

#ifdef EXTERNAL_TIMETRAVEL_IN
bool isEttKeyPressed = false;
unsigned long ettNow = 0;
bool ettDelayed = false;
unsigned long ettDelay = 0; // ms
bool ettLong;
#endif

unsigned long timeNow = 0;

unsigned long lastKeyPressed = 0;

#define DATELEN_ALL   12   // month, day, year, hour, min
#define DATELEN_DATE   8   // month, day, year
#define DATELEN_TIME   4   // hour, minute
#define DATELEN_CODE   3   // special

char dateBuffer[DATELEN_ALL + 2];
char timeBuffer[8]; // 4 characters to accommodate date and time settings

int dateIndex = 0;
int timeIndex = 0;
int yearIndex = 0;

bool doKey = false;

unsigned long enterDelay = 0;

OneButton enterKey = OneButton(ENTER_BUTTON_PIN,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor
);

#ifdef EXTERNAL_TIMETRAVEL_IN
OneButton ettKey = OneButton(EXTERNAL_TIMETRAVEL_IN_PIN,
    true,     // Button is active LOW
    true      // Enable internal pull-up resistor
);
#endif

/*
 * keypad_setup()
 * 
 */
void keypad_setup() 
{
    byte rowMask = (1 << rowPins[0]) | (1 << rowPins[1]) | (1 << rowPins[2]) | (1 << rowPins[3]); // input
    byte colMask = (1 << colPins[0]) | (1 << colPins[1]) | (1 << colPins[2]);                     // output

    keypad.begin(makeKeymap(keys));
    
    keypad.addEventListener(keypadEvent);
    
    keypad.setHoldTime(ENTER_HOLD_TIME); 

    keypad.setDebounceTime(20);

    keypad.colMask = colMask;
    keypad.rowMask = rowMask;

    // Setup pin for white LED
    pinMode(WHITE_LED_PIN, OUTPUT);
    digitalWrite(WHITE_LED_PIN, LOW);  

    // Setup Enter button
    enterKey.setClickTicks(ENTER_CLICK_TIME);
    enterKey.setPressTicks(ENTER_HOLD_TIME);
    enterKey.setDebounceTicks(ENTER_DEBOUNCE);
    enterKey.attachClick(enterKeyPressed);    
    enterKey.attachLongPressStart(enterKeyHeld);

#ifdef EXTERNAL_TIMETRAVEL_IN
    // Setup External time travel button
    ettKey.setClickTicks(ETT_CLICK_TIME);   
    ettKey.setPressTicks(ETT_HOLD_TIME); 
    ettKey.setDebounceTicks(ETT_DEBOUNCE);
    ettKey.attachLongPressStart(ettKeyPressed);   

    ettDelay = (int)atoi(settings.ettDelay);
    if(ettDelay > 300000) ettDelay = 0;

    ettLong = ((int)atoi(settings.ettLong) > 0);
#endif

    dateBuffer[0] = '\0';
    timeBuffer[0] = '\0';
  
    #ifdef TC_DBG
    Serial.println(F("keypad_setup: Setup Complete"));
    #endif
}

/*
 * get_key()
 * 
 * Do not use keypad.getKey() directly!
 */
char get_key() 
{    
    char key;    

    keypad.scanKeys = true;                          
    key = keypad.getKey();  
    keypad.scanKeys = false;      
         
    return key;
}

/* 
 *  The keypad event handler
 */
void keypadEvent(KeypadEvent key) 
{
    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP1) 
        return;
    
    switch(keypad.getState()) {
        case PRESSED:
            if(key != '#' && key != '*') {
                play_keypad_sound(key);
            }
            doKey = true;            
            break;        
        case HOLD:            
            if(key == '0') {    // "0" held down -> time travel
                doKey = false;
                timeTravel(true);     // make long timeTravel             
            }
            if(key == '9') {    // "9" held down -> return from time travel                
                doKey = false;
                resetPresentTime();                
            }
            if(key == '1') {    // "1" held down -> turn alarm on
                doKey = false;
                if(alarmOn()) {          
                    play_file("/alarmon.mp3", 1.0, true, 0);      
                } else {
                    play_file("/baddate.mp3", 1.0, true, 0); 
                }
            }
            if(key == '2') {    // "2" held down -> turn alarm off
                doKey = false;
                alarmOff();                
                play_file("/alarmoff.mp3", 1.0, true, 0);
            }
            if(key == '4') {    // "4" held down -> nightmode on
                doKey = false;
                nightModeOn();
                play_file("/nmon.mp3", 1.0, false, 0);                   
            }
            if(key == '5') {    // "5" held down -> nightmode off
                doKey = false;
                nightModeOff(); 
                play_file("/nmoff.mp3", 1.0, false, 0);                
            }
            if(key == '3') {    // "3" held down -> play audio file "key3"
                doKey = false;
                play_file("/key3.mp3", 1.0, true, 0);                
            }
            if(key == '6') {    // "6" held down -> play audio file "key6"
                doKey = false;
                play_file("/key6.mp3", 1.0, true, 0);                
            }
            break;
        case RELEASED:
            if(doKey && key != '#' && key != '*') {
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
        case IDLE:
            break;
    }
}

void enterKeyPressed() 
{
    isEnterKeyPressed = true;
}

void enterKeyHeld() 
{
    isEnterKeyHeld = true;
}

#ifdef EXTERNAL_TIMETRAVEL_IN
void ettKeyPressed() 
{
    isEttKeyPressed = true;
}
#endif

void recordKey(char key) 
{
    dateBuffer[dateIndex++] = key;
    dateBuffer[dateIndex] = '\0'; 
    if(dateIndex >= DATELEN_ALL) dateIndex = DATELEN_ALL - 1;  // don't overflow, will overwrite end of date next time
    lastKeyPressed = millis();
}

void recordSetTimeKey(char key) 
{
    timeBuffer[timeIndex++] = key;  
    timeBuffer[timeIndex] = '\0';      
    if(timeIndex == 2) timeIndex = 0;       
}

void recordSetYearKey(char key) 
{
    timeBuffer[yearIndex++] = key;
    timeBuffer[yearIndex] = '\0';    
    if(yearIndex == 4) yearIndex = 0;   
}

void resetTimebufIndices() 
{
    timeIndex = yearIndex = 0;
    // Do NOT clear the timeBuffer, might be pre-set
}

void enterkeytick() 
{
    enterKey.tick();  // manages the enter button (updates flags)
    
#ifdef EXTERNAL_TIMETRAVEL_IN
    ettKey.tick();    // manages the extern time travel button (updates flags)
#endif         
}

/*
 * keypad_loop()
 * 
 */
void keypad_loop() 
{   
    char spTxt[16];
    #define EE1_KL2 12
    char spTxtS2[EE1_KL2] = { 181, 224, 179, 231, 199, 140, 197, 129, 197, 140, 194, 133 };

    enterkeytick();

    // Discard keypad input after 5 minutes of inactivity
    if(millis() - lastKeyPressed >= 5*60*1000) {
        dateBuffer[0] = '\0'; 
        dateIndex = 0;
    }

    // Bail out if sequence played or device is fake-"off"
    if(!FPBUnitIsOn || startup || timeTraveled || timeTravelP1) {

        isEnterKeyHeld = false;     
        isEnterKeyPressed = false;
        #ifdef EXTERNAL_TIMETRAVEL_IN
        isEttKeyPressed = false; 
        #endif
        
        return;
        
    }

#ifdef EXTERNAL_TIMETRAVEL_IN
    if(isEttKeyPressed) {
        if(!ettDelay) {
            timeTravel(ettLong);
            ettDelayed = false;          
        } else {
            ettNow = millis();
            ettDelayed = true;
        }
        isEttKeyPressed = false;
    }
    if(ettDelayed) {
        if(millis() - ettNow >= ettDelay) {
            timeTravel(ettLong);
            ettDelayed = false;
        }
    }
#endif    

    // if enter key is held go into keypad menu
    if(isEnterKeyHeld) { 
             
        isEnterKeyHeld = false;     
        isEnterKeyPressed = false;
        cancelEnterAnim();
        cancelETTAnim();
        
        timeIndex = yearIndex = 0;
        timeBuffer[0] = '\0';
        
        enter_menu();     // enter menu mode; in tc_menu.cpp
        
        isEnterKeyHeld = false;     
        isEnterKeyPressed = false;   
        
        #ifdef EXTERNAL_TIMETRAVEL_IN
        // No external tt while in menu mode,
        // so reset flag upon menu exit
        isEttKeyPressed = false;     
        #endif 
        
    }

    // if enter key is merely pressed, copy dateBuffer to destination time (if valid)
    if(isEnterKeyPressed) {

        int strLen = strlen(dateBuffer);
      
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
           strLen != DATELEN_TIME &&
           strLen != DATELEN_CODE) {
            
            Serial.println(F("keypad_loop: Date is too long or too short"));
                        
            play_file("/baddate.mp3", 1.0, true, 0); 
            enterDelay = BADDATE_DELAY;  
                 
        } else if(strLen == DATELEN_CODE) {

            uint16_t code = atoi(dateBuffer);
            switch(code) {
                          
            // TODO

            default:
                play_file("/baddate.mp3", 1.0, true, 0);
                enterDelay = BADDATE_DELAY; 
            }
          
        } else {
          
            int _setMonth = -1, _setDay = -1, _setYear = -1;
            int _setHour = -1, _setMin = -1;
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

            // Convert char to String so substring can be used
            String dateBufferString(dateBuffer); 

            // Convert dateBuffer to date
            if(strLen == DATELEN_TIME) {
                _setHour = dateBufferString.substring(0, 2).toInt();
                _setMin  = dateBufferString.substring(2, 4).toInt();
            } else {         
                _setMonth = dateBufferString.substring(0, 2).toInt();
                _setDay = dateBufferString.substring(2, 4).toInt();
                _setYear = dateBufferString.substring(4, 8).toInt();
                if(strLen == DATELEN_ALL) {
                    _setHour = dateBufferString.substring(8, 10).toInt();
                    _setMin = dateBufferString.substring(10, 12).toInt();
                }
    
                // Fix month
                if (_setMonth < 1)  _setMonth = 1;
                if (_setMonth > 12) _setMonth = 12;            
    
                // Check if day makes sense for the month entered 
                if(_setDay < 1)     _setDay = 1;
                if(_setDay > daysInMonth(_setMonth, _setYear)) {
                    _setDay = daysInMonth(_setMonth, _setYear); //set to max day in that month
                }
    
                // year: 1-9999 allowed. There is no year "0", for crying out loud.
                if(_setYear < 1)    _setYear = 1;

                spTmp = (uint32_t)_setYear << 16 | _setMonth << 8 | _setDay;
                if((spTmp ^ getHrs1KYrs(7)) == 70880781) { 
                    special = 1;
                    spTxt[EE1_KL1] = '\0';
                    for(int i = EE1_KL1-1; i >= 0; i--) {
                        spTxt[i] = spTxtS1[i] ^ (i == 0 ? 0xff : spTxtS1[i-1]);                    
                    }
                } else if((spTmp ^ getHrs1KYrs(8)) == 59652301)  { 
                    if(_setHour >= 9 && _setHour <=11) {
                        special = 2;
                    }
                } else if((spTmp ^ getHrs1KYrs(6)) == 97637962)  {
                    special = 3;
                } else if((spTmp ^ getHrs1KYrs(8)) == 65811679)  {
                    special = 4;
                }
            }

            // hour and min are checked in clockdisplay

            switch(special) {
            case 1:                
                destinationTime.showOnlyText(spTxt);                
                specDisp = 1;
                play_file("/enter.mp3", 1.0, true, 0);
                enterDelay = ENTER_DELAY;
                break;
            case 2:
                play_file("/ee2.mp3", 1.0, true, 0, false);
                enterDelay = EE2_DELAY; 
                break;
            case 3:
                play_file("/ee3.mp3", 1.0, true, 0, false);
                enterDelay = EE3_DELAY; 
                break;
            case 4:
                play_file("/ee4.mp3", 1.0, true, 0, false);
                enterDelay = EE4_DELAY; 
                break;
            default:
                play_file("/enter.mp3", 1.0, true, 0);
                enterDelay = ENTER_DELAY; 
            }

            // Copy date to destination time
            if(_setYear > 0)    destinationTime.setYear(_setYear);
            if(_setMonth > 0)   destinationTime.setMonth(_setMonth);
            if(_setDay > 0)     destinationTime.setDay(_setDay);            
            if(_setHour >= 0)   destinationTime.setHour(_setHour);
            if(_setMin >= 0)    destinationTime.setMinute(_setMin);

            // We only save the new time to the EEPROM if user wants persistence.
            // Might not be preferred; first, this messes with the user's custom
            // times. Secondly, it wears the flash memory.
            if(timetravelPersistent) {
                destinationTime.save();      
            }     

            // Pause autoInterval-cycling so user can play undisturbed
            pauseAuto();
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
                destinationTime.on();
                digitalWrite(WHITE_LED_PIN, LOW);
                timeNow = millis();
                enterWasPressed = true;
                enterDelay = EE1_DELAY2;
                break;
            case 3:
                spTxt[EE1_KL2] = '\0';
                for(int i = EE1_KL2-1; i >= 0; i--) {
                    spTxt[i] = spTxtS2[i] ^ (i == 0 ? 0xff : spTxtS2[i-1]);                    
                }
                destinationTime.showOnlyText(spTxt);
                timeNow = millis();
                enterWasPressed = true;
                enterDelay = EE1_DELAY3;
                play_file("/ee1.mp3", 1.0, true, 0, false);
                break;
            case 4:
                specDisp = 0;
                break;
            }
            
        } 

        if(!specDisp) {
          
            destinationTime.showAnimate1();   // Show all but month
            mysdelay(80);                     // Wait 80ms
            destinationTime.showAnimate2();   // turn on month
        
            digitalWrite(WHITE_LED_PIN, LOW); // turn off white LED
        
            enterWasPressed = false;          // reset flag
       
        }
    }
}

void cancelEnterAnim()
{
    if(enterWasPressed) {
        enterWasPressed = false;
        digitalWrite(WHITE_LED_PIN, LOW);
        destinationTime.show();
        destinationTime.on();
        specDisp = 0;
    }
}

void cancelETTAnim()
{
    #ifdef EXTERNAL_TIMETRAVEL_IN
    if(ettDelayed) {
        ettDelayed = false;
        // ...
    }
    #endif
}    

/*
 * Night mode
 * 
 */
void nightModeOn() 
{
    destinationTime.setNightMode(true);
    presentTime.setNightMode(true);
    departedTime.setNightMode(true);
}

void nightModeOff()
{
    destinationTime.setNightMode(false);
    presentTime.setNightMode(false);
    departedTime.setNightMode(false);
}
