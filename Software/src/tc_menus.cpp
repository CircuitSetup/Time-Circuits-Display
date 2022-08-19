/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
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
 *    - set the alarm ("ALA-RM")
 *    - select the Time-Rotation Interval ("ROT-INT")
 *    - select the brightness for the three displays ("BRI-GHT")
 *    - view network data, ie IP address ("NET-WRK")
 *    - quit the menu ("END")
 *  Pressing ENTER cycles through the list, holding ENTER selects an item, ie a mode.
 *  How to to enter custom dates/times or set the RTC (real time clock):
 *      - Note that the pre-set for destination and last time departed will be the previous 
 *        date/time stored in the EEPROM, not the date/time that was shown before entering 
 *        the menu. The pre-set for the present time display is the time stored in the RTC 
 *        (real time clock).
 *      - the field to enter data into is shown (exclusively)
 *      - data entry works as follows: 
 *        ) a pre-set is shown in the field
 *        ) if you want to keep this pre-set, press ENTER to proceed to next field.
 *        ) if you enter a digit, the pre-set is overwritten
 *        ) if you enter 2 (for year: 4) digits, the menu proceeds to the next field
 *        ) if you enter less than the maximum digits, you can press ENTER to proceed
 *          to the next field.
 *        Note that the month needs to be entered numerically, and the hour needs to be entered in 24
 *        hour mode, which is then shown in 12 hour mode (if this mode is active).
 *      - After entering data into all fields, the data is saved and the menu is left automatically.
 *      - Note that after entering dates/times into the "destination" or "last time departed" displays,
 *        the time-rotation interval is set to 0 and your entered date/time(s) are shown permanently 
 *        (see below).
 *      - If you entered a date/time into the "present" time display, this time is then stored to
 *        the RTC and used as actual the present time, and continues to run like a clock. (As opposed 
 *        to the "destination" and "last departure" times, which are stale.) If you don't have NTP
 *        (network time) available, this is the way to set the clock to actual present time. If the 
 *        clock is connected to the the network, your entered time will eventually be overwritten
 *        by the time received over the network.
 *  How to set the alarm:
 *      - First option is "on" of "off". Press ENTER to toggle, hold ENTER to proceed.
 *      - Next, enter the hour in 24-hour-format. This works in the same way as described above.
 *      - Next, enter the minute.
 *      - "SAVE" is shown briefly, the alarm is saved.
 *      Note that the alarm is a recurring alarm; it will sound every day at the programmed time,
 *      unless switched off through the menu.
 *      The alarm base is configurable in the WiFi-accessible Setup page. It can either be RTC
 *      (ie the actual present time), or "present time", ie whatever is currently displayed in 
 *      present time. 
 *  How to select the Time Rotation Interval (display shows "INT")
 *      - Press ENTER to cycle through the possible settings.
 *      - Hold ENTER for 2 seconds to select the shown value and exit the menu ("SAVE" is displayed briefly)
 *      - 0 makes your custom "destination" and "last time departed" times to be shown permanently.
 *        (CUS-TOM is displayed as a reminder)
 *      - Non-zero values make the clock cycle through a number of pre-programmed times, your
 *        custom times are ignored. The value means "minutes" (hence "MIN-UTES")               
 *  How to select display brightness (display shows "LVL")
 *      - Press ENTER to cycle through the possible levels (1-5)
 *      - Hold ENTER for 2 seconds to use current value and jump to next display
 *      - After the third display, "SAVE" is displayed briefly and the menu is left automatically.
 *  How to display the current IP address ("NETWRK")
 *      - the bottom two displays show the current IP address of the device. If this is 192.168.4.1,
 *        the device very likely could not connect to your WiFi network and runs in AP mode ("TCD-AP")
 *      - Hold ENTER to quit the menu
 *  How to quit the menu ("END")
 *      - Hold ENTER to quit the menu
 */

#include "tc_menus.h"

int menuItemNum;                                               

// array element of autoTimeIntervals[], set's time between automatically displayed times
uint8_t autoInterval = 1;                                     
const uint8_t autoTimeIntervals[6] = {0, 5, 15, 30, 45, 60};  // first must be 0 (=off)

bool isSetUpdate = false;
bool isYearUpdate = false;

clockDisplay* displaySet;  // the current display

