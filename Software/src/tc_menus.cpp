/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Keypad Menu handling
 *
 * Based on code by John Monaco, Marmoset Electronics
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

/*
 * The keypad menu
 *
 * The menu is controlled by "pressing" or "holding" the ENTER key on the 
 * keypad.
 *
 * A "press" is shorter than 2 seconds, a "hold" is 2 seconds or longer.
 * 
 * Data entry is done by pressing the keypad's number keys, and works as 
 * follows: 
 * If you want to keep the currently shown pre-set, press ENTER to proceed 
 * to next field. Otherwise press a digit on the keypad; the pre-set is then 
 * overwritten by the value entered. 2 digits can be entered (4 for years), 
 * upon which the current value is stored and the next field is activated. 
 * You can also enter less than 2/4 digits and press ENTER when done with 
 * the field.
 * Note that the month needs to be entered numerically (1-12), and the hour 
 * needs to be entered in 24 hour notation (ie from 0 to 23), regardless of 
 * 12-hour or 24-hour mode as per the Config Portal setting.
 *
 * The menu is invoked by holding the ENTER button.
 *
 * First step is to choose a menu item. The available "items" are
 *
 *     - set an alarm ("ALA-RM"),
 *     - set the audio volume (VOL-UME),
 *     - select the Time-rotation Interval ("TIME ROTATION INTERVAL"),
 *     - select the brightness for the three displays ("BRIGHTNESS"),
 *     - show network information ("NET-WORK"),
 *     - enter dates/times for the three displays,
 *     - install the default audio files ("INSTALL AUDIO FILES")
 *     - quit the menu ("END").
 *
 * Pressing ENTER cycles through the list, holding ENTER selects an item.
 *
 * How to set up the alarm:
 *
 *     - Hold ENTER to invoke main menu
 *     - "ALA-RM" is shown
 *     - Hold ENTER
 *     - Press ENTER to toggle the alarm on and off, hold ENTER to proceed
 *     - Then enter the hour and minutes. This works as described above.
 *       Remember to enter the hour in 24-hour notation.
 *     - Select the weekday(s); press ENTER to cycle through the list of
 *       available options.
 *     - Hold ENTER. "SAVING" is displayed briefly.
 *
 *    Under normal operation (ie outside of the menu), holding "1" enables 
 *    the alarm, holding "2" disables it.
 *
 *    Note that the alarm is recurring, ie it rings at the programmed time
 *    unless disabled. Also note, as mentioned, that the alarm is by default
 *    relative to your actual present time, not the time displayed (eg after
 *    a time travel). It can, however, be configured to be based on the time
 *    displayed, in the Config Portal.
 *
 *    If the alarm is set and enabled, the dot in the present time's minute
 *    field is lit.
 *
 * How to set the audio volume:
 *
 *     Basically, and by default, the device uses the hardware volume knob to 
 *     determine the desired volume. You can change this to a software setting 
 *     as follows:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "VOL-UME" is shown
 *     - Hold ENTER
 *     - Press ENTER to toggle between "HW" (volume knob) or "SW" (software)
 *     - Hold ENTER to proceed
 *     - If you chose "SW", you can now select the desired level by pressing
 *       ENTER repeatedly. There are 16 levels available.
 *     - Hold ENTER to save and quit the menu
 *
 * How to select the Time-rotation Interval:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "TIME ROTATION INTERVAL" is shown
 *     - Hold ENTER, "INT" is displayed
 *     - Press ENTER to cycle through the possible Time-rotation Interval 
 *       values.
 *
 *       A value of 0 disables automatic time cycling ("OFF").
 *
 *       Non-zero values make the device cycle through a number of pre-
 *       programmed times. The value means "minutes" (hence "MINUTES") between 
 *       changes.
 *
 *     - Hold ENTER to select the value shown and exit the menu ("SAVING" is 
 *       displayed briefly)
 *
 * How to adjust the display brightness:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "BRIGHTNESS" is shown
 *     - Hold ENTER, the displays show all elements, the top-most display 
 *       says "LVL"
 *     - Press ENTER to cycle through the possible levels (1-5)
 *     - Hold ENTER to use current value and proceed to next display
 *     - After the third display, "SAVING" is displayed briefly and the menu 
 *       is left automatically.
 *
 * How to find out the IP address and WiFi status:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "NET-WORK" is shown
 *     - Hold ENTER, the displays shows the IP address
 *     - Press ENTER to view the WiFi status
 *     - Hold ENTER to leave the menu
 *
 * How to enter dates/times for the three displays / set the RTC:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until the desired display is the only one lit
 *     - Hold ENTER until the display goes off except for the first field 
 *       to enter data into
 *     - The field to enter data into is shown (exclusively), pre-set with
 *       its current value.
 *     - Data entry works as described above.
 *       Remember that months need to be entered numerically (1-12), and 
 *       hours in 24 hour notation, regardless of 12-hour or 24-hour mode.
 *     - After entering data into all fields, the data is saved and the menu 
 *       is left automatically.
 *
 *     By entering a date/time into the present time display, the RTC (real 
 *     time clock) of the device is adjusted, which is useful if you can't 
 *     use NTP for time keeping. The time you entered will be overwritten 
 *     if/when the device has access to network time via NTP, or time from 
 *     GPS. Don't forget to configure your time zone in the Config Portal.
 *     
 *     Note that when entering dates/times into the destination time or last 
 *     time departed displays, the Time-rotation Interval is automatically 
 *     set to 0 (ie turned off). Your entered date/time(s) are shown until 
 *     overwritten by time travels (see above, section How to select the 
 *     Time-rotation Interval").
 *
 * How to install the default audio files:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "INSTALL AUDIO FILES" is shown. If this menu does 
 *       not appear, the SD card isn't configured properly.
 *     - Hold ENTER to proceed
 *     - Press ENTER to toggle between "CANCEL" and "COPY"
 *     - Hold ENTER to proceed. If "COPY" was chosen, the display will guide 
 *       you through the rest of the process. When finished, the clock will
 *       reboot.
 *
 * How to leave the menu:
 *
 *     While the menu is active, repeatedly press ENTER until "END" is displayed.
 *     Hold ENTER to leave the menu
 */

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"
#include "tc_wifi.h"

