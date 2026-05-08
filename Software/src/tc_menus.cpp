/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Keypad Menu
 *
 * Based on ideas by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
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

/*
 * The keypad menu
 *
 * The menu is controlled by the keypad as follows:
 * 
 * Hold ENTER for 2 seconds to invoke the keypad menu.
 * 
 * Scrolling through lists or increasing/decreasing numerical
 * values is done by pressing 2/8.
 * Selecting an item is done by pressing 5 or ENTER.
 * Pressing 9 cancels and quits without saving any changes. In
 * case of multi-step menus (such as alarm or volume), 9 
 * cancels all changes in previous steps, too.
 * 
 * ***************  ***************  **************
 * **     1     **  **     2     **  **     3    **
 * **           **  **  UP or +  **  **          **
 * **           **  **           **  **          **
 * ***************  ***************  **************
 * 
 * ***************  ***************  **************
 * **     4     **  **     5     **  **     6    **
 * **           **  **  Select   **  **          **
 * **           **  **           **  **          **
 * ***************  ***************  **************
 * 
 * ***************  ***************  **************
 * **     7     **  **     8     **  **     9    **
 * **           **  ** DOWN or - **  **  Cancel/ **
 * **           **  **           **  **    Quit  **
 * ***************  ***************  **************
 * 
 * When the menu expects numeric data to be entered, it displays "TYPE DIGITS". 
 * Data entry is then to be done by pressing the keypad's number keys, and this
 * works as follows: 
 * If you want to keep the currently shown pre-set, press ENTER to proceed 
 * to next field. Otherwise, press a digit on the keypad; the pre-set is then 
 * overwritten by the value entered. 2 digits can be entered (4 for years). 
 * Press ENTER when done.
 * Note that the month needs to be entered numerically (01-12), and the hour 
 * needs to be entered in 24 hour notation (ie from 0 to 23), regardless of 
 * 12-hour or 24-hour mode as set in the Config Portal.
 *
 * How to set up the alarm:
 *
 *     - Hold ENTER to invoke main menu
 *     - "ALARM" is shown
 *     - Press 5 or ENTER
 *     - Press 2/8 to toggle the alarm on and off, press 5 or ENTER to proceed
 *     - Then type the hour and minutes. This works as described above.
 *       Remember to enter the hour in 24-hour notation.
 *     - Select the weekday(s); press 2/8 to cycle through the list of
 *       available options.
 *     - Press 5 or ENTER to select. 
 *     - If "Custom Days" was selected, press 1-7 to toggle days. Press
 *       ENTER when done.
 *     - "SAVING" is displayed briefly.
 *
 * How to set the audio volume:
 *
 *     By default, the TCD uses the hardware volume knob to determine the desired 
 *     volume. You can change this to a fixed level as follows:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "VOLUME" is shown
 *     - Press 5 or ENTER
 *     - Press 2/8 to toggle between "USE VOLUME KNOB" and "SELECT LEVEL"
 *     - Press 5 or ENTER to proceed
 *     - If you chose "SELECT LEVEL", you can now select the desired level by 
 *       pressing 2/8. There are 20 levels available.
 *     - Press 5 or ENTER to save and quit the menu ("SAVING" is displayed
 *       briefly)
 *       
 *     Commands 3xx (00-19; 99) also allow configuring the volume; using
 *     a rotary encoder for volume requires to set a fixed level (and thereby
 *     disabling the built-in volume knob)
 *
 * How to select the Music Folder Number:
 * 
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "MUSIC FOLDER NUMBER" is shown
 *     - Press 5 or ENTER
 *     - Press 2/8 to cycle through the possible values (0-9)
 *     - Press 5 or ENTER to save and quit the menu ("SAVING" is displayed
 *       briefly)
 * 
 * How to select the Time-cycling Interval:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "TIME CYCLING" is shown
 *     - Press 5 or ENTER
 *     - Press 2/8 to cycle through the possible Time-cycling Interval 
 *       values.
 *     - Press 5 or ENTER to select the value shown and exit the menu ("SAVING" is 
 *       displayed briefly)
 *
 * How to adjust the display brightness:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "BRIGHTNESS" is shown
 *     - Press 5 or ENTER
 *     - Press 2/8 to cycle through the possible levels (1-15) for the
 *       red display.
 *     - Press 5 or ENTER to proceed to the next display
 *     - After the third display, "SAVING" is displayed briefly and the menu 
 *       is left automatically.
 *
 * How to find out the IP address, WiFi status, MAC address, HA Status
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "NETWORK" is shown
 *     - Press 5 or ENTER, the displays shows the IP address
 *     - Press 2/8 to switch between WiFi status, MAC address, IP address
 *       and HomeAssistant/MQTT connection status
 *     - Press 5 or ENTER to leave the menu
 *
 * How to enter dates/times for the three displays / set the RTC:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until the desired display shows a date and time, and
 *       "PROGRAM DATE" or "SET CLOCK" (for the green display) is shown
 *     - Press 5 or ENTER
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
 *     use NTP or GPS for time synchronization. The time you entered will be
 *     overwritten if/when the device has access to network time via NTP, or 
 *     time from GPS. Don't forget to configure your time zone in the Config 
 *     Portal.
 *     
 *     Note that when entering dates/times into the destination time or last 
 *     time departed displays, Time Cycling is paused for 30 minutes. 
 *     Your entered date/time(s) are shown until overwritten by time travels.
 *     To bring them back, use keypad command 998.
 *     
 * How to view light/temperature/humidity sensor data:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press 2/8 until "SENSORS" is shown. If this menu does not appear,
 *       no light and/or temperature sensors were detected during boot.
 *     - Press 5 or ENTER to proceed
 *     - Sensor data is shown; if both light and temperature sensors are present,
 *       press 2/8 to switch between their data.
 *     - Press 5 or ENTER to leave the menu
 *
 * How to leave the menu:
 *
 *     While the main menu is active, press 9 to quit.
 */

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h> 

#include "tcddisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"
#include "tc_wifi.h"

#include "tc_menus.h"

#define MODE_ALRM 0
#define MODE_VOL  1
#define MODE_MSFX 2
#define MODE_AINT 3
#define MODE_BRI  4
#define MODE_NET  5
#define MODE_PRES 6
#define MODE_DEST 7
#define MODE_DEPT 8
#define MODE_SENS 9
#define MODE_LTS  10
#define MODE_CLI  11
#define MODE_VER  12

#define MODE_MIN  MODE_ALRM
#define MODE_MAX  MODE_VER

#define FIELD_MONTH   0
#define FIELD_DAY     1
#define FIELD_YEAR    2
#define FIELD_HOUR    3
#define FIELD_MINUTE  4

static tcdDisplay* displaySet;

// Our generic timeout when waiting for buttons
#define MENUTIMEOUT 60000   // 60 seconds
static unsigned long timeout = 0;