bool    alarmOnOff = false;
uint8_t alarmHour = 255;
uint8_t alarmMinute = 255;

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
void enter_menu() 
{
    bool desNM = destinationTime.getNightMode();
    bool preNM = presentTime.getNightMode();
    bool depNM = departedTime.getNightMode();
    
    #ifdef TC_DBG
    Serial.println("Menu: enter_menu() invoked");
    #endif
    
    isEnterKeyHeld = false;     
    isEnterKeyPressed = false; 

    destinationTime.setNightMode(false);
    presentTime.setNightMode(false);
    departedTime.setNightMode(false);

    stopAudio();
    
    // start with destination time in main menu
    menuItemNum = MODE_DEST;     

    // Load the custom times from EEPROM
    // This means that when the user activates the menu while 
    // autoInterval was > 0, there will be different times
    // shown in the menu than were outside the menu    
    destinationTime.load();     
    departedTime.load();

    // Load the RTC time into present time
    presentTime.setDateTime(myrtcnow());     

    // Show first menu item
    menuShow(menuItemNum);

    waitForEnterRelease();

    timeout = 0;  

    // menuSelect: 
    // Wait for ENTER to cycle through main menu (displays and modes), 
    // HOLDing ENTER selects current menu "item"
    // (Also sets displaySet to selected display, if applicable)    
    menuSelect(menuItemNum);  

    if(checkTimeOut()) 
        goto quitMenu;

    #ifdef TC_DBG
    Serial.println("Menu: Display/mode selected");    
    #endif

    if(menuItemNum <= MODE_DEPT) {  

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
            DateTime currentTime = myrtcnow(); 
            yearSet = currentTime.year() - displaySet->getYearOffset();            
            monthSet = currentTime.month();
            daySet = currentTime.day();
            minSet = currentTime.minute();
            hourSet = currentTime.hour();
            
        } else {
          
            // non RTC, get the time info from the object
            // Remember: These are the ones saved in the EEPROM
            // NOT the ones that were possibly shown on the 
            // display while autoInterval was > 0.
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
        
        if(checkTimeOut()) 
            goto quitMenu;         
        
        // Get month
        setUpdate(monthSet, FIELD_MONTH);
        prepareInput(monthSet);
        waitForEnterRelease();
        setField(monthSet, FIELD_MONTH, 0, 0); 

        if(checkTimeOut()) 
            goto quitMenu;

        // Get day
        setUpdate(daySet, FIELD_DAY);
        prepareInput(daySet);
        waitForEnterRelease();        
        setField(daySet, FIELD_DAY, yearSet, monthSet);  // this depends on the month and year        

        if(checkTimeOut()) 
            goto quitMenu;
            
        // Get hour
        setUpdate(hourSet, FIELD_HOUR);
        prepareInput(hourSet);
        waitForEnterRelease();       
        setField(hourSet, FIELD_HOUR, 0, 0); 

        if(checkTimeOut()) 
            goto quitMenu;

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
    
                rtc.adjust(DateTime(tyr, monthSet, daySet, hourSet, minSet, 0));

                // Resetting the RTC invalidates our timeDifference, ie
                // fake present time. Make the user return to present
                // time after setting the RTC
                timeDifference = 0;
                
            } else {                 

                // Non-RTC: Setting a static display, turn off autoInverval

                displaySet->setYearOffset(0);
                
                autoInterval = 0;    
                saveAutoInterval();  
                
            }

            // Show a save message for a brief moment
            displaySet->showOnlySave();  

            waitAudioDone();
            
            // update the object
            displaySet->setMonth(monthSet);
            displaySet->setDay(daySet);
            displaySet->setYear(yearSet);
            displaySet->setHour(hourSet);
            displaySet->setMinute(minSet);
            
            // save to eeprom (regardless of persistence mode)
            displaySet->save();  
            
            mydelay(1000);

            allOff();
            waitForEnterRelease();
            
        }  

    } else if(menuItemNum == MODE_ALRM) {

        allOff();
        waitForEnterRelease();

        // Set alarm              
        doSetAlarm();

        allOff();
        waitForEnterRelease(); 

    } else if(menuItemNum == MODE_AINT) {  // Select autoInterval

        allOff();
        waitForEnterRelease();

        // Set autoInterval              
        doSetAutoInterval();
        
        allOff();
        waitForEnterRelease();  
        
    } else if(menuItemNum == MODE_BRI) {   // Adjust brightness

        allOff();
        waitForEnterRelease();
      
        // Set brightness settings    
        if(!checkTimeOut()) doSetBrightness(&destinationTime);
        if(!checkTimeOut()) doSetBrightness(&presentTime);
        if(!checkTimeOut()) doSetBrightness(&departedTime);
        
        allOff();
        waitForEnterRelease();  
        
    } else if(menuItemNum == MODE_NET) {   // Show network info

        allOff();
        waitForEnterRelease();

        // Show net info              
        doShowNetInfo();
        
        allOff();
        waitForEnterRelease();  
    
    } else {                              // END: Bail out
      
        allOff();
        waitForEnterRelease();  
        
    }

