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

/* 
 * The keypad menu
 * 
 * The menu is controlled by "pressing" or "holding" the ENTER key on the keypad.
 * Note: A "press" is shorter than 2 seconds, a "hold" is 2 seconds or longer.
 * Data entry is done via the keypad's number keys.
 * 
 * The menu is involked by holding the ENTER button.
 * First step is to choose a menu item. The available "items" are
 *    - enter custom dates/times for the three displays
 *    - select the autoInterval ("PRE-SET")
 *    - select the brightness for the three displays ("BRI-GHT")
 *    - quit the menu ("END")
 *  Pressing ENTER cycles through the list, holding ENTER selects an item, ie a mode.
 *  If mode is "enter custom dates/times":
 *      - the field to enter data into is shown (exclusively)
 *      - 2 or 4 digits can be entered, or ENTER can be pressed, upon which the next field is activated.
 *        (Note that the month needs to be entered numerically, and the hour needs to be entered in 24
 *        hour mode, which is then internally converted to 12 hour mode.)
 *      - After entering data into all fields, the data is saved and the menu is left automatically.
 *      - Note that after entering dates/times into the "destination" or "last departure" displays,
 *        autoInterval is set to 0 and your entered date/time(s) are shown permanently (see below).
 *      - If you entered a custom date/time into the "present" time display, this time is then
 *        used as actual the present time, and continues to run like a clock. (As opposed to the 
 *        "destination" and "last departure" times, which are stale.)
 *  If mode is "select AutoInterval" (display shows "INT")
 *      - Press ENTER to cycle through the possible autoInverval settings.
 *      - Hold ENTER for 2 seconds to select the shown value and exit the menu ("SAVE" is displayed briefly)
 *      - 0 makes your custom "destination" and "last departure" times to be shown permanently.
 *        (CUS-TOM is displayed as a reminder)
 *      - Non-zero values make the clock cycle through a number of pre-programmed times, your
 *        custom times are ignored. The value means "minutes" (hence "MIN-UTES")               
 *  If mode is "select brightness" (display shows "LVL")
 *      - Press ENTER to cycle through the possible levels (1-5)
 *      - Hold ENTER for 2 seconds to use current value and jump to next display
 *      - After the third display, "SAVE" is displayed briefly and the menu is left automatically.
 *  If mode is "end"
 *      - Hold ENTER to quit the menu
 */


#include "tc_menus.h"

int displayNum;                                           // selected display
uint8_t autoInterval = 1;                                 // array element of autoTimeIntervals[], set's time between automatically displayed times
const uint8_t autoTimeIntervals[5] = {0, 5, 15, 30, 60};  // time options, first must be 0, this is the off option.

bool isSetUpdate = false;
bool isYearUpdate = false;

clockDisplay* displaySet;  // the current display

/*
 * menu_setup()
 * 
 */
void menu_setup() {
    /* nada */
}

/*
 * enter_menu() - the real thing
 * 
 */