static const char *digitsHelp = "TYPE DIGITS";

static const char *StrSaving  = "SAVING";
static const char *StrCancel1 = "CANCELED -";
static const char *StrCancel2 = "CHANGES NOT";
static const char *StrCancel3 = "SAVED";

static const char *alarmWDSel = "USER DAYS";
static const char *alarmWDHelp = "1-7 FOR DAYS";

#ifdef IS_ACAR_DISPLAY
static const char *almFmt = "%3s     %02d%02d";
#else
static const char *almFmt = "%3s      %02d%02d";
#endif
static const char *alarmWD[10] = {
      #ifdef IS_ACAR_DISPLAY
      "MON-SUN", "MON-FRI", "SAT-SUN",
      #else
      "MON -SUN", "MON -FRI", "SAT -SUN",
      #endif
      "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"
};
static const char alarmWDLetters[] = "SMTWTFS";
static char       alarmWDBuf[8];

static int  doSetVolume();
static int  doSetMSfx();
static int  doSetAlarm();
static int  doSetAutoInterval();
static int  doSetBrightness(tcdDisplay* displaySet, uint8_t& newbri);
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
static void doShowSensors();
#endif
static void doShowNetInfo();
static void doShowBTTFNInfo();
static bool menuWaitForReleaseNC();
static bool checkEnterPress();
static void waitForEnterRelease();
static int  checkForMenuControl(bool &wasEnter, bool& dirDown, bool& wasQuit, bool& wasSelect);
static int  checkForMenuInput(bool &wasEnter, char& key);

static void resetTimeOut();
static bool checkTimeOut();

static void waitAudioDoneMenu();
static void menuDelay(unsigned long mydel);
static void menuLoops();
//static void menuShortDelay(unsigned long mydel);

// Some bin-size-friendly short-cuts, also used by other modules
void dt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    destinationTime.showTextDirect(text, flags);
}
void pt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    presentTime.showTextDirect(text, flags);
}
void lt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    departedTime.showTextDirect(text, flags);
}
void dt_off() { destinationTime.off(); }
void pt_off() { presentTime.off();     }
void lt_off() { departedTime.off();    }
void dt_on()  { destinationTime.on();  }
void pt_on()  { presentTime.on();      }
void lt_on()  { departedTime.on();     }

#define D_D 1
#define D_P 2
#define D_L 4
static void sw_sel(int w) {
    (w & D_D) ? dt_on() : dt_off();
    (w & D_P) ? pt_on() : pt_off();
    (w & D_L) ? lt_on() : lt_off();
}

static void allOffWaitEnterRelease()
{
    allOff();
    waitForEnterRelease();
}

static void prepareForInput()
{
    resetTimeOut();

    keypadKeyPressed = 0;
    resetKeypadState();
    keypadMode = 2;
}

/*
 *  Display the current main menu item
 *
 */
static void menuShow(int number, tcdDisplay*& displayHelp)
{
    char buf[32];
    
    switch (number) {
    case MODE_DEST:  // Destination Time
        displaySet = &destinationTime;
        displaySet->setColon(true);
        displaySet->show();
        pt_showTextDirect("PROGRAM DATE");
        sw_sel(D_P|D_D);
        displayHelp = &presentTime;
        break;
    case MODE_PRES:  // Present Time (RTC)
        displaySet = &presentTime;
        displaySet->setColon(true);
        displaySet->show();
        dt_showTextDirect("SET CLOCK");
        sw_sel(D_P|D_D);
        displayHelp = &departedTime;
        break;
    case MODE_DEPT:  // Last Time Departed
        displaySet = &departedTime;
        displaySet->setColon(true);
        displaySet->show();
        pt_showTextDirect("PROGRAM DATE");
        sw_sel(D_P|D_L);
        displayHelp = &presentTime;
        break;
    case MODE_VOL:    // Volume
        dt_showTextDirect("VOLUME");
        sw_sel(D_D);
        break;
    case MODE_MSFX:   // Music Folder Number
        dt_showTextDirect("MUSIC FOLDER");
        sw_sel(D_D);
        break;
    case MODE_ALRM:   // Alarm
        dt_showTextDirect("ALARM");
        sw_sel(D_D);
        break;
    case MODE_AINT:   // Time Cycling inverval
        dt_showTextDirect("TIME CYCLING");
        sw_sel(D_D);
        break;
    case MODE_BRI:    // Brightness
        dt_showTextDirect("BRIGHTNESS");
        sw_sel(D_D);
        break;
    case MODE_NET:    // Network info
        dt_showTextDirect("NETWORK");
        sw_sel(D_D);
        break;
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    case MODE_SENS:
        dt_showTextDirect("SENSORS");
        sw_sel(D_D);
        break;
    #endif
    case MODE_LTS:    // Last time sync info
        dt_showTextDirect("TIME SYNC");
        if(!lastAuthTime64) {
            pt_showTextDirect("NEVER");
            sw_sel(D_P|D_D);
        } else {
            uint64_t ago64 = (millis64() - lastAuthTime64) / 1000;
            uint32_t ago = ago64;
            if(ago > 24*60*60) {
                sprintf(buf, "%d DAYS", ago / (24*60*60));
            } else if(ago > 60*60) {
                sprintf(buf, "%d HOURS", ago / (60*60));
            } else if(ago > 60) {
                sprintf(buf, "%d MINS", ago / 60);
            } else {
                sprintf(buf, "%d SECS", ago);
            }
            pt_showTextDirect(buf);
            lt_showTextDirect("AGO");
            sw_sel(D_L|D_P|D_D);
        }
        break;
    case MODE_CLI:
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("BTTFN");
        pt_showTextDirect("CLIENTS");
        sw_sel(D_P|D_D);
        #else
        dt_showTextDirect("BTTFN CLIENTS");
        sw_sel(D_D);
        #endif
        break;
    case MODE_VER:  // Version info
        dt_showTextDirect("VERSION");
        pt_showTextDirect(TC_VERSION);
        #ifdef TC_VERSION_EXTRA
        lt_showTextDirect(TC_VERSION_EXTRA);
        sw_sel(D_L|D_P|D_D);
        #else
        sw_sel(D_P|D_D);
        #endif
        break;
    }
}

/*
 *  Cycle through main menu
 *  
 *  Returns
 *  - true if item selected
 *  - false on timeout or cancel
 *
 */