#include "tc_menus.h"

#define MODE_ALRM 0
#define MODE_VOL  1
#define MODE_AINT 2
#define MODE_BRI  3
#define MODE_NET  4
#define MODE_DEST 5
#define MODE_PRES 6
#define MODE_DEPT 7
#define MODE_VER  8
#define MODE_CPA  9
#define MODE_END  10
#define MODE_MIN  MODE_ALRM
#define MODE_MAX  MODE_END

#define FIELD_MONTH   0
#define FIELD_DAY     1
#define FIELD_YEAR    2
#define FIELD_HOUR    3
#define FIELD_MINUTE  4

uint8_t autoInterval = 1;
const uint8_t autoTimeIntervals[6] = {0, 5, 10, 15, 30, 60};  // first must be 0 (=off)

bool    alarmOnOff = false;
uint8_t alarmHour = 255;
uint8_t alarmMinute = 255;
uint8_t alarmWeekday = 0;

static int menuItemNum;

bool isSetUpdate = false;
bool isYearUpdate = false;

static clockDisplay* displaySet;

// File copy progress
static bool fcprog = false;
static unsigned long fcstart = 0;

// For tracking second changes
static bool xx;  
static bool yy;

// Our generic timeout when waiting for buttons, in seconds. max 255.
#define maxTime 240
static uint8_t timeout = 0;

static const char *StrSaving = "SAVING";

static void menuSelect(int& number);
static void menuShow(int number);
static void setUpdate(uint16_t number, int field);
static void setField(uint16_t& number, uint8_t field, int year, int month);
static void saveCurVolume();
static bool loadCurVolume();
static void showCurVolHWSW();
static void showCurVol();
static void doSetVolume();
static void doSetAlarm();

static void saveAutoInterval();
static void doSetAutoInterval();
static void doSetBrightness(clockDisplay* displaySet);
static void doShowNetInfo();
static void doCopyAudioFiles();
static void waitForEnterRelease();
static bool checkEnterPress();
static void prepareInput(uint16_t number);

static bool checkTimeOut();

static void myssdelay(unsigned long mydel);
static void myloop();