void enter_menu() {

    #ifdef TC_DBG
    Serial.println("Menu: enter_menu() invoked");
    #endif
    
    isEnterKeyHeld = false;     
    isEnterKeyPressed = false; 
    
    menuFlag = false;

    // start with destination time in main menu
    displayNum = MODE_DEST;     

    // Load the times
    destinationTime.load();     
    departedTime.load();

    // Highlight current display
    displayHighlight(displayNum);

    waitForEnterRelease();

    timeout = 0;  

    // DisplaySelect: 
    // Wait for ENTER to cycle through main menu (displays and modes), 
    // HOLDing ENTER selects current menu "item"
    // (Also sets displaySet to selected display)    
    displaySelect(displayNum);        

    #ifdef TC_DBG
    Serial.println("Menu: Display/mode selected");    
    #endif
    
    if(displayNum <= MODE_DEPT) {  

        // Enter display times

        // Generate pre-set values, which the user may keep (or overwrite)
         
        uint16_t yearSet;  
        uint16_t monthSet;  
        uint16_t daySet;
        uint16_t minSet;
        uint16_t hourSet;
        // all need to be same type since they're passed by ref

        if(displaySet->isRTC()) {  
          
            // This is the RTC, get the current time, corrected by yearOffset         
            DateTime currentTime = rtc.now();
            yearSet = currentTime.year() - displaySet->getYearOffset();            
            monthSet = currentTime.month();
            daySet = currentTime.day();
            minSet = currentTime.minute();
            hourSet = currentTime.hour();
            
        } else {
          
            // non RTC, get the time info from the object
            yearSet = displaySet->getYear();
            monthSet = displaySet->getMonth();
            daySet = displaySet->getDay();
            minSet = displaySet->getMinute();
            hourSet = displaySet->getHour();
            
        }
 
        timeout = 0;  

        // Get year
        isYearUpdate = true;    // Allow 4 digits instead of 2
        setUpdate(yearSet, FIELD_YEAR);  
        prepareInput(yearSet);
        waitForEnterRelease();      
        setField(yearSet, FIELD_YEAR, 0, 0);         
        isYearUpdate = false;

        // Get month
        setUpdate(monthSet, FIELD_MONTH);
        prepareInput(monthSet);
        waitForEnterRelease();
        setField(monthSet, FIELD_MONTH, 0, 0);          

        // Get day
        setUpdate(daySet, FIELD_DAY);
        prepareInput(daySet);
        waitForEnterRelease();        
        setField(daySet, FIELD_DAY, yearSet, monthSet);  // this depends on the month and year        

        // Get hour
        setUpdate(hourSet, FIELD_HOUR);
        prepareInput(hourSet);
        waitForEnterRelease();       
        setField(hourSet, FIELD_HOUR, 0, 0);       

        // Get minute
        setUpdate(minSet, FIELD_MINUTE);
        prepareInput(minSet);
        waitForEnterRelease();        
        setField(minSet, FIELD_MINUTE, 0, 0); 

        isSetUpdate = false;

        // Have new date & time at this point

        // Do nothing if there was a timeout waiting for button presses                                                  
        if(!checkTimeOut()) {  

            #ifdef TC_DBG
            Serial.println("Menu: Fields all set");
            #endif
    
            if(displaySet->isRTC()) {  // this is the RTC display, set the RTC 
                
                // Update RTC and fake year if neccessary
                // see comments in tc_time.cpp

                int tyr = yearSet, tyroffs = 0;
                if(tyr < 2000) {
                    while(tyr < 2000) {
                        tyr += 28;
                        tyroffs += 28;
                    }
                } else if(tyr > 2050) {
                    while(tyr > 2050) {
                        tyr -= 28;
                        tyroffs -= 28;
                    }
                }
    
                displaySet->setYearOffset(tyroffs);
                presentTimeBogus = true;  // Avoid overwriting this time by NTP time in loop
    
                rtc.adjust(DateTime(tyr, monthSet, daySet, hourSet, minSet, 0));
                
            } else {                 
                // Not rtc, setting a static display, turn off autoInverval
                autoInterval = 0;    
                saveAutoInterval();  
            }

            // Show a save message for a brief moment
            displaySet->showOnlySave();  
            
            // update the object
            displaySet->setMonth(monthSet);
            displaySet->setDay(daySet);
            displaySet->setYear(yearSet);
            displaySet->setHour(hourSet);
            displaySet->setMinute(minSet);
            displaySet->save();  // save to eeprom
            
            mydelay(1000);

            allOff();
            waitForEnterRelease();
            
        }  
        
    } else if(displayNum == 4) {    // Adjust brightness

        allOff();
        waitForEnterRelease();
      
        // Set brightness settings    
        if(!checkTimeOut()) doSetBrightness(&destinationTime);
        if(!checkTimeOut()) doSetBrightness(&presentTime);
        if(!checkTimeOut()) doSetBrightness(&departedTime);
        
        allOff();
        waitForEnterRelease();  
        
        
    } else if(displayNum == 3) {  // Select autoInterval

        allOff();
        waitForEnterRelease();

        // Set autoInterval              
        doSetAutoInterval();
        
        allOff();
        waitForEnterRelease();  
        
    } else {                      // END: Bail out
      
        allOff();
        waitForEnterRelease();  
        
    }
    
    // Return dest/dept displays to where they should be
    // Menu system wipes out auto times
    if(autoTimeIntervals[autoInterval] == 0) {
        destinationTime.load();
        departedTime.load();
    } else {
        destinationTime.setFromStruct(&destinationTimes[autoTime]);
        departedTime.setFromStruct(&departedTimes[autoTime]);
    }

    // Done, turn off displays
    allOff();

    mydelay(1000);

    #ifdef TC_DBG
    Serial.println("Menu: Update Present Time");
    #endif
    presentTime.setDateTime(rtc.now());  // Set the current time in the display, 2+ seconds gone
    
    // all displays on and show
    animate();  // show all with month showing last
                // then = millis(); // start count to prevent double animate if it's been too long

    myloop();
    
    waitForEnterRelease();

    #ifdef TC_DBG
    Serial.println("Menu: Exiting....");
    #endif
}