static int menuSelect(int& number, DateTime& dtu, DateTime& dtl, tcdDisplay*& displayHelp)
{
    bool wasEnter, dirDown, wasQuit, wasSelect;

    prepareForInput();

    while(!checkTimeOut()) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;
                
            if(wasSelect || (wasEnter && menuWaitForReleaseNC())) {
                keypadMode = 0;
                return 1;
            }

            if(dirDown) {
                if(number == MODE_MAX) number = MODE_MIN;
                else {
                    number++;
                    if(number == MODE_MSFX && !haveSD) number++;
                    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
                    if(number == MODE_SENS && (!(sgf & (SGF_UTemp|SGF_ULightSens)))) number++;
                    #else
                    if(number == MODE_SENS) number++;
                    #endif
                }
            } else {
                if(number == MODE_MIN) number = MODE_MAX;
                else {
                    number--;
                    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
                    if(number == MODE_SENS && (!(sgf & (SGF_UTemp|SGF_ULightSens)))) number--;
                    #else
                    if(number == MODE_SENS) number--;
                    #endif
                    if(number == MODE_MSFX && !haveSD) number--;
                }
            }

            if(number == MODE_PRES) {
                myrtcnow(dtu);
                UTCtoLocal(dtu, dtl, 0);
                presentTime.setDateTime(dtl);
            }

            // Show only the selected display, or menu item text
            menuShow(number, displayHelp);

        } else {

            menuDelay(50);

        }

    }

    keypadMode = 0;

    return 0;
}

/*
 * Show only the field to be updated and set up pre-set contents
 *
 */
static void showField(int number, int field, uint16_t dflags)
{
    switch(field) {
    case FIELD_MONTH:
        displaySet->showMonthDirect(number, dflags);
        break;
    case FIELD_DAY:
        displaySet->showDayDirect(number, dflags);
        break;
    case FIELD_YEAR:
        displaySet->showYearDirect(number, dflags);
        break;
    case FIELD_HOUR:
        displaySet->showHourDirect(number, dflags);
        break;
    case FIELD_MINUTE:
        displaySet->showMinuteDirect(number, dflags);
        break;
    }
}

/*
 * Wait for user input to update a field
 *
 */