/*
 * menu_setup()
 *
 */
void menu_setup() {

    // Load volume settings from EEPROM
    loadCurVolume();

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

    pwrNeedFullNow();

    isEnterKeyHeld = false;
    isEnterKeyPressed = false;

    destinationTime.setNightMode(false);
    presentTime.setNightMode(false);
    departedTime.setNightMode(false);

    stopAudio();

    // start with first menu item
    menuItemNum = MODE_MIN;

    // Load the custom times from EEPROM
    // This means that when the user activates the menu while
    // autoInterval was > 0 or after time travels, there will
    // be different times shown in the menu than were outside
    // the menu.
    destinationTime.load();
    departedTime.load();

    // Load the RTC time into present time
    presentTime.setDateTime(myrtcnow());

    // Show first menu item
    menuShow(menuItemNum);

    waitForEnterRelease();

    timeout = 0;

    // menuSelect:
    // Wait for ENTER to cycle through main menu,
    // HOLDing ENTER selects current menu "item"
    // (Also sets displaySet to selected display, if applicable)
    menuSelect(menuItemNum);

    if(checkTimeOut())
        goto quitMenu;

    if(menuItemNum >= MODE_DEST && menuItemNum <= MODE_DEPT) {

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
            // display before invoking the menu
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
        setField(yearSet, FIELD_YEAR, displaySet->isRTC() ? TCEPOCH : 0, 0);
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

            if(displaySet->isRTC()) {  // this is the RTC display, set the RTC

                // Update RTC and fake year if neccessary
                // see comments in tc_time.cpp

                uint16_t newYear = yearSet;
                int16_t tyroffs = 0;

                // Set up DST data for now current year
                if(!(parseTZ(settings.timeZone, yearSet))) {
                    #ifdef TC_DBG
                    Serial.println(F("Menu: Failed to parse TZ"));
                    #endif
                }

                // Get RTC-fit year plus offs for given real year
                correctYr4RTC(newYear, tyroffs);

                displaySet->setYearOffset(tyroffs);

                rtc.adjust(0, minSet, hourSet, dayOfWeek(daySet, monthSet, yearSet), daySet, monthSet, newYear-2000U);

                // User entered current local time; set DST flag to current DST status
                if(couldDST) {
                    int currTimeMins;
                    presentTime.setDST(timeIsDST(yearSet, monthSet, daySet, hourSet, minSet, currTimeMins));
                    // Saved below
                } else {
                    presentTime.setDST(0);
                }

                // Avoid immediate re-adjustment in time_loop
                lastYear = yearSet;

                // Resetting the RTC invalidates our timeDifference, ie
                // fake present time. Make the user return to present
                // time after setting the RTC
                timeDifference = 0;

                // 'newYear' is the year to write to display
                // (where it is corrected by subtracting tyroffs)
                yearSet = newYear;

            } else {

                // Non-RTC: Setting a static display, turn off autoInverval

                displaySet->setYearOffset(0);

                autoInterval = 0;
                saveAutoInterval();

            }

            // Show a save message for a brief moment
            displaySet->showOnlyText(StrSaving);

            waitAudioDone();

            // update the object
            displaySet->setYear(yearSet);
            displaySet->setMonth(monthSet);
            displaySet->setDay(daySet);
            displaySet->setHour(hourSet);
            displaySet->setMinute(minSet);

            // save to eeprom (regardless of persistence mode)
            displaySet->save();

            mydelay(1000);

            allOff();
            waitForEnterRelease();

        }

    } else if(menuItemNum == MODE_VOL) {

        allOff();
        waitForEnterRelease();

        // Set volume
        doSetVolume();

        allOff();
        waitForEnterRelease();

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
        waitForEnterRelease();
        if(!checkTimeOut()) doSetBrightness(&presentTime);
        waitForEnterRelease();
        if(!checkTimeOut()) doSetBrightness(&departedTime);

        // Now save
        if(!checkTimeOut()) {

            presentTime.showOnlyText(StrSaving);

            // (Re)Set current brightness as "initial" brightness
            destinationTime.setBrightness(destinationTime.getBrightness(), true);
            presentTime.setBrightness(presentTime.getBrightness(), true);
            departedTime.setBrightness(departedTime.getBrightness(), true);

            mydelay(1000);

            // Convert bri values to strings, write to settings, write settings file
            sprintf(settings.destTimeBright, "%d", destinationTime.getBrightness());
            sprintf(settings.presTimeBright, "%d", presentTime.getBrightness());
            sprintf(settings.lastTimeBright, "%d", departedTime.getBrightness());
            write_settings();
            updateConfigPortalValues();

        }

        allOff();
        waitForEnterRelease();

    } else if(menuItemNum == MODE_NET) {   // Show network info

        allOff();
        waitForEnterRelease();

        // Show net info
        doShowNetInfo();

        allOff();
        waitForEnterRelease();

    } else if(menuItemNum == MODE_CPA) {   // Copy audio files

        allOff();
        waitForEnterRelease();

        // Copy audio files
        doCopyAudioFiles();

        allOff();
        waitForEnterRelease();

    } else {                              // VERSION, END: Bail out

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

    // Restore present time
    presentTime.setDateTimeDiff(myrtcnow());

    // all displays on and show

    animate();

    myloop();

    waitForEnterRelease();

    // Restore night mode
    destinationTime.setNightMode(desNM);
    presentTime.setNightMode(preNM);
    departedTime.setNightMode(depNM);
}

