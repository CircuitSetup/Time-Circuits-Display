/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * Enhanced/modified in 2022 by Thomas Winischhofer (A10001986)
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
 * 
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
byte rowPins[4] = {1, 6, 5, 3}; // connect to the row pinouts of the keypad
byte colPins[3] = {2, 0, 4};    // connect to the column pinouts of the keypad
#endif

Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, KEYPAD_ADDR, PCF8574);

bool isEnterKeyPressed = false;
bool isEnterKeyHeld = false;
bool isEnterKeyDouble = false;
bool enterWasPressed = false;
bool menuFlag = false;

int enterVal = 0;
long timeNow = 0;

const int maxDateLength = 12;  //month, day, year, hour, min
const int minDateLength = 8;   //month, day, year

char dateBuffer[maxDateLength + 2];
char timeBuffer[8]; // 4 characters to accomodate date and time settings

int dateIndex = 0;
int timeIndex = 0;
int yearIndex = 0;

bool doKey = false;

OneButton enterKey = OneButton(ENTER_BUTTON,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor
);

/*
 * keypad_setup()
 * 
 */
void keypad_setup() 
{
    keypad.begin(makeKeymap(keys));
    
    keypad.addEventListener(keypadEvent);  // add an event listener for this keypad
    
    keypad.setHoldTime(ENTER_HOLD_TIME);   // 3 sec hold time

    // Setup pins for white LED and Enter key
    pinMode(WHITE_LED, OUTPUT);
    digitalWrite(WHITE_LED, LOW);  
    
    pinMode(ENTER_BUTTON, INPUT_PULLDOWN);
    digitalWrite(ENTER_BUTTON, LOW);

    enterKey.attachClick(enterKeyPressed);
    //enterKey.attachDuringLongPress(enterKeyHeld); // FIXME - we do not need repeated events as long as the button is held,
    enterKey.attachLongPressStart(enterKeyHeld);    // FIXME - we only need info when the button is held long enough
    //enterKey.attachDoubleClick(enterKeyDouble);
    enterKey.setPressTicks(ENTER_HOLD_TIME);
    enterKey.setClickTicks(ENTER_DOUBLE_TIME);
    enterKey.setDebounceTicks(ENTER_DEBOUNCE);

    #ifdef TC_DBG
    Serial.println("keypad_setup: Setup Complete");
    #endif
}

char get_key() 
{
    return keypad.getKey();
}

// handle different cases for keypresses
void keypadEvent(KeypadEvent key) 
{
    switch (keypad.getState()) {
        case PRESSED:
            if(key != '#' && key != '*') {
                play_keypad_sound(key);
            }
            doKey = true;            
            break;        
        case HOLD:            
            if(key == '0') {    // "0" held down -> time travel
                doKey = false;
                timeTravel();                
            }
            if(key == '9') {    // "9" held down -> return from time travel
                doKey = false;
                resetPresentTime();                
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

void recordKey(char key) 
{
    dateBuffer[dateIndex++] = key;
    dateBuffer[dateIndex] = '\0'; 
    if (dateIndex >= maxDateLength) dateIndex = maxDateLength - 1;  // don't overflow, will overwrite end of date next time
    //Serial.println(dateIndex);
}

void recordSetTimeKey(char key) 
{
    timeBuffer[timeIndex++] = key;  
    timeBuffer[timeIndex] = '\0';      
    if(timeIndex == 2) timeIndex = 0;    
    //Serial.println(timeIndex);
}

void recordSetYearKey(char key) 
{
    timeBuffer[yearIndex++] = key;
    timeBuffer[yearIndex] = '\0';    
    if(yearIndex == 4) yearIndex = 0;   
    //Serial.println(yearIndex); 
}

void resetTimebufIndices() 
{
    timeIndex = yearIndex = 0;
    // Do NOT clear the timeBuffer, might be pre-set
}

void enterkeytick() 
{
    enterKey.tick();  // manages the enter key (updates flags)
}

/*
 * keypad_loop()
 * 
 */
void keypad_loop() 
{
    enterkeytick();

    // if enter key is held go into keypad menu
    if(isEnterKeyHeld && !menuFlag) { 
             
        isEnterKeyHeld = false;     
        isEnterKeyPressed = false;
        
        menuFlag = true;
        
        timeIndex = yearIndex = 0;
        timeBuffer[0] = '\0';
        
        //audioMute = true;
        enter_menu();     // enter menu mode; in tc_menu.cpp
        //audioMute = false;
        
        isEnterKeyHeld = false;     
        isEnterKeyPressed = false;   
        
    }

    // if enter key is merely pressed, copy dateBuffer to destination time (if valid)
    if(isEnterKeyPressed && !menuFlag) {
      
        isEnterKeyPressed = false; 
        enterWasPressed = true;

        // Turn on white LED
        digitalWrite(WHITE_LED, HIGH); 

        // Turn off destination time only
        destinationTime.off(); 
                 
        timeNow = millis();        
        
        if(strlen(dateBuffer) > maxDateLength || strlen(dateBuffer) < minDateLength) {
            
            Serial.println(F("keypad_loop: Date is too long or too short"));
                        
            play_file("/baddate.mp3", getVolume());   
                 
        } else {
        
            play_file("/enter.mp3", getVolume());

            #ifdef TC_DBG
            Serial.print(F("date entered: ["));
            Serial.print(dateBuffer);
            Serial.println(F("]"));
            #endif

            // Convert char to String so substring can be used
            String dateBufferString(dateBuffer);  

            // Copy dates in dateBuffer and make ints
            int _setMonth = dateBufferString.substring(0, 2).toInt();
            int _setDay = dateBufferString.substring(2, 4).toInt();
            int _setYear = dateBufferString.substring(4, 8).toInt();
            int _setHour = dateBufferString.substring(8, 10).toInt();
            int _setMin = dateBufferString.substring(10, 12).toInt();

            // Fix month
            if (_setMonth < 1)  _setMonth = 1;
            if (_setMonth > 12) _setMonth = 12;            

            // Check if day makes sense for the month entered 
            if(_setDay < 1)     _setDay = 1;
            if(_setDay > daysInMonth(_setMonth, _setYear)) {
                _setDay = daysInMonth(_setMonth, _setYear); //set to max day in that month
            }

            // year: all allowed that can be entered with 4 digits

            // hour and min are checked in clockdisplay

            // Copy date to destination time and save()
            destinationTime.setMonth(_setMonth);
            destinationTime.setDay(_setDay);
            destinationTime.setYear(_setYear);
            destinationTime.setHour(_setHour);
            destinationTime.setMinute(_setMin);
            destinationTime.save();           
        }

        // Prepare for next input
        dateIndex = 0;              
        dateBuffer[0] = '\0';
    }
    
    // Turn everything back on after entering date
    // (might happen in next iteration of loop)
    
    if(enterWasPressed && (millis() > timeNow + ENTER_DELAY)) {
      
        destinationTime.showAnimate1();   // Show all but month
        delay(80);                        // Wait 80ms
        destinationTime.showAnimate2();   // turn on month
        
        digitalWrite(WHITE_LED, LOW);     // turn off white LED
        
        enterWasPressed = false;          // reset flag
        
    }
}