static int requestNumericInput(int& number, int field, int year = 0, int month = 0, uint16_t dflags = 0)
{
    int upperLimit, lowerLimit;
    int setNum, prevNum = number;
    unsigned long blinkNow = 0;
    bool blinkSwitch = true;

    resetTimeOut();

    showField(number, field, dflags);

    // Write pre-set into buffer, reset index to 0
    // Upon first key input, the pre-set will be overwritten
    // This allows the user to keep the pre-set by pressing
    // enter
    sprintf(timeBuffer, "%d", number);
    resetTimebufIndices();
    timeBufferSize = 2;

    // Start blinking
    displaySet->onBlink(2);

    switch(field) {
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
        timeBufferSize = 4;
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

    waitForEnterRelease();

    // Force keypad to send keys to our timeBuffer (and block key holding)
    keypadMode = 1;

    // Reset key state machine
    resetKeypadState();

    while(!checkTimeOut() && !checkEnterPress()) {

        scanKeypad();

        setNum = atoi(timeBuffer);

        // Show setNum in field
        if(prevNum != setNum) {
            showField(setNum, field, dflags | CDD_NOLEAD0);
            prevNum = setNum;
            resetTimeOut();
            displaySet->on();
            blinkNow = millis();
            blinkSwitch = false;
        }

        menuDelay(50);

        if(!blinkSwitch && (millis() - blinkNow > 500)) {
            displaySet->onBlink(2);
            blinkSwitch = true;
        }

    }
    
    // Force keypad to send keys somewhere else but our buffer
    keypadMode = 0;

    // Stop blinking
    displaySet->on();

    if(checkTimeOut())
        return 0;

    setNum = atoi(timeBuffer);
    if(setNum < lowerLimit)      setNum = lowerLimit;
    else if(setNum > upperLimit) setNum = upperLimit;
    #ifdef TC_JULIAN_CAL
    if(field == FIELD_DAY) {
        correctNonExistingDate(year, month, setNum);
    }
    #endif
    number = setNum;

    // Display (corrected) number for .5 seconds
    showField(setNum, field, dflags);

    menuDelay(500);

    return 1;
}

/*
 * enter_menu(): The main menu loop
 *
 */
void enter_menu()
{
    bool currNM = destinationTime.getNightMode();
    bool mpActive;
    int  displayChanged = 0;
    int  showCancel = 0;
    int  menuItemNum;
    dateStruct dBackup;
    dateStruct lBackup;
    DateTime dtu, dtl;
    tcdDisplay *displayHelp;

    pwrNeedFullNow();

    allNightmode(false);
    allresetBrightness();
    // Do not propagate through BTTFN, menu is TCD-private

    mpActive = mp_stop();
    stopAudio();

    flushDelayedSave();

    // Start with first menu item
    menuItemNum = MODE_ALRM;

    // Backup currently displayed times
    destinationTime.getToStruct(&dBackup);
    departedTime.getToStruct(&lBackup);

    // Load the user times from NVM
    loadUserDLTimes();

    // Load local time into present time
    myrtcnow(dtu);
    UTCtoLocal(dtu, dtl, 0);
    presentTime.setDateTime(dtl);

    // Show first menu item
    menuShow(menuItemNum, displayHelp);

    waitForEnterRelease();

    if(!menuSelect(menuItemNum, dtu, dtl, displayHelp))
        goto quitMenu;

    if(menuItemNum >= MODE_PRES && menuItemNum <= MODE_DEPT) {

        // Enter display times

        displayHelp->showTextDirect(digitsHelp);
        displayHelp->on();

        // Generate pre-set values, which the user may keep (or overwrite)

        int yearSet, monthSet, daySet, hourSet, minSet;

        if(displaySet->isRTC()) {

            // This is the RTC, get the current local date/time
            myrtcnow(dtu);
            UTCtoLocal(dtu, dtl, 0);
            yearSet = dtl.year();
            monthSet = dtl.month();
            daySet = dtl.day();
            hourSet = dtl.hour();
            minSet = dtl.minute();

        } else {

            // Non-RTC, get the time info from the object
            // Remember: These are the ones saved, NOT
            // the ones that were possibly shown on the
            // display before invoking the menu
            displaySet->getToParms(yearSet, monthSet, daySet, hourSet, minSet);

        }

        // Get year
        if(!requestNumericInput(yearSet, FIELD_YEAR, displaySet->isRTC() ? TCEPOCH_GEN : 0, 0)) {
            showCancel = 1;
            goto quitMenu;
        }

        // Get month
        if(!requestNumericInput(monthSet, FIELD_MONTH)) {
            showCancel = 1;
            goto quitMenu;
        }

        // Get day
        if(!requestNumericInput(daySet, FIELD_DAY, yearSet, monthSet)) {  // this depends on the month and year
            showCancel = 1;
            goto quitMenu;
        }

        // Get hour
        if(!requestNumericInput(hourSet, FIELD_HOUR, 0, 0, CDD_FORCE24)) {
            showCancel = 1;
            goto quitMenu;
        }

        // Get minute
        if(!requestNumericInput(minSet, FIELD_MINUTE)) {
            showCancel = 1;
            goto quitMenu;
        }

        // Have new date & time at this point

        waitAudioDoneMenu();

        // Show a save message for a brief moment
        displaySet->showTextDirect(StrSaving);
        displayHelp->off();

        if(displaySet->isRTC()) {  // this is the RTC display, set the RTC

            uint16_t rtcYear;
            int16_t  tyroffs = 0;
            int ny = yearSet, nm = monthSet, nd = daySet, nh = hourSet, nmm = minSet;

            // Set up DST data for now current year
            parseTZ(0, yearSet);

            // Convert given local time to UTC
            // (DST-end ambiguity ignored here)
            LocalToUTC(ny, nm, nd, nh, nmm, 0);

            // Get RTC-fit year/offs
            rtcYear = ny;
            correctYr4RTC(rtcYear, tyroffs);

            // Adjust RTC & yoffs
            rtc.adjust(0, nmm, nh, dayOfWeek(nd, nm, ny), nd, nm, rtcYear-2000U);
            displaySet->setYearOffset(tyroffs);

            // Update & save lastYear & yearOffset
            lastYear = ny;
            displaySet->saveClockStateData(lastYear);

            // Resetting the RTC invalidates our timeDifference, ie
            // fake present time. Make the user return to present
            // time after setting the RTC
            timeDifference = 0;

        } else {

            // Non-RTC: Setting a static display, turn off autoInverval

            /* Previously, we here disabled time cycling (persistently)
            if(autoInterval) {
                autoInterval = 0;
                saveBeepAutoInterval();
            }
            */
            pauseAuto();    // Now we only pause for 30 mins

            displayChanged = menuItemNum;

        }

        // update the object
        displaySet->setFromParms(yearSet, monthSet, daySet, hourSet, minSet);

        // copy date to user slot
        displaySet->copyToUserTimes();

        if(displayChanged) {
            (displayChanged == MODE_DEST) ? backupDestTime() : backupLastTime();
        }

        // Save to NVM (regardless of persistence mode)
        displaySet->save();

        menuDelay(1000);

    } else if(menuItemNum == MODE_VOL) {

        allOffWaitEnterRelease();

        // Set volume
        showCancel = doSetVolume();

    } else if(menuItemNum == MODE_MSFX) {

        allOffWaitEnterRelease();

        // Set music folder number
        showCancel = doSetMSfx();

    } else if(menuItemNum == MODE_ALRM) {

        allOffWaitEnterRelease();

        // Set alarm
        showCancel = doSetAlarm();

    } else if(menuItemNum == MODE_AINT) {

        allOffWaitEnterRelease();

        // Set autoInterval
        showCancel = doSetAutoInterval();

    } else if(menuItemNum == MODE_BRI) {

        uint8_t dtbri = destinationTime.getBrightness();
        uint8_t ptbri = presentTime.getBrightness();
        uint8_t ltbri = departedTime.getBrightness();
        uint8_t dtbri2, ptbri2, ltbri2;

        allOffWaitEnterRelease();
        
        // Set brightness settings

        if(!(showCancel = doSetBrightness(&destinationTime, dtbri2))) {
            waitForEnterRelease();
            if(!(showCancel = doSetBrightness(&presentTime, ptbri2))) {
                waitForEnterRelease();
                showCancel = doSetBrightness(&departedTime, ltbri2);
            }
        }

        if(showCancel) {

            destinationTime.setBrightness(dtbri);
            presentTime.setBrightness(ptbri);
            
        } else {

            // Now save
            dt_showTextDirect(StrSaving);
            sw_sel(D_D);
    
            // (Re)Set current brightness as "initial" brightness
            destinationTime.setBrightness(dtbri2, true);
            presentTime.setBrightness(ptbri2, true);
            departedTime.setBrightness(ltbri2, true);
    
            // Update & write settings file
            settings.destTimeBright = dtbri2;
            settings.presTimeBright = ptbri2;
            settings.lastTimeBright = ltbri2;
    
            saveBrightness();
    
            menuDelay(1000);

        }

    } else if(menuItemNum == MODE_NET) {   // Show network info

        allOffWaitEnterRelease();

        // Show net info
        doShowNetInfo();

    } else if(menuItemNum == MODE_CLI) {   // Show bttfn clients

        allOffWaitEnterRelease();

        // Show client info
        doShowBTTFNInfo();
 
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    } else if(menuItemNum == MODE_SENS) {   // Show sensor info

        allOffWaitEnterRelease();

        doShowSensors();
    #endif

    }                                      // LTS, VERSION: Bail out

quitMenu:

    allOffWaitEnterRelease();

    if(showCancel) {
        dt_showTextDirect(StrCancel1);
        pt_showTextDirect(StrCancel2);
        lt_showTextDirect(StrCancel3);
        sw_sel(D_D|D_P|D_L);
        menuDelay(1500);
    }

    // Return dest/dept displays to where they should be
    if(!isWcMode() || (!(wcf & WCF_HaveTZ1))) {
        if(displayChanged == MODE_DEST) {
            destinationTime.load();
        } else {
            destinationTime.setFromStruct(&dBackup);
        }
    }
    if(!isWcMode() || (!(wcf & WCF_HaveTZ2))) {
        if(displayChanged == MODE_DEPT) {
            departedTime.load();
        } else {
            departedTime.setFromStruct(&lBackup);
        }
    }

    // Reset TZ-Name-Animation
    destShowAlt = depShowAlt = 0;

    // Done, turn off displays
    allOff();

    menuDelay(500);

    waitForEnterRelease();

    // Reset keypad key state and clear input buffer
    keypadMode = 0;
    resetKeypadState();
    discardKeypadInput();

    // Restore present time
    
    myrtcnow(gdtu);
    UTCtoLocal(gdtu, gdtl, 0);
    
    if(stalePresent)
        updateStalePresent(1);  // presentTime.setFromStruct(&stalePresentTime[1]);
    else
        updatePresentTime();    // Uses gdt{u,l}

    if(isWcMode()) {
        setDatesTimesWC(gdtu);
    }

    // Restore night mode
    allNightmode(currNM);

    // All displays on and show
    animate();

    menuLoops();

    // make time_loop immediately re-eval auto-nm
    // unless manual-override
    if(manualNightMode < 0) forceReEvalANM = true;

    // Re-set RotEnc for volume (ie ignore all
    // changes while in menu; adjust to cur level)
    #ifdef TC_HAVE_RE
    re_vol_reset();
    #endif

    // Restart mp if it was active before entering the menu
    if(mpActive) mp_play();
}


/*
 *  Alarm ######################################################
 */

const char *getAlWD(unsigned int wd, bool menuMode)
{
    char *buf = alarmWDBuf;
    int ri;
    
    if(menuMode && wd > 9) {
        return alarmWDSel;
    }
    if(wd & 0x80) {
        for(int i = 0; i < 7; i++) {
            if(i == 0) ri = 6;
            else       ri = i - 1;
            buf[ri] = (wd & (1 << i)) ? alarmWDLetters[i] : '_';
        }
        buf[7] = 0;
        return (const char *)alarmWDBuf;
    } else
        return alarmWD[wd];
}

// Set Alarm
static int doSetAlarm()
{
    char almBuf[16];
    char almBuf2[16];
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool alarmDone = false;
    bool newAlarmOnOff = alarmOnOff;
    int  newAlarmHour = (alarmHour <= 23) ? alarmHour : 0;
    int  newAlarmMinute = (alarmMinute <= 59) ? alarmMinute : 0;
    unsigned int newAlarmWeekday = alarmWeekday;
    bool wasEnter, dirDown, wasQuit = false, wasSelect;
    char newkey = 0;

    // For requestNumericInput()
    displaySet = &destinationTime;

    // On/Off
    sprintf(almBuf2, almFmt, "" , newAlarmHour, newAlarmMinute);
    sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
    dt_showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
    dt_on();

    prepareForInput();

    while(!checkTimeOut() && !alarmDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            if(blinkSwitch) {
                dt_showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
            }

            alarmDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!alarmDone) {

                newAlarmOnOff = !newAlarmOnOff;

                sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
                dt_showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);

                blinkSwitch = false;
                blinkNow = millis();

            }

        } else {

            unsigned long mm = millis();
            
            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                dt_showTextDirect(blinkSwitch ? almBuf2 : almBuf, CDT_CLEAR|CDT_COLON);
                blinkNow = mm;
            }

            menuDelay(50);

        }

    }

    keypadMode = 0;

    if(!(alarmDone & (!wasQuit))) return 1;

    lt_showTextDirect(digitsHelp);
    lt_on();

    // Get hour
    if(!requestNumericInput(newAlarmHour, FIELD_HOUR, 0, 0, CDD_FORCE24)) {
        waitAudioDoneMenu();    // (keypad; should not be necessary)
        return 1;
    }

    // Get minute
    if(!requestNumericInput(newAlarmMinute, FIELD_MINUTE)) {
        waitAudioDoneMenu();    // (keypad; should not be necessary)
        return 1;
    }

    lt_off();
    waitAudioDoneMenu();    // For keypad sounds

    // Weekday(s)

    waitForEnterRelease();

    if(alarmWeekday & 0x80) newAlarmWeekday = 10;
    
    dt_showTextDirect(getAlWD(newAlarmWeekday, true));
    destinationTime.onBlink(2);
    blinkSwitch = true;

    prepareForInput();

    alarmDone = false;

    while(!checkTimeOut() && !alarmDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            dt_on();
            blinkSwitch = false;

            alarmDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!alarmDone) {

                if(dirDown) {
                    newAlarmWeekday++;
                    if(newAlarmWeekday > 10) newAlarmWeekday = 0;
                } else {
                    if(!newAlarmWeekday) newAlarmWeekday = 10;
                    else newAlarmWeekday--;
                }

                dt_showTextDirect(getAlWD(newAlarmWeekday, true));

            }

        } else {

            menuDelay(50);

            if(!blinkSwitch) {
                destinationTime.onBlink(2);
                blinkSwitch = true;
            }

        }

    }

    keypadMode = 0;

    if(alarmDone & (!wasQuit)) {

        // Individual weekday selection
        if(newAlarmWeekday > 9) {
    
            if(alarmWeekday & 0x80)
                newAlarmWeekday = alarmWeekday;
            else
                newAlarmWeekday = 0xff;

            lt_showTextDirect(alarmWDHelp);
            lt_on();
            
            dt_showTextDirect(getAlWD(newAlarmWeekday));
            destinationTime.onBlink(2);
            blinkSwitch = true;
        
            prepareForInput();
        
            alarmDone = false;
        
            while(!checkTimeOut() && !alarmDone) {

                if(checkForMenuInput(wasEnter, newkey)) {
        
                    if(newkey == '9') {
                        wasQuit = true;
                        break;
                    }
        
                    dt_on();
                    blinkSwitch = false;
        
                    alarmDone = (wasEnter && menuWaitForReleaseNC());
        
                    if(!alarmDone) {
    
                        if(newkey >= '1' && newkey <= '7') {
                            newkey -= '0';
                            if(newkey == 7) newkey = 0;
                            newAlarmWeekday ^= (1 << newkey);
                            dt_showTextDirect(getAlWD(newAlarmWeekday));
                        }
        
                    }
        
                } else {
        
                    menuDelay(50);
        
                    if(!blinkSwitch) {
                        destinationTime.onBlink(2);
                        blinkSwitch = true;
                    }
        
                }
        
            }
            keypadMode = 0;
            waitAudioDoneMenu();    // For keypad sounds
        }
    }
    
    allOff();

    if(alarmDone & (!wasQuit)) {

        dt_showTextDirect(StrSaving);
        dt_on();

        // Save it
        alarmOnOff = newAlarmOnOff;
        alarmHour = newAlarmHour;
        alarmMinute = newAlarmMinute;
        alarmWeekday = newAlarmWeekday;
        saveAlarm();

        cancelSnooze();

        menuDelay(1000);

        return 0;
    }

    return 1;
}