/* 
 *  Cycle through main menu:
 *  Select display to update or mode (bri, autoInt, End)
 *  
 */
void displaySelect(int& number) 
{   
    isEnterKeyHeld = false;

    // Wait for enter
    while(!checkTimeOut()) {
      
      // If pressed
      if(digitalRead(ENTER_BUTTON)) {
        
          // wait for release
          while(!checkTimeOut() && digitalRead(ENTER_BUTTON)) {
              
              myloop();

              // If hold threshold is passed, return false */
              if(isEnterKeyHeld) {
                  isEnterKeyHeld = false;
                  return;              
              }
              delay(100); 
          }

          if(checkTimeOut()) return;
              
          timeout = 0;  // button pressed, reset timeout
          
          number++;
          if(number > MODE_MAX) number = MODE_MIN;

          // Show only the selected display and set pointer to it
          displayHighlight(number);   
        
      } else {

          myloop();
          delay(100);
          
      }
      
    }
}  

/* 
 *  Turns on the currently selected display during setting
 *  Also sets the displaySet pointer to the display being updated
 *  
 */ 
void displayHighlight(int& number) 
{   
    switch (number) {
        case MODE_DEST:  // Destination Time
            destinationTime.on();
            destinationTime.setColon(false);
            destinationTime.show();
            presentTime.off();
            departedTime.off();
            displaySet = &destinationTime;
            break;
        case MODE_PRES:  // Present Time
            destinationTime.off();
            presentTime.on();
            presentTime.setColon(false);
            presentTime.show();
            departedTime.off();
            displaySet = &presentTime;
            break;
        case MODE_DEPT:  // Last Time Departed
            destinationTime.off();
            presentTime.off();
            departedTime.on();
            departedTime.setColon(false);
            departedTime.show();
            displaySet = &departedTime;
            break;
        case MODE_AINT:  // auto enable
            destinationTime.showOnlySettingVal("PRE", -1, true);  // display PRESET, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("SET", -1, true);  
            destinationTime.on();
            presentTime.on();            
            departedTime.off();
            displaySet = NULL;
            break;
        case MODE_BRI:  // Brightness
            destinationTime.showOnlySettingVal("BRI", -1, true);  // display BRIGHT, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("GHT", -1, true);  
            destinationTime.on();
            presentTime.on();            
            departedTime.off();
            displaySet = NULL;
            break;
        case MODE_END:  // end
            destinationTime.showOnlySettingVal("END", -1, true);  // display end, no numbers, clear rest of screen
            destinationTime.on();
            destinationTime.setColon(false);            
            presentTime.off();
            departedTime.off();
            displaySet = NULL;
            break;
    }
}

/*  
 * Show only the field being updated
 * and setup pre-set contents
 * 
 * number = number to show
 * field = field to show it in
 * 
 */ 
void setUpdate(uint16_t& number, int field) 
{        
    switch (field) {
        case FIELD_MONTH:
            displaySet->showOnlyMonth(number);
            break;
        case FIELD_DAY:
            displaySet->showOnlyDay(number);
            break;
        case FIELD_YEAR:
            displaySet->showOnlyYear(number);
            break;
        case FIELD_HOUR:
            displaySet->showOnlyHour(number);
            break;
        case FIELD_MINUTE:
            displaySet->showOnlyMinute(number);
            break;
    }    
}

// Prepare timeBuffer
void prepareInput(uint16_t& number) 
{
    // Write pre-set into buffer, reset index to 0
    // Upon first key input, the pre-set will be overwritten
    // This allows the user to keep the pre-set by pressing
    // enter
    sprintf(timeBuffer, "%d", number);
    resetTimebufIndices(); 
}

/*
 * Update a field of a display using user input
 * 
 * number - a value we're updating (pre-set and result of input)
 * displayNum - the display number being modified
 * field - field being modified, this will be displayed on displayNum as it is updated
 *          0 Destination Time, 1 Present Time, 2 Last Time Departed
 */