/*
 *  Cycle through main menu
 *
 */
static void menuSelect(int& number)
{
    isEnterKeyHeld = false;

    // Wait for enter
    while(!checkTimeOut()) {

        // If pressed
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {

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
            if(number == MODE_CPA && !check_allow_CPA()) number++;
            if(number > MODE_MAX) number = MODE_MIN;

            // Show only the selected display, or menu item text
            menuShow(number);

        } else {

            mydelay(50);

        }

    }
}

/*
 *  Displays the current main menu item
 *
 */
static void menuShow(int number)
{
    displaySet = NULL;
    
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
        destinationTime.showOnlyText("RTC");
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
    case MODE_VOL:   // Software volume
        #ifdef IS_ACAR_DISPLAY
        destinationTime.showOnlyText("VOLUME");
        presentTime.off();
        #else
        destinationTime.showOnlyText("VOL");    // "M" on 7seg no good, 2 lines
        presentTime.showOnlyText("UME");
        presentTime.on();
        #endif
        destinationTime.on();
        departedTime.off();
        break;
    case MODE_ALRM:   // Alarm
        #ifdef IS_ACAR_DISPLAY
        destinationTime.showOnlyText("ALARM");
        presentTime.off();
        #else
        destinationTime.showOnlyText("ALA");    // "M" on 7seg no good, 2 lines
        presentTime.showOnlyText("RM");
        presentTime.on();
        #endif
        destinationTime.on();
        departedTime.off();
        displaySet = &destinationTime;
        break;
    case MODE_AINT:  // Time Rotation inverval
        #ifdef IS_ACAR_DISPLAY
        destinationTime.showOnlyText("ROT INT");
        presentTime.off();
        departedTime.off();
        #else
        destinationTime.showOnlyText("TIME");
        presentTime.showOnlyText("ROTATION");
        departedTime.showOnlyText("INTERVAL");
        presentTime.on();
        departedTime.on();
        #endif
        destinationTime.on();
        break;
    case MODE_BRI:  // Brightness
        destinationTime.showOnlyText("BRIGHTNESS");
        presentTime.off();
        departedTime.off();
        destinationTime.on();
        break;
    case MODE_NET:  // Network info
        #ifdef IS_ACAR_DISPLAY
        destinationTime.showOnlyText("NETWORK");
        destinationTime.on();
        presentTime.off();
        #else
        destinationTime.showOnlyText("NET");  // "W" on 7seg no good, 2 lines
        presentTime.showOnlyText("WORK");
        destinationTime.on();
        presentTime.on();
        #endif
        departedTime.off();
        break;
    case MODE_VER:  // Version info
        destinationTime.showOnlyText("VERSION");
        destinationTime.on();
        presentTime.showOnlyText(TC_VERSION);
        presentTime.on();
        #ifdef TC_VERSION_EXTRA
        departedTime.showOnlyText(TC_VERSION_EXTRA);
        departedTime.on();
        #else
        departedTime.off();
        #endif
        break;
    case MODE_CPA:  // Install audio files
        destinationTime.showOnlyText("INSTALL");
        destinationTime.on();
        presentTime.showOnlyText("AUDIO FILES");
        presentTime.on();
        departedTime.off();
        break;
    case MODE_END:  // end
        destinationTime.showOnlyText("END");
        destinationTime.on();
        destinationTime.setColon(false);
        presentTime.off();
        departedTime.off();
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
static void setUpdate(uint16_t number, int field)
{
    switch(field) {
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
static void prepareInput(uint16_t number)
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
static void setField(uint16_t& number, uint8_t field, int year = 0, int month = 0)
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
        lowerLimit = year;
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
    Serial.println(F("setField: Awaiting digits or ENTER..."));
    #endif

    // Force keypad to send keys to our buffer (and block key holding)
    isSetUpdate = true;

    while( !checkTimeOut() && !checkEnterPress() &&
              ( (!someupddone && number == prevNum) || strlen(timeBuffer) < numChars) ) {

        scanKeypad();      // We're outside our main loop, so poll here

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

        mydelay(10);

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
    Serial.print(F("Setfield: number: "));
    Serial.println(number);
    #endif
}

/*
 *  Volume ###################################################
 */

static void saveCurVolume()
{
    EEPROM.write(SWVOL_PREF, (uint8_t)curVolume);
    EEPROM.write(SWVOL_PREF + 1, (uint8_t)((curVolume ^ 0xff) & 0xff));
    EEPROM.commit();
}

static bool loadCurVolume()
{
    uint8_t loadBuf[2];

    loadBuf[0] = EEPROM.read(SWVOL_PREF);
    loadBuf[1] = EEPROM.read(SWVOL_PREF + 1) ^ 0xff;

    if(loadBuf[0] != loadBuf[1]) {

        Serial.println(F("loadVolume: Invalid volume data in EEPROM"));

        curVolume = 255;

        saveCurVolume();

        return false;
    }

    curVolume = loadBuf[0];

    if(curVolume > 15 && curVolume != 255)
        curVolume = 255;

    return true;
}

static void showCurVolHWSW()
{
    if(curVolume == 255) {
        destinationTime.showOnlyText("HW");
        presentTime.showOnlyText("VOLUME");
        departedTime.showOnlyText("KNOB");
        destinationTime.on();
        presentTime.on();
        departedTime.on();
    } else {
        destinationTime.showOnlyText("SW");
        destinationTime.on();
        presentTime.off();
        departedTime.off();
    }
}

static void showCurVol()
{
    #ifdef IS_ACAR_DISPLAY
    destinationTime.showOnlySettingVal("LV", curVolume, true);
    #else
    destinationTime.showOnlySettingVal("LVL", curVolume, true);
    #endif
    destinationTime.on();
    if(curVolume == 0) {
        presentTime.showOnlyText("MUTE");
        presentTime.on();
    } else {
        presentTime.off();
    }
    departedTime.off();
}

static void doSetVolume()
{
    bool volDone = false;
    uint8_t oldVol = curVolume;
    unsigned long playNow;
    bool triggerPlay = false;

    showCurVolHWSW();

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !volDone) {

        // If pressed
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
                // If hold threshold is passed, return false */
                myloop();
                if(isEnterKeyHeld) {
                    isEnterKeyHeld = false;
                    volDone = true;
                    break;
                }
                delay(10);
            }

            if(!checkTimeOut() && !volDone) {

                timeout = 0;  // button pressed, reset timeout

                if(curVolume <= 15)
                    curVolume = 255;
                else
                    curVolume = 0;

                showCurVolHWSW();

            }

        } else {

            mydelay(50);

        }

    }

    if(!checkTimeOut() && curVolume != 255) {

        curVolume = (oldVol == 255) ? 0 : oldVol;

        showCurVol();

        timeout = 0;  // reset timeout

        volDone = false;

        waitForEnterRelease();

        // Wait for enter
        while(!checkTimeOut() && !volDone) {

            // If pressed
            if(checkEnterPress()) {

                // wait for release
                while(!checkTimeOut() && checkEnterPress()) {
                    // If hold threshold is passed, return false */
                    myloop();
                    if(isEnterKeyHeld) {
                        isEnterKeyHeld = false;
                        volDone = true;
                        break;
                    }
                    delay(10);
                }

                if(!checkTimeOut() && !volDone) {

                    timeout = 0;  // button pressed, reset timeout

                    curVolume++;
                    if(curVolume == 16) curVolume = 0;

                    showCurVol();

                    playNow = millis();
                    triggerPlay = true;
                    stopAudio();

                }

            } else {

                if(triggerPlay && (millis() - playNow >= 1*1000)) {
                    play_file("/alarm.mp3", 1.0, true, 0);
                    triggerPlay = false;
                }

                mydelay(50);

            }
        }

    }

    presentTime.off();
    departedTime.off();

    if(!checkTimeOut()) {

        stopAudio();

        destinationTime.showOnlyText(StrSaving);

        // Save it
        saveCurVolume();

        mydelay(1000);

    } else {

        oldVol = curVolume;

    }
}