/*
 *  Volume #####################################################
 */

static void showCurVolHWSW(bool blink)
{
    if(blink) {
        allOff();
    } else {
        if(curVolume == 255) {
            dt_showTextDirect("USE VOLUME");
            pt_showTextDirect("KNOB");
        } else {
            dt_showTextDirect("SELECT");
            pt_showTextDirect("LEVEL");
        }
        sw_sel(D_P|D_D);
    }
}

static void showCurVol(bool blink, bool doComment)
{
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("LEVEL", curVolume, flags);
    dt_on();

    if(doComment) {
        int w = D_D;
        if(!curVolume) {
            pt_showTextDirect("MUTE");
            w |= D_P;
        }
        sw_sel(w);
    }
}

static int doSetVolume()
{
    bool volDone = false;
    int oldVol = curVolume;
    unsigned long playNow;
    bool triggerPlay = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    showCurVolHWSW(false);

    prepareForInput();

    while(!checkTimeOut() && !volDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            if(blinkSwitch) {
                showCurVolHWSW(false);
            }

            volDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!volDone) {

                if(curVolume <= 19)
                    curVolume = 255;
                else
                    curVolume = 0;

                showCurVolHWSW(false);

                blinkNow = millis();
                blinkSwitch = false;

            }

        } else {

            unsigned long mm = millis();
                
            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                showCurVolHWSW(blinkSwitch);
                blinkNow = mm;
            }

            menuDelay(50);

        }

    }

    keypadMode = 0;

    if((volDone & (!wasQuit)) && curVolume != 255) {
        
        if(oldVol == 255) {
            curVolume = getSWVolFromHWVol();
            triggerPlay = true;
        } else {
            curVolume = oldVol;
        }
        
        showCurVol(false, true);

        blinkSwitch = false;
        blinkNow = millis();

        volDone = false;

        waitForEnterRelease();

        prepareForInput();

        playNow = millis();

        while(!checkTimeOut() && !volDone) {

            if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

                if(wasQuit) break;

                if(blinkSwitch) {
                    showCurVol(false, false);
                }

                volDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));
                
                if(!volDone) {

                    if(dirDown) {
                        if(curVolume > 0) curVolume--;
                    } else {
                        if(curVolume < 19) curVolume++;
                    }

                    showCurVol(false, true);

                    blinkNow = playNow = millis();
                    blinkSwitch = false;
                    triggerPlay = true;
                    stopAudio();

                }

            } else {

                unsigned long mm = millis();
                
                if(mm - blinkNow > 500) {
                    blinkSwitch = !blinkSwitch;
                    showCurVol(blinkSwitch, false);
                    blinkNow = mm;
                }

                if(triggerPlay && (mm - playNow >= 1*1000)) {
                    play_file("/startup.mp3", PA_INTRMUS|PA_ALLOWSD);
                    triggerPlay = false;
                }

                menuDelay(50);

            }
        }

    }

    stopAudio();
    allOff();

    keypadMode = 0;

    if(volDone & (!wasQuit)) {

        dt_showTextDirect(StrSaving);
        sw_sel(D_D);

        // Save it
        saveCurVolume();
        
        menuDelay(1000);
        
        return 0;

    }

    curVolume = oldVol;

    return 1;
}