quitMenu:

    isSetUpdate = false;
    
    // Return dest/dept displays to where they should be    
    if(autoTimeIntervals[autoInterval] == 0 || checkIfAutoPaused()) {
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
    presentTime.setDateTimeDiff(myrtcnow()); // Set the current time in the display, 2+ seconds gone
    
    // all displays on and show  
    
    animate(); 
                    
    myloop();
    
    waitForEnterRelease();

    destinationTime.setNightMode(desNM);
    presentTime.setNightMode(preNM);
    departedTime.setNightMode(depNM);

    #ifdef TC_DBG
    Serial.println("Menu: Exiting....");
    #endif
}

/* 
 *  Cycle through main menu:
 *  Select display to update, or mode (alarm, rot-Int, bri, ...)
 *  
 */
void menuSelect(int& number) 
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
              delay(10); 
          }

          if(checkTimeOut()) return;
              
          timeout = 0;  // button pressed, reset timeout
          
          number++;
          if(number > MODE_MAX) number = MODE_MIN;

          // Show only the selected display, or menu item text
          menuShow(number);   
        
      } else {

          myloop();
          delay(50);
          
      }
      
    }
}  

/* 
 *  Shows the currently selected menu item, ie it turns on the currently 
 *  selected display, or shows descriptive text
 *  
 */ 
void menuShow(int& number) 
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
        case MODE_PRES:  // Present Time (RTC)
            destinationTime.showOnlyRTC();
            destinationTime.on();
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
        case MODE_ALRM:   // Alarm
            #ifdef IS_ACAR_DISPLAY
            destinationTime.showOnlySettingVal("AL", -1, true);  
            presentTime.showOnlySettingVal("RM", -1, true);  
            #else
            destinationTime.showOnlySettingVal("ALA", -1, true);  // display ALA-RM, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("RM", -1, true);  
            #endif
            destinationTime.on();
            presentTime.on();            
            departedTime.off();
            displaySet = &destinationTime;
            break;
        case MODE_AINT:  // autoInterval
            #ifdef IS_ACAR_DISPLAY
            destinationTime.showOnlySettingVal("IN", -1, true);
            presentTime.showOnlySettingVal("T", -1, true); 
            #else
            destinationTime.showOnlySettingVal("ROT", -1, true);  // display ROT-INT, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("INT", -1, true);  
            #endif
            destinationTime.on();
            presentTime.on();            
            departedTime.off();
            displaySet = NULL;
            break;
        case MODE_BRI:  // Brightness
            #ifdef IS_ACAR_DISPLAY
            destinationTime.showOnlySettingVal("BR", -1, true); 
            presentTime.showOnlySettingVal("I", -1, true); 
            #else
            destinationTime.showOnlySettingVal("BRI", -1, true);  // display BRI-GHT, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("GHT", -1, true);  
            #endif
            destinationTime.on();
            presentTime.on();            
            departedTime.off();
            displaySet = NULL;
            break;
        case MODE_NET:  // Network info
            #ifdef IS_ACAR_DISPLAY
            destinationTime.showOnlySettingVal("IP", -1, true); 
            destinationTime.on(); 
            presentTime.off(); 
            #else
            destinationTime.showOnlySettingVal("NET", -1, true);  // display NET-WRK, no numbers, clear rest of screen
            presentTime.showOnlySettingVal("WRK", -1, true);  
            destinationTime.on();
            presentTime.on();    
            #endif                    
            departedTime.off();
            displaySet = NULL;
            break;
        case MODE_END:  // end
            #ifdef IS_ACAR_DISPLAY
            destinationTime.showOnlySettingVal("EN", -1, true);
            #else
            destinationTime.showOnlySettingVal("END", -1, true);  // display END, no numbers, clear rest of screen
            #endif
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
 * field - field being modified, this will be displayed as it is updated
 * year, month - check checking maximum day number
 *          
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
            lowerLimit = 1;           
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
        
        get_key();      // We're outside our main loop, so poll here

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
        delay(10);
                                   
    }

    // Force keypad to send keys somewhere else but our buffer
    isSetUpdate = false; 

    if(checkTimeOut())
        return;
    
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