void setField(uint16_t& number, uint8_t field, int year = 0, int month = 0) 
{    
    int upperLimit;
    int lowerLimit;
    int i;
    uint16_t setNum = 0, prevNum = number;
    int16_t numVal = 0;

    int numChars = 2;

    bool someupddone = false;

    //timeBuffer[0] = '\0';  // No, timeBuffer is pre-set with previous value

    switch (field) {
        case FIELD_MONTH:
            upperLimit = 12;
            lowerLimit = 1;
            break;
        case FIELD_DAY:
            upperLimit = daysInMonth(month, year);
            lowerLimit = 1;
            break;
        case FIELD_YEAR:
            upperLimit = 9999;
            lowerLimit = 0;           
            numChars = 4;
            break;
        case FIELD_HOUR: 
            upperLimit = 23;
            lowerLimit = 0;
            break;
        case FIELD_MINUTE:  
            upperLimit = 59;
            lowerLimit = 0;
            break;
    }    

    #ifdef TC_DBG        
    Serial.println("setField: Awaiting digits or ENTER...");
    #endif

    // Force keypad to send keys to our buffer
    isSetUpdate = true; 
    
    while( !checkTimeOut() && !digitalRead(ENTER_BUTTON) &&     
              ( (!someupddone && number == prevNum) || strlen(timeBuffer) < numChars) ) {
        
        get_key();      // Why???

        /* Using strlen here means that we always need to start a new number at timebuffer[0]. 
         * This is therefore NOT a ringbuffer!
         */
        setNum = 0;
        for(i = 0; i < strlen(timeBuffer); i++) {
            setNum *= 10;
            setNum += (timeBuffer[i] - '0');             
        }

        // Show setNum in field
        if(prevNum != setNum) {
            someupddone = true;
            setUpdate(setNum, field);                   
            prevNum = setNum;            
        } 

        myloop();    
        delay(50);
                    
        //Serial.print("Setfield: setNum: ");
        //Serial.println(setNum);                 
    }

    // Force keypad to send keys somewhere else but our buffer
    isSetUpdate = false; 
    
    numVal = 0;
    for(i = 0; i < strlen(timeBuffer); i++) {
       numVal *= 10;
       numVal += (timeBuffer[i] - '0');             
    }
    if(numVal < lowerLimit) numVal = lowerLimit;
    if(numVal > upperLimit) numVal = upperLimit;
    number = numVal;
    setNum = numVal;
    
    setUpdate(setNum, field);  

    #ifdef TC_DBG 
    Serial.print("Setfield: number: ");
    Serial.println(number);
    #endif
}  

/* 
 *  Load the autoInterval from Settings in memory (config file is not reloaded)
 *  
 *  Note: The autoInterval is no longer saved to the EEPROM.
 *  It is written to the config file, which is updated accordingly.
 */
bool loadAutoInterval() 
{
    #ifdef TC_DBG
    Serial.println("Load Auto Interval");
    #endif
    
    autoInterval = (uint8_t)atoi(settings.autoRotateTimes);   //EEPROM.read(AUTOINTERVAL_PREF);
    if(autoInterval > 5) {
        autoInterval = 1;  
        Serial.println("loadAutoInterval: Bad autoInterval, resetting to 1");
        return false;
    }
    return true;
}

/* 
 *  Save the autoInterval 
 *  
 *  Note: The autoInterval is no longer saved to the EEPROM.
 *  It is written to the config file, which is updated accordingly.
 */
void saveAutoInterval() 
{
    // Convert 'autoInterval' to string, write to settings, write settings file
    sprintf(settings.autoRotateTimes, "%d", autoInterval);   
    write_settings();
    
    // Obsolete    
    //EEPROM.write(AUTOINTERVAL_PREF, autoInterval);
    //EEPROM.commit();
}

/*
 * Adjust the autoInverval and save
 */
void doSetAutoInterval() 
{
    bool autoDone = false;

    #ifdef TC_DBG
    Serial.println("doSetAutoInterval() involked");
    #endif
    
    destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);
    destinationTime.on();

    presentTime.on();
    departedTime.on();
    if(autoTimeIntervals[autoInterval] == 0) {
        presentTime.showOnlySettingVal("CUS", -1, true);    // Custom times to be shown
        departedTime.showOnlySettingVal("TOM", -1, true);   
    } else {       
        presentTime.showOnlySettingVal("MIN", -1, true);    // Times cycled in xx minutes
        departedTime.showOnlyUtes();        
    }

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !autoDone) {
      
      // If pressed
      if(digitalRead(ENTER_BUTTON)) {
        
          // wait for release
          while(!checkTimeOut() && digitalRead(ENTER_BUTTON)) {
              // If hold threshold is passed, return false */
              myloop();
              if(isEnterKeyHeld) {
                  isEnterKeyHeld = false;
                  autoDone = true;
                  break;              
              }
              delay(10); 
          }

          if(!checkTimeOut() && !autoDone) {
              
              timeout = 0;  // button pressed, reset timeout

              autoInterval++;       
              if(autoInterval > 4)
                  autoInterval = 0;

              destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);

              if(autoTimeIntervals[autoInterval] == 0) {
                  presentTime.showOnlySettingVal("CUS", -1, true);
                  departedTime.showOnlySettingVal("TOM", -1, true);                  
              } else {                  
                  presentTime.showOnlySettingVal("MIN", -1, true);
                  departedTime.showOnlyUtes();  
              }
          }
          
      } else {

          myloop();
          delay(100);
          
      }
          
    }

    if(!checkTimeOut()) {  // only if there wasn't a timeout
      
        presentTime.off();
        departedTime.off();

        destinationTime.showOnlySave();

        // Save it
        saveAutoInterval();
        
        mydelay(1000);
                
    }
}