/*
 *  Alarm ###################################################
 */

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
static void doSetAlarm()
{
    char almBuf[16];
    bool alarmDone = false;
    bool newAlarmOnOff = alarmOnOff;
    uint16_t newAlarmHour = (alarmHour <= 23) ? alarmHour : 0;
    uint16_t newAlarmMinute = (alarmMinute <= 59) ? alarmMinute : 0;
    uint16_t newAlarmWeekday = alarmWeekday;
    const char *alarmWD[10] = {
      #ifdef IS_ACAR_DISPLAY
      "MON-SUN", "MON-FRI", "SAT-SUN",
      #else
      "MON -SUN", "MON -FRI", "SAT -SUN",
      #endif
      "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"
    };
    #ifdef IS_ACAR_DISPLAY
    const char *almFmt = "%s     %02d%02d";
    #else
    const char *almFmt = "%s      %02d%02d";
    #endif

    // On/Off
    sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
    displaySet->showOnlyText(almBuf);
    displaySet->on();

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        // If pressed
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
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

                sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
                displaySet->showOnlyText(almBuf);

            }

        } else {

            mydelay(50);

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

    // Weekday(s)
    waitForEnterRelease();
    displaySet->showOnlyText(alarmWD[newAlarmWeekday]);

    isEnterKeyHeld = false;
    alarmDone = false;

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        // If pressed
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
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

                newAlarmWeekday++;
                if(newAlarmWeekday > 9) newAlarmWeekday = 0;

                displaySet->showOnlyText(alarmWD[newAlarmWeekday]);

            }

        } else {

            mydelay(50);

        }

    }

    // Do nothing if there was a timeout waiting for button presses
    if(!checkTimeOut()) {

        displaySet->showOnlyText(StrSaving);

        waitAudioDone();

        alarmOnOff = newAlarmOnOff;
        alarmHour = newAlarmHour;
        alarmMinute = newAlarmMinute;
        alarmWeekday = newAlarmWeekday;

        // Save it
        saveAlarm();

        mydelay(1000);
    }
}