void alarmOff()
{
    alarmOnOff = false;
    saveAlarm();
}

bool alarmOn()
{
    if(alarmHour <= 23 && alarmMinute <= 59) {
        alarmOnOff = true;
        saveAlarm();       
    } else {
        return false;
    }

    return true;
}

// Set Alarm            
void doSetAlarm() 
{
    bool alarmDone = false;

    bool newAlarmOnOff = alarmOnOff;
    uint16_t newAlarmHour = (alarmHour <= 23) ? alarmHour : 0;
    uint16_t newAlarmMinute = (alarmMinute <= 59) ? alarmMinute : 0;
    
    #ifdef TC_DBG
    Serial.println("doSetAlarm() involked");
    #endif

    // On/Off
    #ifdef IS_ACAR_DISPLAY
    displaySet->showOnlySettingVal(newAlarmOnOff ? "ON" : "OF", -1, true);
    #else
    displaySet->showOnlySettingVal(newAlarmOnOff ? "ON" : "OFF", -1, true);
    #endif
    displaySet->on();
    
    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {
      
      // If pressed
      if(digitalRead(ENTER_BUTTON)) {
        
          // wait for release
          while(!checkTimeOut() && digitalRead(ENTER_BUTTON)) {
              // If hold threshold is passed, return false */
              myloop();
              if(isEnterKeyHeld) {
                  isEnterKeyHeld = false;
                  alarmDone = true;
                  break;              
              }
              delay(10); 
          }

          if(!checkTimeOut() && !alarmDone) {
              
              timeout = 0;  // button pressed, reset timeout

              newAlarmOnOff = !newAlarmOnOff;       

              #ifdef IS_ACAR_DISPLAY
              displaySet->showOnlySettingVal(newAlarmOnOff ? "ON" : "OF", -1, true);
              #else
              displaySet->showOnlySettingVal(newAlarmOnOff ? "ON" : "OFF", -1, true);
              #endif
              
          }
          
      } else {

          myloop();
          delay(50);
          
      }
          
    }

    if(checkTimeOut()) {
        return;  
    }

    // Get hour
    setUpdate(newAlarmHour, FIELD_HOUR);
    prepareInput(newAlarmHour);
    waitForEnterRelease();       
    setField(newAlarmHour, FIELD_HOUR, 0, 0); 

    if(checkTimeOut()) {
        return;  
    }

    // Get minute
    setUpdate(newAlarmMinute, FIELD_MINUTE);
    prepareInput(newAlarmMinute);
    waitForEnterRelease();       
    setField(newAlarmMinute, FIELD_MINUTE, 0, 0);  

    // Do nothing if there was a timeout waiting for button presses                                                  
    if(!checkTimeOut()) {
    
        displaySet->showOnlySave();

        waitAudioDone();

        alarmOnOff = newAlarmOnOff;
        alarmHour = newAlarmHour;
        alarmMinute = newAlarmMinute;

        // Save it
        saveAlarm();
        
        mydelay(1000);
    }
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
    
    autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
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
 *  It is written to the config file, which is updated accordingly.
 */
void saveAutoInterval() 
{
    // Convert 'autoInterval' to string, write to settings, write settings file
    sprintf(settings.autoRotateTimes, "%d", autoInterval);   
    write_settings();
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

    #ifdef IS_ACAR_DISPLAY
    destinationTime.showOnlySettingVal("IN", autoTimeIntervals[autoInterval], true);
    #else
    destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);
    #endif
    destinationTime.on();

    presentTime.on();
    departedTime.on();
    if(autoTimeIntervals[autoInterval] == 0) {
        #ifdef IS_ACAR_DISPLAY
        presentTime.showOnlySettingVal("CU", -1, true);    // Custom times to be shown
        departedTime.showOnlySettingVal("ST", -1, true);  
        #else
        presentTime.showOnlySettingVal("CUS", -1, true);    // Custom times to be shown
        departedTime.showOnlySettingVal("TOM", -1, true);   
        #endif
    } else {       
        #ifdef IS_ACAR_DISPLAY
        presentTime.showOnlyMin();
        departedTime.showOnlyUtes();
        #else
        presentTime.showOnlySettingVal("MIN", -1, true);    // Times cycled in xx minutes
        departedTime.showOnlyUtes();
        #endif        
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
              if(autoInterval > 5)
                  autoInterval = 0;

              #ifdef IS_ACAR_DISPLAY
              destinationTime.showOnlySettingVal("IN", autoTimeIntervals[autoInterval], true);
              #else
              destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);
              #endif

              if(autoTimeIntervals[autoInterval] == 0) {
                  #ifdef IS_ACAR_DISPLAY
                  presentTime.showOnlySettingVal("CU", -1, true); 
                  departedTime.showOnlySettingVal("ST", -1, true);  
                  #else
                  presentTime.showOnlySettingVal("CUS", -1, true);
                  departedTime.showOnlySettingVal("TOM", -1, true);                  
                  #endif
              } else {                  
                  #ifdef IS_ACAR_DISPLAY
                  presentTime.showOnlyMin();
                  departedTime.showOnlyUtes();
                  #else
                  presentTime.showOnlySettingVal("MIN", -1, true);
                  departedTime.showOnlyUtes();  
                  #endif
              }
          }
          
      } else {

          myloop();
          delay(50);
          
      }
          
    }

    if(!checkTimeOut()) {  // only if there wasn't a timeout
      
        presentTime.off();
        departedTime.off();

        destinationTime.showOnlySave();

        // Save it
        saveAutoInterval();
        updateConfigPortalValues();
        
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
    #ifdef IS_ACAR_DISPLAY
    displaySet->showOnlySettingVal("LV", displaySet->getBrightness(), false);
    #else
    displaySet->showOnlySettingVal("LVL", displaySet->getBrightness(), false);
    #endif
  
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

              number++;
              if(number > 15) number = 0;
              displaySet->setBrightness(number);
              #ifdef IS_ACAR_DISPLAY
              displaySet->showOnlySettingVal("LV", number, false);
              #else
              displaySet->showOnlySettingVal("LVL", number, false);
              #endif
              
          }
          
      } else {
        
          myloop();
          delay(50);
          
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
        updateConfigPortalValues();
    }

    waitForEnterRelease();
}