/*  
 *   Music Folder Number ########################################
 */

static void displayMSfx(int msfx, bool blink, bool doFolderChk, int& folderState)
{
    int w = D_D|D_P;
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("FOLDER", msfx, flags);
    dt_on();
    
    if(doFolderChk) {
        pt_showTextDirect("WAIT...");
        sw_sel(w);
        folderState = mp_checkForFolder(msfx);
        switch(folderState) {
        case 1:
            pt_showTextDirect("OK");
            break;
        case 0:
            pt_showTextDirect("NOT FOUND");
            break;
        case -1:
            pt_showTextDirect("NEEDS");
            lt_showTextDirect("PROCESSING");
            w |= D_L;
            break;
        case -2:
            pt_showTextDirect("NO AUDIO");
            lt_showTextDirect("FILES");
            w |= D_L;
            break;
        case -3:
            pt_showTextDirect("NOT A FOLDER");
            break;
        }
        sw_sel(w);
    }
}

static int doSetMSfx()
{
    bool msfxDone = false;
    int oldmsfx = musFolderNum, folderState, dummy;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    displayMSfx(musFolderNum, false, true, folderState);

    prepareForInput();

    while(!checkTimeOut() && !msfxDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            if(blinkSwitch) {
                displayMSfx(musFolderNum, false, false, dummy);
            }

            msfxDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!msfxDone) {

                if(dirDown) {
                    if(!musFolderNum) musFolderNum = 9;
                    else musFolderNum--;
                } else {
                    if(musFolderNum == 9) musFolderNum = 0;
                    else musFolderNum++;
                }

                displayMSfx(musFolderNum, false, true, folderState);

                blinkNow = millis();
                blinkSwitch = false;

            }

        } else {

            unsigned long mm = millis();
                
            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                displayMSfx(musFolderNum, blinkSwitch, false, dummy);
                blinkNow = mm;
            }

            menuDelay(50);

        }

    }

    keypadMode = 0;

    if(msfxDone & (!wasQuit)) {

        dt_showTextDirect(StrSaving);
        sw_sel(D_D);

        // Save it
        saveMusFoldNum();
        
        if(folderState == -1) {
            // We do not do processing (=renaming) here.
            // This is more or less a blocking activity,
            // not suitable for our "multitasking" (with
            // regard to speedo action, especially).
            menuDelay(1000);
            prepareReboot();
            delay(1000);
            esp_restart();
        } else {
            mp_init();
        }

        menuDelay(1000);

        return 0;

    }

    musFolderNum = oldmsfx;
        
    return 1;
}


/*
 *  Time-cycling Interval (aka "autoInterval") #################
 */

static void displayAI(int interval, bool blink, bool doComment)
{
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("INTERVAL", interval, flags);

    if(doComment) {
        if(interval == 0) {
            pt_showTextDirect("OFF");
        } else {
            pt_showTextDirect("MINUTES");
        }
    }
}

/*
 * Adjust the autoInverval and save
 */
static int doSetAutoInterval()
{
    bool autoDone = false;
    int newAutoInterval = autoInterval;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    displayAI(autoTimeIntervals[newAutoInterval], false, true);
    sw_sel(D_P|D_D);

    prepareForInput();

    while(!checkTimeOut() && !autoDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            if(blinkSwitch) {
                displayAI(autoTimeIntervals[newAutoInterval], false, false);
            }

            autoDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!autoDone) {

                if(dirDown) {
                    if(newAutoInterval > 0) newAutoInterval--;
                } else {
                    if(newAutoInterval < 5) newAutoInterval++;
                }

                displayAI(autoTimeIntervals[newAutoInterval], false, true);

                blinkSwitch = false;
                blinkNow = millis();

            }

        } else {

            unsigned long mm = millis();

            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                displayAI(autoTimeIntervals[newAutoInterval], blinkSwitch, false);
                blinkNow = mm;
            }

            menuDelay(50);

        }

    }

    keypadMode = 0;

    if(autoDone & (!wasQuit)) {

        dt_showTextDirect(StrSaving);
        sw_sel(D_D);

        // Save it
        autoInterval = newAutoInterval;
        saveBeepAutoInterval();

        // End pause if current setting != off
        if(autoTimeIntervals[autoInterval]) 
            endPauseAuto();

        menuDelay(1000);

        return 0;

    }
        
    return 1;
}

/*
 * Brightness ##################################################
 */

static void displayBri(tcdDisplay* displaySet, int8_t number, bool blink)
{
    uint16_t flags = 0;
    if(blink) flags |= CDT_BLINK;
    
    #ifdef IS_ACAR_DISPLAY
    displaySet->showSettingValDirect("LV", number, flags);
    #else
    displaySet->showSettingValDirect("LVL", number, flags);
    #endif
}