/*
 *  Time-rotation Interval (aka "autoInterval") #################
 */

/*
 *  Load the autoInterval from Settings in memory (config file is not reloaded)
 *
 */
bool loadAutoInterval()
{
    #ifdef TC_DBG
    Serial.println(F("Load Auto Interval"));
    #endif

    autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
    if(autoInterval > 5) {
        autoInterval = DEF_AUTOROTTIMES;
        Serial.println(F("loadAutoInterval: Bad autoInterval, resetting to default"));
        return false;
    }
    return true;
}

/*
 *  Save the autoInterval
 *
 *  It is written to the config file, which is updated accordingly.
 */
static void saveAutoInterval()
{
    // Convert 'autoInterval' to string, write to settings, write settings file
    sprintf(settings.autoRotateTimes, "%d", autoInterval);
    write_settings();
}

/*
 * Adjust the autoInverval and save
 */
static void doSetAutoInterval()
{
    bool autoDone = false;

    #ifdef IS_ACAR_DISPLAY
    destinationTime.showOnlySettingVal("IN", autoTimeIntervals[autoInterval], true);
    #else
    destinationTime.showOnlySettingVal("INT", autoTimeIntervals[autoInterval], true);
    #endif
    destinationTime.on();

    presentTime.on();
    if(autoTimeIntervals[autoInterval] == 0) {
        presentTime.showOnlyText("OFF");
    } else {
        presentTime.showOnlyText("MINUTES");    // Times cycled in xx minutes
    }
    departedTime.off();

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !autoDone) {

        // If pressed
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
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
                    presentTime.showOnlyText("OFF");
                } else {
                    presentTime.showOnlyText("MINUTES");
                }
                departedTime.off();
            }

        } else {

            mydelay(50);

        }

    }

    if(!checkTimeOut()) {  // only if there wasn't a timeout

        presentTime.off();
        departedTime.off();

        destinationTime.showOnlyText(StrSaving);

        // Save it
        saveAutoInterval();
        updateConfigPortalValues();

        mydelay(1000);

    }
}