/*
 * Show network info
 * 
 */
void doShowNetInfo() 
{
    uint8_t a, b, c, d;
    //int mymode;
    bool netDone = false;
    
    #ifdef TC_DBG
    Serial.println("doShowNetInfo() involked");
    #endif

    //mymode = wifi_getmode();
    
    wifi_getIP(a, b, c, d);

    #ifdef IS_ACAR_DISPLAY
    destinationTime.showOnlySettingVal("IP", a, true);
    #else
    destinationTime.showOnlySettingVal("IP", a, true);
    #endif
    destinationTime.on();

    presentTime.showOnlyHalfIP(a, b, true);
    presentTime.on();

    departedTime.showOnlyHalfIP(c, d, true);
    departedTime.on();
    
    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !netDone) {
      
        // If pressed
        if(digitalRead(ENTER_BUTTON)) {
          
            // wait for release
            while(!checkTimeOut() && digitalRead(ENTER_BUTTON)) {
                // If hold threshold is passed, bail out
                myloop();
                if(isEnterKeyHeld) {
                    isEnterKeyHeld = false; 
                    netDone = true;                 
                    break;              
                }
                delay(10); 
            }
            
        } else {
  
            myloop();
            delay(50);
            
        }
          
    }
}

// Show all, month after a short delay
void animate() 
{    
    destinationTime.showAnimate1();
    presentTime.showAnimate1();
    departedTime.showAnimate1();
    mysdelay(80); //delay(80);
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
    while(digitalRead(ENTER_BUTTON)) {
        myloop();        
        delay(10);  
    }
    isEnterKeyPressed = false;
    isEnterKeyHeld = false;
}

void waitAudioDone()
{
  int timeout = 100;
  
  while(!checkAudioDone() && timeout--) {       
       myloop();
       delay(10);
  }
}

/*
 * MyDelay: For delays > 150ms
 * Calls myloop() periodically
 */
void mydelay(int mydel) 
{  
    unsigned long startNow = millis();
    while(millis() - startNow < mydel) {
        delay(20);
        myloop();
    }     
}

void mysdelay(int mydel) 
{  
    unsigned long startNow = millis();
    while(millis() - startNow < mydel) {
        delay(5);
        myloop();
    }     
}

/*
 *  Do this during enter_menu whenever we are caught in a while() loop
 *  This allows other stuff to proceed
 */
void myloop() 
{
    enterkeytick();       
    wifi_loop();
    audio_loop();     
}