static int doSetBrightness(tcdDisplay* displaySet, uint8_t& newbri)
{
    uint8_t oldBri = displaySet->getBrightness();
    uint8_t number;
    bool briDone = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    number = oldBri;

    // turn on all the segments
    allShowPattern();

    // Display "LVL"
    displayBri(displaySet, number, false);

    prepareForInput();

    while(!checkTimeOut() && !briDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            if(blinkSwitch) {
                displayBri(displaySet, number, false);
            }

            briDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!briDone) {

                if(dirDown) {
                    if(number > 0) number--;
                } else {
                    if(number < 15) number++;
                }

                displaySet->setBrightness(number);

                displayBri(displaySet, number, false);

                blinkSwitch = false;
                blinkNow = millis();

            }

        } else {

            unsigned long mm = millis();

            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                displayBri(displaySet, number, blinkSwitch);
                blinkNow = mm;
            }

            menuDelay(50);

        }

    }

    allShowPattern();  // turn on all the segments

    keypadMode = 0;

    if(!(briDone & (!wasQuit))) {
        displaySet->setBrightness(oldBri);
        return 1;
    }

    newbri = number;

    return 0;
}

/*
 * Show sensor info ############################################
 */

static void sensWait()
{
    dt_showTextDirect("WAIT...");
    pt_showTextDirect("");
}

#if defined(TC_HAVETEMP) || defined(TC_HAVELIGHT)
static void doShowSensors()
{
    char buf[13];
    bool luxDone = false;
    unsigned sensNow = 0;
    uint8_t numberArr[3];
    uint8_t numIdx = 0, maxIdx = 0;
    int hum;
    float temp;
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    #ifdef TC_HAVELIGHT
    if(sgf & SGF_ULightSens) numberArr[numIdx++] = 0;
    #endif
    #ifdef TC_HAVETEMP
    if(sgf & SGF_UTemp) {  
        numberArr[numIdx++] = 1;
        if(tempSens.haveHum()) numberArr[numIdx++] = 2;
    }
    #endif
    maxIdx = numIdx - 1;
    numIdx = 0;

    sensWait();
    sw_sel(D_P|D_D);

    prepareForInput();

    sensNow = millis();

    // Don't use timeOut here, user might want to keep
    // sensor data displayed for a longer period.

    while(!luxDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            luxDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!luxDone) {
                if(maxIdx > 0) {
                    if(dirDown) {
                        numIdx++;
                        if(numIdx > maxIdx) numIdx = 0;
                    } else {
                        if(!numIdx) numIdx = maxIdx;
                        else numIdx--;
                    }
                    sensWait();
                }
            }
            
        } else {

            menuDelay(50);

            if(millis() - sensNow > 3000) {
                switch(numberArr[numIdx]) {
                case 0:
                    #ifdef TC_HAVELIGHT
                    lightSens.loop();
                    dt_showTextDirect("LIGHT");
                    //#ifdef TC_DBG_SENS
                    //sprintf(buf, "%08x", lightSens.readDebug());
                    //#else
                    sprintf(buf, "%d LUX", lightSens.readLux());
                    //#endif
                    #else
                    buf[0] = 0;
                    #endif
                    break;
                case 1:
                    #ifdef TC_HAVETEMP
                    dt_showTextDirect("TEMPERATURE");
                    temp = tempSens.readTemp();
                    if(isnan(temp)) {
                        sprintf(buf, "--.--~%c", (sgf & SGF_TempCelsius) ? 'C' : 'F');
                    } else {
                        sprintf(buf, "%.2f~%c", temp, (sgf & SGF_TempCelsius) ? 'C' : 'F');
                    }
                    #else
                    buf[0] = 0;
                    #endif
                    break;
                case 2:
                    #ifdef TC_HAVETEMP
                    tempSens.readTemp();
                    dt_showTextDirect("HUMIDITY");
                    hum = tempSens.readHum();
                    if(hum < 0) {
                        #ifdef IS_ACAR_DISPLAY
                        sprintf(buf, "--\x7f\x80");
                        #else
                        sprintf(buf, "-- \x7f\x80");
                        #endif
                    } else {
                        #ifdef IS_ACAR_DISPLAY
                        sprintf(buf, "%2d\x7f\x80", hum);
                        #else
                        sprintf(buf, "%2d \x7f\x80", hum);
                        #endif
                    }
                    #else
                    buf[0] = 0;
                    #endif
                    break;
                }
                pt_showTextDirect(buf);
                sensNow = millis();
            }

        }

    }

    keypadMode = 0;
}
#endif

/*
 * Show network info ###########################################
 */

static void displayIP()
{
    uint8_t a, b, c, d;

    wifi_getIP(a, b, c, d);
    
    dt_showTextDirect("IP");
    presentTime.showHalfIPDirect(a, b, CDT_CLEAR);
    departedTime.showHalfIPDirect(c, d, CDT_CLEAR);
    
    sw_sel(D_L|D_P|D_D);
}

static void doShowNetInfo()
{
    int number = 0;
    bool netDone = false;
    char macBuf[16];
    int maxMI = 2;
    int w;
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    #ifdef TC_HAVEMQTT
    maxMI = 3;
    #endif

    wifi_getMAC(macBuf);

    displayIP();

    prepareForInput();

    while(!checkTimeOut() && !netDone) {

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            netDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!netDone) {

                if(dirDown) {
                    number++;
                    if(number > maxMI) number = 0;
                } else {
                    if(!number) number = maxMI;
                    else number--;
                }

                switch(number) {
                case 0:
                    displayIP();
                    break;
                case 1:
                    dt_showTextDirect("WIFI");
                    w = D_D|D_P;
                    switch(wifi_getStatus()) {
                    case WL_IDLE_STATUS:
                        pt_showTextDirect("IDLE");
                        break;
                    case WL_SCAN_COMPLETED:
                        pt_showTextDirect("SCAN");
                        lt_showTextDirect("COMPLETE");
                        w |= D_L;
                        break;
                    case WL_NO_SSID_AVAIL:
                        pt_showTextDirect("SSID NOT");
                        lt_showTextDirect("AVAILABLE");
                        w |= D_L;
                        break;
                    case WL_CONNECTED:
                        pt_showTextDirect("CONNECTED");
                        break;
                    case WL_CONNECT_FAILED:
                        pt_showTextDirect("CONNECT");
                        lt_showTextDirect("FAILED");
                        w |= D_L;
                        break;
                    case WL_CONNECTION_LOST:
                        pt_showTextDirect("CONNECTION");
                        lt_showTextDirect("LOST");
                        w |= D_L;
                        break;
                    case WL_DISCONNECTED:
                        pt_showTextDirect("DISCONNECTED");
                        break;
                    case 0x10000:
                    case 0x10003:   // AP-STA, treat as AP (=AP with conn.config'd but not connected)
                        pt_showTextDirect("AP MODE");
                        break;
                    case 0x10001:
                        pt_showTextDirect("OFF");
                        break;
                    //case 0x10002:     // UNKNOWN
                    default:
                        pt_showTextDirect("UNKNOWN");
                    }
                    sw_sel(w);
                    break;
                case 2:
                    dt_showTextDirect("MAC");
                    pt_showTextDirect(macBuf, CDT_CLEAR|CDT_CORR6);
                    sw_sel(D_P|D_D);
                    break;
                #ifdef TC_HAVEMQTT
                case 3:
                    #ifdef IS_ACAR_DISPLAY
                    dt_showTextDirect("MQTT");
                    #else
                    dt_showTextDirect("HOMEASSISTANT");
                    #endif
                    if(useMQTT) {
                        pt_showTextDirect(mqttState() ? "CONNECTED" : "DISCONNECTED");
                    } else {
                        pt_showTextDirect("OFF");
                    }
                    sw_sel(D_P|D_D);
                    break;
                #endif
                }

            }

        } else {

            menuDelay(50);

        }

    }

    keypadMode = 0;
}