/*
 * Brightness ###################################################
 */

static void doSetBrightness(clockDisplay* displaySet) {

    int8_t number = displaySet->getBrightness();
    bool briDone = false;

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
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
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

            mydelay(50);

        }

    }

    allLampTest();  // turn on all the segments
}

/*
 * Show network info ##################################
 */

static void doShowNetInfo()
{
    uint8_t a, b, c, d;
    int number = 0;
    bool netDone = false;

    wifi_getIP(a, b, c, d);

    destinationTime.showOnlyText("IP");
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
        if(checkEnterPress()) {

            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
                // If hold threshold is passed, bail out
                myloop();
                if(isEnterKeyHeld) {
                    isEnterKeyHeld = false;
                    netDone = true;
                    break;
                }
                delay(10);
            }

            if(!checkTimeOut() && !netDone) {

                timeout = 0;  // button pressed, reset timeout

                number++;
                if(number > 1) number = 0;
                switch(number) {
                case 0:
                    wifi_getIP(a, b, c, d);
                    destinationTime.showOnlyText("IP");
                    destinationTime.on();
                    presentTime.showOnlyHalfIP(a, b, true);
                    presentTime.on();
                    departedTime.showOnlyHalfIP(c, d, true);
                    departedTime.on();
                    break;
                case 1:
                    destinationTime.showOnlyText("WIFI");
                    destinationTime.on();
                    switch(wifi_getStatus()) {
                    case WL_IDLE_STATUS:
                        presentTime.showOnlyText("IDLE");
                        departedTime.off();
                        break;
                    case WL_SCAN_COMPLETED:
                        presentTime.showOnlyText("SCAN");
                        departedTime.showOnlyText("COMPLETE");
                        departedTime.on();
                        break;
                    case WL_NO_SSID_AVAIL:
                        presentTime.showOnlyText("SSID NOT");
                        departedTime.showOnlyText("AVAILABLE");
                        departedTime.on();
                        break;
                    case WL_CONNECTED:
                        presentTime.showOnlyText("CONNECTED");
                        departedTime.off();
                        break;
                    case WL_CONNECT_FAILED:
                        presentTime.showOnlyText("CONNECT");
                        departedTime.showOnlyText("FAILED");
                        departedTime.on();
                        break;
                    case WL_CONNECTION_LOST:
                        presentTime.showOnlyText("CONNECTION");
                        departedTime.showOnlyText("LOST");
                        departedTime.on();
                        break;
                    case WL_DISCONNECTED:
                        presentTime.showOnlyText("DISCONNECTED");
                        departedTime.off();
                        break;
                    case 0x10000:
                        presentTime.showOnlyText("AP MODE");
                        departedTime.off();
                        break;
                    case 0x10001:
                        presentTime.showOnlyText("OFF");
                        departedTime.off();
                        break;
                    //case 0x10002:     // UNKNOWN
                    default:
                        presentTime.showOnlyText("UNKNOWN");
                        departedTime.off();
                    }
                    presentTime.on();
                    break;
                }

            }

        } else {

            mydelay(50);

        }

    }
}

/*
 * Install default audio files from SD to flash FS #################
 */