/*
 * Adjust the brightness of the selected displaySet and save
 */
void doSetBrightness(clockDisplay* displaySet) {
  
    int8_t number = displaySet->getBrightness();
    bool briDone = false;

    #ifdef TC_DBG
    Serial.println("Set Brightness");
    #endif

    // turn on all the segments
    allLampTest();  

    // Display "LVL"
    displaySet->showOnlySettingVal("LVL", displaySet->getBrightness() / 3, false);
  
    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !briDone) {     
      
      // If pressed
      if(digitalRead(ENTER_BUTTON)) {
        
          // wait for release
          while(!checkTimeOut() && digitalRead(ENTER_BUTTON)) {
              // If hold threshold is passed, return false */
              myloop();
              if(isEnterKeyHeld) {
                  isEnterKeyHeld = false;
                  briDone = true;
                  break;              
              }
              delay(10); 
          }

          if(!checkTimeOut() && !briDone) {
              
              timeout = 0;  // button pressed, reset timeout

              number = number + 3;
              if(number > 15) number = 3;
              displaySet->setBrightness(number);
              displaySet->showOnlySettingVal("LVL", number / 3, false);
              
          }
          
      } else {
        
          myloop();
          delay(100);
          
      }
          
    }

    if(!checkTimeOut()) {  // only if there wasn't a timeout 
                
        displaySet->showOnlySave();

        displaySet->save();
        
        mydelay(1000);        
        
        allLampTest();  // turn on all the segments

        // Convert bri values to strings, write to settings, write settings file
        sprintf(settings.destTimeBright, "%d", destinationTime.getBrightness());           
        sprintf(settings.presTimeBright, "%d", presentTime.getBrightness());
        sprintf(settings.lastTimeBright, "%d", departedTime.getBrightness());        
        write_settings();
    }

    waitForEnterRelease();
}

// Show all, month after a short delay
void animate() 
{    
    destinationTime.showAnimate1();
    presentTime.showAnimate1();
    departedTime.showAnimate1();
    delay(80);
    destinationTime.showAnimate2();
    presentTime.showAnimate2();
    departedTime.showAnimate2();
}

// Activate lamp test on all displays and turn on
void allLampTest() 
{    
    destinationTime.on();
    presentTime.on();
    departedTime.on();
    destinationTime.lampTest();
    presentTime.lampTest();
    departedTime.lampTest();
}

void allOff() 
{
    destinationTime.off();
    presentTime.off();
    departedTime.off();
}

/* 
 * Wait for ENTER to be released 
 * Call myloop() to allow other stuff to proceed
 * 
 */
void waitForEnterRelease() 
{    
    // Would be nice to activate eg the green LED here
    // to signal the user to release the button....
    
    while(digitalRead(ENTER_BUTTON)) {
        myloop();        
        delay(100);   // wait for release
    }
    isEnterKeyPressed = false;
    isEnterKeyHeld = false;
    isEnterKeyDouble = false;
    return;
}

/*
 * MyDelay: For delays > 150ms
 * Calls myloop() periodically
 */
void mydelay(int mydel) 
{  
    while(mydel >= 150) {
        delay(100);
        mydel -= 120;
        myloop();
    }  
    delay(mydel);   
}

/*
 *  Do this during enter_menu whenever we are cought in a while() loop
 *  This allows other stuff to proceed
 */
void myloop() 
{
    //keypadLoop();   No, interferes with our menu
    enterkeytick();    
    //time_loop();    No, interferes with our menu
    wifi_loop();
    audio_loop();     
}