/*
 * Show BTTFN network clients ##################################
 */

static bool bttfnValChar(char *s, int i)
{
    char a = s[i];

    if(a == '-') return true;
    if(a < '0') return false;
    if(a > '9' && a < 'A') return false;
    if(a <= 'Z') return true;
    if(a >= 'a' && a <= 'z') {
        s[i] &= ~0x20;
        return true;
    }
    return false; 
}

static void displayClient(int numCli, int number)
{
    uint8_t *ip;
    char *id;
    uint8_t type;
    char idbuf[16];
    const char *tpArr[6] = { "[FLUX]", "[SID]", "[GAUGES]", "[VSR]", "[AUX]", "[REMOTE]" };
    
    if(!numCli) {
        dt_showTextDirect("NO CLIENTS");
        sw_sel(D_D);
        return;
    }

    if(number >= numCli) number = numCli - 1;

    if(bttfnGetClientInfo(number, &id, &ip, &type)) {
       
        bool badName = false;
        if(!*id) {
            badName = true;
        } else {
            for(int j = 0; j < 13; j++) {
                if(!id[j] || id[j] == '.') break;
                if(!bttfnValChar(id, j)) {
                    badName = true;
                    break;
                }
            }
        }

        if(badName) {
            if(type >= BTTFN_TYPE__MIN && type <= BTTFN_TYPE__MAX) {
                strcpy(idbuf, tpArr[type-1]);
            } else {
                strcpy(idbuf, "[UNKNOWN]");
            }
        } else {
            strncpy(idbuf, id, 13);
            idbuf[13] = 0;
        }
      
        dt_showTextDirect(idbuf);
        
        presentTime.showHalfIPDirect(ip[0], ip[1], CDT_CLEAR);
        departedTime.showHalfIPDirect(ip[2], ip[3], CDT_CLEAR);
        sw_sel(D_D|D_P|D_L);
        #ifdef TC_DBG_NET
        Serial.printf("BTTFN client type %d\n", type);
        #endif
    } else {
        dt_showTextDirect("EXPIRED");
        sw_sel(D_D);
    }
}

void doShowBTTFNInfo()
{
    int number = 0;
    bool netDone = false;
    int numCli = bttfnNumClients();
    int oldNumCli;
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    oldNumCli = numCli;

    displayClient(numCli, number);

    prepareForInput();

    while(!checkTimeOut() && !netDone) {

        if(oldNumCli != numCli) {
            number = 0;
            displayClient(numCli, number);
            oldNumCli = numCli;
        }

        if(checkForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            netDone = (wasSelect || (wasEnter && menuWaitForReleaseNC()));

            if(!netDone) {

                if(numCli > 1) {
                    if(dirDown) {
                        number++;
                        if(number >= numCli) number = 0;
                    } else {
                        if(!number) number = numCli - 1;
                        else number--;
                    }
                } else
                    number = 0;

                displayClient(numCli, number);

            }

        } else {

            menuDelay(50);
            numCli = bttfnNumClients();

        }

    }

    keypadMode = 0;
}


/* *** Helpers ################################################### */


/*
 * Wait for ENTER to be released 
*/
static bool menuWaitForReleaseNC()
{
    while(checkEnterPress()) {
        menuLoops();
    }
    return true;
}

/*
 * Check if ENTER is pressed
 * (and properly debounce)
 */
static bool checkEnterPress()
{
    if(digitalRead(ENTER_BUTTON_PIN)) {
        menuDelay(50);
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
            menuDelay(50);
        }
        menuDelay(50);
        if(!digitalRead(ENTER_BUTTON_PIN))
            break;
    }
}

static void waitAudioDoneMenu()
{
    unsigned long startNow = millis();

    while(!checkAudioDone() && (millis() - startNow < 3000)) {
        menuDelay(50);
    }
}

static int checkForMenuControl(bool &wasEnter, bool& dirDown, bool& wasQuit, bool& wasSelect)
{
    int ret = 0;
    wasEnter = wasQuit = wasSelect = false;

    scanKeypad();
    
    if(checkEnterPress()) {
        wasEnter = true;
        ret++;
    } else if(keypadKeyPressed) {
        char c = keypadKeyPressed;
        keypadKeyPressed = 0;
        switch(c) {
        case '2':
            dirDown = false;
            ret++;
            break;
        case '8':
            dirDown = true;
            ret++;
            break;
        case '5':
            wasSelect = true;
            ret++;
            break;
        case '9':
            wasQuit = true;
            ret++;
        }
        
    }

    if(ret) resetTimeOut();

    return ret;
}

static int checkForMenuInput(bool &wasEnter, char& key)
{
    int ret = 0;
    
    wasEnter = false;
    key = 0;

    scanKeypad();
    
    if(checkEnterPress()) {
        wasEnter = true;
        ret++;
    } else if(keypadKeyPressed) {
        key = keypadKeyPressed;
        keypadKeyPressed = 0;
        ret++;
    }

    if(ret) resetTimeOut();

    return ret;
}

/*
 * Call this frequently while waiting for button presses,
 * increments timeout each second, returns true when 
 * MENUTIMEOUT reached.
 *
 */
static void resetTimeOut()
{
    timeout = millis();
}

static bool checkTimeOut()
{
    return (millis() - timeout > MENUTIMEOUT);
}

/*
 * MenuDelay: Delay() for keypad menu.
 * Calls myloops() periodically
 */
static void menuDelay(unsigned long mydel)
{
    unsigned long startNow = millis();

    while(millis() - startNow < mydel) {
        menuLoops();
        delay(5);
    }
}

/*
 *  Call a few loops, meant to be used inside delays.
 *  This allows other stuff to proceed while we wait.
 */
static void menuLoops()
{
    audio_loop();
    wifi_loop();
    audio_loop();
    ntp_loop();
    audio_loop();
    bttfn_loop();
    audio_loop();
    #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
    speedoUpdate_loop(true);  // GPS part: 6-12ms without delay, 8-13ms with delay
    audio_loop();
    #endif
}