static void doCopyAudioFiles()
{
    bool doCancDone = false;
    bool newCanc = false;

    // Cancel/Copy
    destinationTime.on();
    presentTime.on();
    departedTime.showOnlyText("CANCEL");
    departedTime.on();

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !doCancDone) {

        // If pressed
        if(checkEnterPress()) {
  
            // wait for release
            while(!checkTimeOut() && checkEnterPress()) {
                // If hold threshold is passed, return false */
                myloop();
                if(isEnterKeyHeld) {
                    isEnterKeyHeld = false;
                    doCancDone = true;
                    break;
                }
                delay(10);
            }
  
            if(!checkTimeOut() && !doCancDone) {
  
                timeout = 0;  // button pressed, reset timeout
  
                newCanc = !newCanc;
  
                departedTime.showOnlyText(newCanc ? "COPY" : "CANCEL");
  
            }
  
        } else {
  
            mydelay(50);
  
        }

    }

    if(checkTimeOut() || !newCanc) {
        return;
    }

    if(!copy_audio_files()) {
        mydelay(3000);
    }
    mydelay(2000);

    allOff();
    destinationTime.showOnlyText("REBOOTING");
    destinationTime.on();

    esp_restart();
}

/* *** Helpers ################################################### */

// Show all, month after a short delay
void animate()
{
    destinationTime.showAnimate1();
    presentTime.showAnimate1();
    departedTime.showAnimate1();
    mydelay(80);
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

void start_file_copy()
{
    stopAudio();
  
    destinationTime.on();
    presentTime.on();
    departedTime.on();
    destinationTime.showOnlyText("COPYING");
    presentTime.showOnlyText("FILES");
    departedTime.showOnlyText("PLEASE");
    fcprog = false;
    fcstart = millis();
}

void file_copy_progress()
{
    if(millis() - fcstart >= 1000) {
        departedTime.showOnlyText(fcprog ? "PLEASE" : "WAIT");
        fcprog = !fcprog;
        fcstart = millis();
    }
}

void file_copy_done()
{
    departedTime.showOnlyText("DONE");
}

void file_copy_error()
{
    departedTime.showOnlyText("ERROR");
}

/*
 * Check if ENTER is pressed
 * (and properly debounce)
 */
static bool checkEnterPress()
{
    if(digitalRead(ENTER_BUTTON_PIN)) {
        myssdelay(50);
        if(digitalRead(ENTER_BUTTON_PIN)) return true;
        return false;
    }

    return false;
}

/*
 * Wait for ENTER to be released
 * Call myloop() to allow other stuff to proceed
 *
 */
static void waitForEnterRelease()
{
    while(1) {
        while(digitalRead(ENTER_BUTTON_PIN)) {
            mydelay(10);
        }
        myssdelay(30);
        if(!digitalRead(ENTER_BUTTON_PIN))
            break;
    }
    
    isEnterKeyPressed = false;
    isEnterKeyHeld = false;
}

void waitAudioDone()
{
    int timeout = 200;

    while(!checkAudioDone() && timeout--) {
        mydelay(10);
    }
}

/*
 * Call this frequently while waiting for button presses,
 * increments timeout each second, returns true when 
 * maxtime reached.
 *
 */
static bool checkTimeOut()
{
    yy = digitalRead(SECONDS_IN_PIN);
    if(xx != yy) {
        xx = yy;
        if(yy == 0) {
            timeout++;
        }
    }

    return (timeout >= maxTime);
}

/*
 * MyDelay:
 * Calls myloop() periodically
 */
void mydelay(unsigned long mydel)
{
    unsigned long startNow = millis();
    myloop();
    while(millis() - startNow < mydel) {
        delay(10);
        myloop();
    }
}

/*
 * Special case for very short, non-recurring delays
 * Only calls essential loops while waiting
 */
static void myssdelay(unsigned long mydel)
{
    unsigned long startNow = millis();
    audio_loop();
    while(millis() - startNow < mydel) {
        ntp_short_loop();
        delay(10);
        audio_loop();
    }
}

/*
 *  Do this during enter_menu whenever we are caught in a while() loop
 *  This allows other stuff to proceed
 */
static void myloop()
{
    enterkeyScan();
    wifi_loop();
    audio_loop();
    ntp_loop();
    #ifdef TC_HAVEGPS
    gps_loop();
    #endif
}
