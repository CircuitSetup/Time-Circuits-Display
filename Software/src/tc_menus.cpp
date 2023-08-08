/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 *
 * Keypad Menu handling
 *
 * Based on code by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
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
 *     - select the music folder number ("MUSIC FOLDER NUMBER")
 *     - select the Time-Cycling Interval ("TIME CYCLING"),
 *     - select the brightness for the three displays ("BRIGHTNESS"),
 *     - show network information ("NET-WORK"),
 *     - enter dates/times for the three displays/set built-in RTC,
 *     - show currently measured data from connected sensors ("SENSORS"),
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
 *    Under normal operation (ie outside of the menu), holding "1" toggles 
 *    the alarm on/off. When the alarm is set and enabled, the dot in the 
 *    present time's minute field will light up.
 *    
 *    This quickly set the alarm time outside of the menu, enter "11hhMM" and 
 *    press ENTER. (Weekday selection must be done through the menu.)
 *    To see the current time/weekday settings, type 11 and press ENTER.
 *
 *    Note that the alarm is recurring, ie it rings at the programmed time
 *    unless disabled. Also note that the alarm is by default relative to your 
 *    actual present time, not the time displayed (eg after a time travel). 
 *    It can, however, be configured to be based on the time displayed, in 
 *    the Config Portal.
 *
 * How to set the audio volume:
 *
 *     Basically, and by default, the device uses the hardware volume knob to 
 *     determine the desired volume. You can change this to a fixed level as 
 *     follows:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "VOL-UME" is shown
 *     - Hold ENTER
 *     - Press ENTER to toggle between "USE VOLUME KNOB" and "FIXED LEVEL"
 *     - Hold ENTER to proceed
 *     - If you chose "FIXED LEVEL", you can now select the desired level by 
 *       pressing ENTER repeatedly. There are 20 levels available.
 *     - Hold ENTER to save and quit the menu ("SAVING" is displayed
 *       briefly)
 *
 * How to select the Music Folder Number:
 * 
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "MUSIC FOLDER NUMBER" is shown
 *     - Hold ENTER, "NUMBER" is displayed
 *     - Press ENTER to cycle through the possible settings (0-9)
 *       values.
 *     - Hold ENTER to save and quit the menu ("SAVING" is displayed
 *       briefly)
 * 
 * How to select the Time-cycling Interval:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "TIME CYCLING" is shown
 *     - Hold ENTER, "INTERVAL" is displayed
 *     - Press ENTER to cycle through the possible Time-cycling Interval 
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
 *     - Press ENTER to cycle through the possible levels (1-15)
 *     - Hold ENTER to use current value and proceed to next display
 *     - After the third display, "SAVING" is displayed briefly and the menu 
 *       is left automatically.
 *
 * How to find out the IP address, WiFi status and MAC address
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "NET-WORK" is shown
 *     - Hold ENTER, the displays shows the IP address
 *     - Press ENTER to toggle between WiFi status, MAC address, IP address
 *       and HomeAssistant/MQTT connection status
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
 *     use NTP or GPS for time synchronization. The time you entered will be
 *     overwritten if/when the device has access to network time via NTP, or 
 *     time from GPS. Don't forget to configure your time zone in the Config 
 *     Portal.
 *     
 *     Note that when entering dates/times into the destination time or last 
 *     time departed displays, the Time-rotation Interval is automatically 
 *     set to 0 (ie turned off). Your entered date/time(s) are shown until 
 *     overwritten by time travels (see above, section How to select the 
 *     Time-rotation Interval").
 *     
 * How to view light/temperature/humidity sensor data:
 *
 *     - Hold ENTER to invoke main menu
 *     - Press ENTER until "SENSORS" is shown. If this menu does not appear,
 *       no light and/or temperature sensors were detected during boot.
 *     - Hold ENTER to proceed
 *     - Sensor data is shown; if both light and temperature sensors are present,
 *       press ENTER to toggle between their data.
 *     - Hold ENTER to leave the menu
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
#include <WiFi.h> 

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"
#include "tc_wifi.h"

#include "tc_menus.h"

#define MODE_CPA  0
#define MODE_ALRM 1
#define MODE_VOL  2
#define MODE_MSFX 3
#define MODE_AINT 4
#define MODE_BRI  5
#define MODE_NET  6
#define MODE_PRES 7
#define MODE_DEST 8
#define MODE_DEPT 9
#define MODE_SENS 10
#define MODE_LTS  11
#define MODE_CLI  12
#define MODE_VER  13
#define MODE_END  14
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

uint8_t remMonth = 0;
uint8_t remDay = 0;
uint8_t remHour = 0;
uint8_t remMin = 0;

static int menuItemNum;

bool keypadInMenu = false;
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
static const char *alarmWD[10] = {
      #ifdef IS_ACAR_DISPLAY
      "MON-SUN", "MON-FRI", "SAT-SUN",
      #else
      "MON -SUN", "MON -FRI", "SAT -SUN",
      #endif
      "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"
};

static bool menuSelect(int& number, int mode_min);
static void menuShow(int number);
static void setUpdate(uint16_t number, int field, uint16_t dflags = 0);
static bool setField(uint16_t& number, uint8_t field, int year = 0, int month = 0, uint16_t dflags = 0);
static void showCurVolHWSW();
static void showCurVol();
static void doSetVolume();
static void displayMSfx(int msfx);
static void doSetMSfx();
static void doSetAlarm();
static void saveAutoInterval();
static void displayAI(int interval);
static void doSetAutoInterval();
static bool doSetBrightness(clockDisplay* displaySet);
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
static void doShowSensors();
#endif
static void displayIP();
static void doShowNetInfo();
#ifdef TC_HAVEBTTFN
static void doShowBTTFNInfo();
#endif
static bool menuWaitForRelease();
static bool checkEnterPress();
static void prepareInput(uint16_t number);

static bool checkTimeOut();

static void myssdelay(unsigned long mydel);
static void myloop();

static void dt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    destinationTime.showTextDirect(text, flags);
}
static void pt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    presentTime.showTextDirect(text, flags);
}
static void lt_showTextDirect(const char *text, uint16_t flags = CDT_CLEAR)
{
    departedTime.showTextDirect(text, flags);
}
static void dt_off() { destinationTime.off(); }
static void pt_off() { presentTime.off();     }
static void lt_off() { departedTime.off();    }
static void dt_on() {  destinationTime.on(); }
static void pt_on() {  presentTime.on();     }
static void lt_on() {  departedTime.on();    }
/*
 * enter_menu()
 *
 */
void enter_menu()
{
    bool desNM = destinationTime.getNightMode();
    bool preNM = presentTime.getNightMode();
    bool depNM = departedTime.getNightMode();
    bool mpActive;
    bool nonRTCdisplayChanged = false;
    int mode_min;
    int y1, yo1, m1, d1, h1, mi1;
    int y2, yo2, m2, d2, h2, mi2;
    DateTime dt;

    pwrNeedFullNow();

    isEnterKeyHeld = false;
    isEnterKeyPressed = false;

    destinationTime.setNightMode(false);
    presentTime.setNightMode(false);
    departedTime.setNightMode(false);
    destinationTime.resetBrightness();
    presentTime.resetBrightness();
    departedTime.resetBrightness();

    mpActive = mp_stop();
    stopAudio();

    // start with first menu item
    mode_min = check_allow_CPA() ? MODE_CPA : MODE_ALRM;
    menuItemNum = mode_min;

    // Load the custom times from NVM
    // Backup current times
    destinationTime.getToParms(y1, yo1, m1, d1, h1, mi1);
    departedTime.getToParms(y2, yo2, m2, d2, h2, mi2);
    // Load NWM times
    destinationTime.load();
    departedTime.load();

    // Load the RTC time into present time
    myrtcnow(dt);
    presentTime.setDateTime(dt);

    // Show first menu item
    menuShow(menuItemNum);

    waitForEnterRelease();

    // menuSelect:
    // Wait for ENTER to cycle through main menu,
    // HOLDing ENTER selects current menu "item"
    // (Also sets displaySet to selected display, if applicable)
    if(!(menuSelect(menuItemNum, mode_min)))
        goto quitMenu;

    if(menuItemNum >= MODE_PRES && menuItemNum <= MODE_DEPT) {

        // Enter display times

        // Generate pre-set values, which the user may keep (or overwrite)

        uint16_t yearSet;
        uint16_t monthSet;
        uint16_t daySet;
        uint16_t minSet;
        uint16_t hourSet;

        if(displaySet->isRTC()) {

            // This is the RTC, get the current date/time
            myrtcnow(dt);
            yearSet = dt.year() - displaySet->getYearOffset();
            monthSet = dt.month();
            daySet = dt.day();
            minSet = dt.minute();
            hourSet = dt.hour();

        } else {

            // Non-RTC, get the time info from the object
            // Remember: These are the ones saved in NVM
            // NOT the ones that were possibly shown on the
            // display before invoking the menu
            yearSet = displaySet->getYear();
            monthSet = displaySet->getMonth();
            daySet = displaySet->getDay();
            minSet = displaySet->getMinute();
            hourSet = displaySet->getHour();

        }

        // Get year
        setUpdate(yearSet, FIELD_YEAR);
        prepareInput(yearSet);
        waitForEnterRelease();
        if(!setField(yearSet, FIELD_YEAR, displaySet->isRTC() ? TCEPOCH_GEN : 0, 0))
            goto quitMenu;

        // Get month
        setUpdate(monthSet, FIELD_MONTH);
        prepareInput(monthSet);
        waitForEnterRelease();
        if(!setField(monthSet, FIELD_MONTH))
            goto quitMenu;          

        // Get day
        setUpdate(daySet, FIELD_DAY);
        prepareInput(daySet);
        waitForEnterRelease();
        if(!setField(daySet, FIELD_DAY, yearSet, monthSet))  // this depends on the month and year
            goto quitMenu;

        // Get hour
        setUpdate(hourSet, FIELD_HOUR, CDD_FORCE24);
        prepareInput(hourSet);
        waitForEnterRelease();
        if(!setField(hourSet, FIELD_HOUR, 0, 0, CDD_FORCE24))
            goto quitMenu;

        // Get minute
        setUpdate(minSet, FIELD_MINUTE);
        prepareInput(minSet);
        waitForEnterRelease();
        if(!setField(minSet, FIELD_MINUTE))
            goto quitMenu;

        // Have new date & time at this point

        waitAudioDone();

        if(displaySet->isRTC()) {  // this is the RTC display, set the RTC

            // Update RTC and fake year if neccessary
            // see comments in tc_time.cpp

            uint16_t newYear = yearSet;
            int16_t tyroffs = 0;

            // Set up DST data for now current year
            if(!(parseTZ(0, yearSet))) {
                #ifdef TC_DBG
                Serial.println(F("Menu: Failed to parse TZ"));
                #endif
            }

            // Get RTC-fit year plus offs for given real year
            correctYr4RTC(newYear, tyroffs);

            displaySet->setYearOffset(tyroffs);

            rtc.adjust(0, minSet, hourSet, dayOfWeek(daySet, monthSet, yearSet), daySet, monthSet, newYear-2000U);

            // User entered current local time; set DST flag accordingly
            // (This assumes user did not set time in the "loop-hour" when
            // DST is ending, otherwise there's a 50% chance DST is wrong.)
            if(couldDST[0]) {
                int currTimeMins;
                presentTime.setDST(timeIsDST(0, yearSet, monthSet, daySet, hourSet, minSet, currTimeMins));
                // Saved below
            } else {
                presentTime.setDST(0);
            }

            // Avoid immediate re-adjustment in time_loop
            lastYear = yearSet;
            presentTime.saveLastYear(lastYear);

            // Resetting the RTC invalidates our timeDifference, ie
            // fake present time. Make the user return to present
            // time after setting the RTC
            timeDifference = 0;

            yearSet = newYear;

        } else {

            // Non-RTC: Setting a static display, turn off autoInverval

            displaySet->setYearOffset(0);

            if(autoInterval) {
                autoInterval = 0;
                saveAutoInterval();
                updateConfigPortalValues();
            }

            nonRTCdisplayChanged = true;

        }

        // Show a save message for a brief moment
        displaySet->showTextDirect(StrSaving);

        // update the object
        displaySet->setYear(yearSet);
        displaySet->setMonth(monthSet);
        displaySet->setDay(daySet);
        displaySet->setHour(hourSet);
        displaySet->setMinute(minSet);

        // Save to NVM (regardless of persistence mode)
        displaySet->save();

        mydelay(1000);

    } else if(menuItemNum == MODE_VOL) {

        allOff();
        waitForEnterRelease();

        // Set volume
        doSetVolume();

    } else if(menuItemNum == MODE_MSFX) {

        allOff();
        waitForEnterRelease();

        // Set music folder number
        doSetMSfx();

    } else if(menuItemNum == MODE_ALRM) {

        allOff();
        waitForEnterRelease();

        // Set alarm
        doSetAlarm();

    } else if(menuItemNum == MODE_AINT) {

        allOff();
        waitForEnterRelease();

        // Set autoInterval
        doSetAutoInterval();

    } else if(menuItemNum == MODE_BRI) {

        uint8_t dtbri = destinationTime.getBrightness();
        uint8_t ptbri = presentTime.getBrightness();
        uint8_t ltbri = departedTime.getBrightness();

        allOff();
        
        // Set brightness settings
        waitForEnterRelease();
        if(!doSetBrightness(&destinationTime))
            goto quitMenu;

        waitForEnterRelease();
        if(!doSetBrightness(&presentTime))
            goto quitMenu;

        waitForEnterRelease();
        if(!doSetBrightness(&departedTime))
            goto quitMenu;

        // Now save
        dt_showTextDirect(StrSaving);
        pt_off();
        lt_off();

        uint8_t dtbri2 = destinationTime.getBrightness();
        uint8_t ptbri2 = presentTime.getBrightness();
        uint8_t ltbri2 = departedTime.getBrightness();

        if( (dtbri2 != dtbri) ||
            (ptbri2 != ptbri) || 
            (ltbri2 != ltbri) ) {

            // (Re)Set current brightness as "initial" brightness
            destinationTime.setBrightness(dtbri2, true);
            presentTime.setBrightness(ptbri2, true);
            departedTime.setBrightness(ltbri2, true);

            // Convert bri values to strings, write to settings, write settings file
            sprintf(settings.destTimeBright, "%d", dtbri2);
            sprintf(settings.presTimeBright, "%d", ptbri2);
            sprintf(settings.lastTimeBright, "%d", ltbri2);
            write_settings();
            updateConfigPortalValues();
        }

        mydelay(1000);

    } else if(menuItemNum == MODE_NET) {   // Show network info

        allOff();
        waitForEnterRelease();

        // Show net info
        doShowNetInfo();

    #ifdef TC_HAVEBTTFN
    } else if(menuItemNum == MODE_CLI) {   // Show bttfn clients

        allOff();
        waitForEnterRelease();

        // Show client info
        doShowBTTFNInfo();

    #endif  
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    } else if(menuItemNum == MODE_SENS) {   // Show light sensor info

        allOff();
        waitForEnterRelease();

        doShowSensors();
    #endif
    
    } else if(menuItemNum == MODE_CPA) {   // Copy audio files

        allOff();
        waitForEnterRelease();

        // Copy audio files
        doCopyAudioFiles();

    }                                      // LTS, VERSION, END: Bail out

quitMenu:

    allOff();
    waitForEnterRelease();

    // Return dest/dept displays to where they should be
    if((autoTimeIntervals[autoInterval] == 0) || checkIfAutoPaused()) {
        if(nonRTCdisplayChanged) {
            if(!isWcMode() || !WcHaveTZ1) destinationTime.load();
            if(!isWcMode() || !WcHaveTZ2) departedTime.load();
        } else {
            if(!isWcMode() || !WcHaveTZ1) {
                  destinationTime.setFromParms(y1, m1, d1, h1, mi1); 
                  destinationTime.setYearOffset(yo1); 
            }
            if(!isWcMode() || !WcHaveTZ2) {
                  departedTime.setFromParms(y2, m2, d2, h2, mi2);
                  departedTime.setYearOffset(yo2); 
            }
        }
    } else {
        if(!isWcMode() || !WcHaveTZ1) destinationTime.setFromStruct(&destinationTimes[autoTime]);
        if(!isWcMode() || !WcHaveTZ2) departedTime.setFromStruct(&departedTimes[autoTime]);
    }
    destShowAlt = depShowAlt = 0; // Reset TZ-Name-Animation

    // Done, turn off displays
    allOff();

    mydelay(500);

    waitForEnterRelease();

    resetKeypadState();

    // Restore present time
    myrtcnow(dt);
    presentTime.setDateTimeDiff(dt);
    if(isWcMode()) {
        setDatesTimesWC(dt);
    }

    // all displays on and show

    animate();

    // Restore night mode
    destinationTime.setNightMode(desNM);
    presentTime.setNightMode(preNM);
    departedTime.setNightMode(depNM);

    myloop();

    // make time_loop immediately re-eval auto-nm
    // unless manual-override
    if(manualNightMode < 0) forceReEvalANM = true;

    // Restart mp if it was active before entering the menu
    if(mpActive) mp_play();
}

/*
 *  Cycle through main menu
 *  
 *  Returns
 *  - true if item selected
 *  - false if timeout
 *
 */
static bool menuSelect(int& number, int mode_min)
{
    timeout = 0;

    // Wait for enter
    while(!checkTimeOut()) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(menuWaitForRelease()) return true;

            number++;
            if(number == MODE_MSFX && !haveSD) number++;
            #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
            if(number == MODE_SENS && !useLight && !useTemp) number++;
            #else
            if(number == MODE_SENS) number++;
            #endif
            #ifndef TC_HAVEBTTFN
            if(number == MODE_CLI) number++;
            #endif
            if(number > MODE_MAX) number = mode_min;

            // Show only the selected display, or menu item text
            menuShow(number);

        } else {

            mydelay(50);

        }

    }

    return false;
}

/*
 *  Displays the current main menu item
 *
 */
static void menuShow(int number)
{
    displaySet = NULL;
    char buf[32];
    
    switch (number) {
    case MODE_DEST:  // Destination Time
        dt_on();
        destinationTime.setColon(false);
        destinationTime.show();
        pt_off();
        lt_off();
        displaySet = &destinationTime;
        break;
    case MODE_PRES:  // Present Time (RTC)
        dt_showTextDirect("SET RTC");
        dt_on();
        pt_on();
        presentTime.setColon(false);
        presentTime.show();
        lt_off();
        displaySet = &presentTime;
        break;
    case MODE_DEPT:  // Last Time Departed
        dt_off();
        pt_off();
        lt_on();
        departedTime.setColon(false);
        departedTime.show();
        displaySet = &departedTime;
        break;
    case MODE_VOL:   // Software volume
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("VOLUME");
        pt_off();
        #else
        dt_showTextDirect("VOL");    // "M" on 7seg no good, 2 lines
        pt_showTextDirect("UME");
        pt_on();
        #endif
        dt_on();
        lt_off();
        break;
    case MODE_MSFX:   // Music Folder Number
        dt_showTextDirect("MUSIC");
        pt_showTextDirect("FOLDER");
        lt_showTextDirect("NUMBER");
        pt_on();
        dt_on();
        lt_on();
        break;
    case MODE_ALRM:   // Alarm
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("ALARM");
        pt_off();
        #else
        dt_showTextDirect("ALA");    // "M" on 7seg no good, 2 lines
        pt_showTextDirect("RM");
        pt_on();
        #endif
        dt_on();
        lt_off();
        displaySet = &destinationTime;
        break;
    case MODE_AINT:  // Time Cycling inverval
        dt_showTextDirect("TIME");
        pt_showTextDirect("CYCLING");
        dt_on();
        pt_on();
        lt_off();
        break;
    case MODE_BRI:  // Brightness
        dt_showTextDirect("BRIGHTNESS");
        pt_off();
        lt_off();
        dt_on();
        break;
    case MODE_NET:  // Network info
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("NETWORK");
        dt_on();
        pt_off();
        #else
        dt_showTextDirect("NET");  // "W" on 7seg no good, 2 lines
        pt_showTextDirect("WORK");
        dt_on();
        pt_on();
        #endif
        lt_off();
        break;
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    case MODE_SENS:
        dt_showTextDirect("SENSORS");
        dt_on();
        pt_off();
        lt_off();
        break;
    #endif
    case MODE_LTS:  // last time sync info
        dt_showTextDirect("TIME SYNC");
        dt_on();
        if(!lastAuthTime64) {
            pt_showTextDirect("NEVER");
            pt_on();
            lt_off();
        } else {
            uint64_t ago = (((uint64_t)millis() + millisEpoch) - lastAuthTime64) / 1000;
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
            pt_on();
            lt_showTextDirect("AGO");
            lt_on();
        }
        break;
    #ifdef TC_HAVEBTTFN
    case MODE_CLI:
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("BTTFN");
        pt_showTextDirect("CLIENTS");
        pt_on();
        #else
        dt_showTextDirect("BTTFN CLIENTS");
        pt_off();
        #endif
        dt_on();
        lt_off();
        break;
    #endif
    case MODE_VER:  // Version info
        dt_showTextDirect("VERSION");
        dt_on();
        pt_showTextDirect(TC_VERSION);
        pt_on();
        #ifdef TC_VERSION_EXTRA
        lt_showTextDirect(TC_VERSION_EXTRA);
        lt_on();
        #else
        lt_off();
        #endif
        break;
    case MODE_CPA:  // Install audio files
        dt_showTextDirect("INSTALL");
        dt_on();
        pt_showTextDirect("AUDIO FILES");
        pt_on();
        lt_off();
        break;
    case MODE_END:  // end
        dt_showTextDirect("END");
        dt_on();
        destinationTime.setColon(false);
        pt_off();
        lt_off();
        break;
    }
}

/*
 * Show only the field to be updated
 * and set up pre-set contents
 *
 * number = number to show
 * field = field to show it in
 *
 */
static void setUpdate(uint16_t number, int field, uint16_t dflags)
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
static bool setField(uint16_t& number, uint8_t field, int year, int month, uint16_t dflags)
{
    int upperLimit;
    int lowerLimit;
    int i;
    uint16_t setNum = 0, prevNum = number;
    int16_t  numVal = 0;
    int numChars = 2;
    bool someupddone = false;
    unsigned long blinkNow = 0;
    bool blinkSwitch = true;

    timeout = 0;  // reset timeout

    // Start blinking
    displaySet->onBlink(2);

    // No, timeBuffer is pre-set with previous value
    //timeBuffer[0] = '\0';   

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
        numChars = 4;
        isYearUpdate = true;
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

    // Force keypad to send keys to our buffer (and block key holding)
    keypadInMenu = true;

    // Reset key state machine
    resetKeypadState();

    while( !checkTimeOut() && 
           !checkEnterPress() &&
           ( (!someupddone && number == prevNum) || strlen(timeBuffer) < numChars) ) {

        // We're outside our main loop, so poll here
        scanKeypad();

        setNum = 0;
        for(i = 0; i < strlen(timeBuffer); i++) {
            setNum *= 10;
            setNum += (timeBuffer[i] - '0');
        }

        // Show setNum in field
        if(prevNum != setNum) {
            someupddone = true;
            setUpdate(setNum, field, dflags | CDD_NOLEAD0);
            prevNum = setNum;
            timeout = 0;  // key pressed, reset timeout
            displaySet->on();
            blinkNow = millis();
            blinkSwitch = false;
        }

        mydelay(10);

        if(!blinkSwitch && (millis() - blinkNow > 500)) {
            displaySet->onBlink(2);
            blinkSwitch = true;
        }

    }

    isYearUpdate = false;
    
    // Force keypad to send keys somewhere else but our buffer
    keypadInMenu = false;

    // Stop blinking
    displaySet->on();

    if(checkTimeOut())
        return false;

    numVal = 0;
    for(i = 0; i < strlen(timeBuffer); i++) {
       numVal *= 10;
       numVal += (timeBuffer[i] - '0');
    }
    if(numVal < lowerLimit) numVal = lowerLimit;
    if(numVal > upperLimit) numVal = upperLimit;
    number = numVal;

    // Display (corrected) number for .5 seconds
    setUpdate((uint16_t)numVal, field, dflags);

    mydelay(500);

    return true;
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
            dt_showTextDirect("USE");
            pt_showTextDirect("VOLUME");
            lt_showTextDirect("KNOB");
            lt_on();
        } else {
            dt_showTextDirect("FIXED");
            pt_showTextDirect("LEVEL");
            lt_off();
        }
        dt_on();
        pt_on();
    }
}

static void showCurVol(bool blink, bool doComment)
{
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("LEVEL", curVolume, flags);
    dt_on();

    if(doComment) {
        if(curVolume == 0) {
            pt_showTextDirect("MUTE");
            pt_on();
        } else {
            pt_off();
        }
        lt_off();
    }
}

static void doSetVolume()
{
    bool volDone = false;
    uint8_t oldVol = curVolume;
    unsigned long playNow;
    bool triggerPlay = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();

    showCurVolHWSW(false);

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !volDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(blinkSwitch) {
                showCurVolHWSW(false);
            }

            if(!(volDone = menuWaitForRelease())) {

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

            mydelay(50);

        }

    }

    if(volDone && curVolume != 255) {

        curVolume = (oldVol == 255) ? 0 : oldVol;

        showCurVol(false, true);

        timeout = 0;  // reset timeout

        blinkSwitch = false;
        blinkNow = millis();

        volDone = false;

        waitForEnterRelease();

        // Wait for enter
        while(!checkTimeOut() && !volDone) {

            // If pressed
            if(checkEnterPress()) {

                timeout = 0;  // button pressed, reset timeout

                if(blinkSwitch) {
                    showCurVol(false, false);
                }
                
                if(!(volDone = menuWaitForRelease())) {

                    curVolume++;
                    if(curVolume == 20) curVolume = 0;

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
                    play_file("/alarm.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                    triggerPlay = false;
                }

                mydelay(50);

            }
        }

    }

    stopAudio();
    allOff();

    if(volDone) {

        dt_showTextDirect(StrSaving);
        dt_on();
        pt_off();
        lt_off();

        // Save it (if changed)
        if(oldVol != curVolume) {
            saveCurVolume();
        }
        mydelay(1000);

    } else {

        curVolume = oldVol;

    }
}

/*  
 *   Music Folder Number ########################################
 */

static void displayMSfx(int msfx, bool blink, bool doFolderChk)
{
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("FOLDER", msfx, flags);
    dt_on();

    if(doFolderChk) {
        int folderState = mp_checkForFolder(msfx);
        switch(folderState) {
        case 1:
            pt_off();
            lt_off();
            break;
        case 0:
            pt_showTextDirect("NOT FOUND");
            pt_on();
            lt_off();
            break;
        case -1:
            pt_showTextDirect("PROCESSING");
            lt_showTextDirect("REQUIRED");
            break;
        case -2:
            pt_showTextDirect("NO AUDIO");
            lt_showTextDirect("FILES");
            break;
        case -3:
            pt_showTextDirect("NOT A");
            lt_showTextDirect("FOLDER");
            break;
        }
        if(folderState < 0) {
            pt_on();
            lt_on();
        }
    }
}

static void doSetMSfx()
{
    bool msfxDone = false;
    int oldmsfx = musFolderNum;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();

    displayMSfx(musFolderNum, false, true);

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !msfxDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(blinkSwitch) {
                displayMSfx(musFolderNum, false, false);
            }

            if(!(msfxDone = menuWaitForRelease())) {

                musFolderNum++;
                if(musFolderNum > 9)
                    musFolderNum = 0;

                displayMSfx(musFolderNum, false, true);

                blinkNow = millis();
                blinkSwitch = false;

            }

        } else {

            unsigned long mm = millis();
                
            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                displayMSfx(musFolderNum, blinkSwitch, false);
                blinkNow = mm;
            }

            mydelay(50);

        }

    }

    if(msfxDone) {

        dt_showTextDirect(StrSaving);
        pt_off();
        lt_off();

        // Save it (if changed)
        if(oldmsfx != musFolderNum) {
            saveMusFoldNum();
            mp_init();
        }

        mydelay(1000);

    } else {

        musFolderNum = oldmsfx;
        
    }
}

/*
 *  Alarm ######################################################
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

int toggleAlarm()
{
    if(alarmOnOff) {
        alarmOff();
        return 0;
    }
    return alarmOn() ? 1 : -1;
}

int getAlarm()
{
    if(alarmHour <= 23 && alarmMinute <= 59) {
        return (alarmHour << 8) | alarmMinute;
    }
    return -1;
}

const char *getAlWD(int wd)
{
    return alarmWD[wd];
}

// Set Alarm
static void doSetAlarm()
{
    char almBuf[16];
    char almBuf2[16];
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool alarmDone = false;
    bool newAlarmOnOff = alarmOnOff;
    uint16_t newAlarmHour = (alarmHour <= 23) ? alarmHour : 0;
    uint16_t newAlarmMinute = (alarmMinute <= 59) ? alarmMinute : 0;
    uint16_t newAlarmWeekday = alarmWeekday;
    #ifdef IS_ACAR_DISPLAY
    const char *almFmt = "%s     %02d%02d";
    #else
    const char *almFmt = "%s      %02d%02d";
    #endif

    // On/Off
    sprintf(almBuf2, almFmt, "   " , newAlarmHour, newAlarmMinute);
    sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
    displaySet->showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
    displaySet->on();

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(blinkSwitch) {
                displaySet->showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
            }

            if(!(alarmDone = menuWaitForRelease())) {

                newAlarmOnOff = !newAlarmOnOff;

                sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
                displaySet->showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);

                blinkSwitch = false;
                blinkNow = millis();

            }

        } else {

            unsigned long mm = millis();
            
            if(mm - blinkNow > 500) {
                blinkSwitch = !blinkSwitch;
                displaySet->showTextDirect(blinkSwitch ? almBuf2 : almBuf, CDT_CLEAR|CDT_COLON);
                blinkNow = mm;
            }

            mydelay(50);

        }

    }

    if(!alarmDone) return;

    // Get hour
    setUpdate(newAlarmHour, FIELD_HOUR, CDD_FORCE24);
    prepareInput(newAlarmHour);
    waitForEnterRelease();
    if(!setField(newAlarmHour, FIELD_HOUR, 0, 0, CDD_FORCE24)) {
        waitAudioDone();    // (keypad; should not be necessary)
        return;
    }

    // Get minute
    setUpdate(newAlarmMinute, FIELD_MINUTE);
    prepareInput(newAlarmMinute);
    waitForEnterRelease();
    if(!setField(newAlarmMinute, FIELD_MINUTE)) {
        waitAudioDone();    // (keypad; should not be necessary)
        return;
    }

    // Weekday(s)
    waitForEnterRelease();
    displaySet->showTextDirect(getAlWD(newAlarmWeekday));
    displaySet->onBlink(2);
    blinkSwitch = true;

    alarmDone = false;
    timeout = 0;

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout
            displaySet->on();
            blinkSwitch = false;

            if(!(alarmDone = menuWaitForRelease())) {

                newAlarmWeekday++;
                if(newAlarmWeekday > 9) newAlarmWeekday = 0;

                displaySet->showTextDirect(getAlWD(newAlarmWeekday));

            }

        } else {

            mydelay(50);

            if(!blinkSwitch) {
                displaySet->onBlink(2);
                blinkSwitch = true;
            }

        }

    }

    waitAudioDone();    // For keypad sounds
    allOff();

    if(alarmDone) {

        displaySet->showTextDirect(StrSaving);
        displaySet->on();

        // Save it (if changed)
        if( (alarmOnOff != newAlarmOnOff)   ||
            (alarmHour != newAlarmHour)     ||
            (alarmMinute != newAlarmMinute) ||
            (alarmWeekday != newAlarmWeekday) ) {
        
            alarmOnOff = newAlarmOnOff;
            alarmHour = newAlarmHour;
            alarmMinute = newAlarmMinute;
            alarmWeekday = newAlarmWeekday;

            saveAlarm();
        }

        mydelay(1000);
    }
}

/*
 *  Time-cycling Interval (aka "autoInterval") #################
 */

/*
 *  Load the autoInterval from Settings in memory (config file is not reloaded)
 *
 */
bool loadAutoInterval()
{
    autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
    if(autoInterval > 5) {
        autoInterval = DEF_AUTOROTTIMES;
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
static void doSetAutoInterval()
{
    bool autoDone = false;
    int newAutoInterval = autoInterval;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();

    displayAI(autoTimeIntervals[newAutoInterval], false, true);
    dt_on();
    pt_on();
    lt_off();

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !autoDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(blinkSwitch) {
                displayAI(autoTimeIntervals[newAutoInterval], false, false);
            }

            if(!(autoDone = menuWaitForRelease())) {

                newAutoInterval++;
                if(newAutoInterval > 5)
                    newAutoInterval = 0;

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

            mydelay(50);

        }

    }

    if(autoDone) {

        dt_showTextDirect(StrSaving);
        pt_off();
        lt_off();

        // Save it (if changed)
        if(autoInterval != newAutoInterval) {
            autoInterval = newAutoInterval;
            saveAutoInterval();
            updateConfigPortalValues();
        }

        // End pause if current setting != off
        if(autoTimeIntervals[autoInterval]) 
            endPauseAuto();

        mydelay(1000);

    }
}

/*
 * Brightness ##################################################
 */

static void displayBri(clockDisplay* displaySet, int8_t number, bool blink)
{
    uint16_t flags = 0;
    if(blink) flags |= CDT_BLINK;
    
    #ifdef IS_ACAR_DISPLAY
    displaySet->showSettingValDirect("LV", number, flags);
    #else
    displaySet->showSettingValDirect("LVL", number, flags);
    #endif
}


static bool doSetBrightness(clockDisplay* displaySet)
{
    int8_t number = displaySet->getBrightness();
    bool briDone = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();

    // turn on all the segments
    allLampTest();

    // Display "LVL"
    displayBri(displaySet, number, false);

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !briDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(blinkSwitch) {
                displayBri(displaySet, number, false);
            }

            if(!(briDone = menuWaitForRelease())) {

                number++;
                if(number > 15) number = 0;

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

            mydelay(50);

        }

    }

    allLampTest();  // turn on all the segments

    return briDone;
}

/*
 * Show sensor info ############################################
 */

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

    #ifdef TC_HAVELIGHT
    if(useLight) numberArr[numIdx++] = 0;
    #endif
    #ifdef TC_HAVETEMP
    if(useTemp) {  
        numberArr[numIdx++] = 1;
        if(tempSens.haveHum()) numberArr[numIdx++] = 2;
    }
    #endif
    maxIdx = numIdx - 1;
    numIdx = 0;

    dt_showTextDirect("WAIT");
    dt_on();
    pt_showTextDirect("");
    pt_on();
    lt_off();

    isEnterKeyHeld = false;

    sensNow = millis();

    // Don't use timeOut here, user might want to keep
    // sensor data displayed for a longer period.

    // Wait for enter
    while(!luxDone) {

        // If pressed
        if(checkEnterPress()) {

            if(!(luxDone = menuWaitForRelease())) {
                if(maxIdx > 0) {
                    numIdx++;
                    if(numIdx > maxIdx) numIdx = 0;
                    dt_showTextDirect("WAIT");
                    pt_showTextDirect("");
                }
            }
            
        } else {

            mydelay(50);

            if(millis() - sensNow > 3000) {
                switch(numberArr[numIdx]) {
                case 0:
                    #ifdef TC_HAVELIGHT
                    lightSens.loop();
                    dt_showTextDirect("LIGHT");
                    //#ifdef TC_DBG
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
                    temp = tempSens.readTemp(tempUnit);
                    if(isnan(temp)) {
                        sprintf(buf, "--.--~%c", tempUnit ? 'C' : 'F');
                    } else {
                        sprintf(buf, "%.2f~%c", temp, tempUnit ? 'C' : 'F');
                    }
                    #else
                    buf[0] = 0;
                    #endif
                    break;
                case 2:
                    #ifdef TC_HAVETEMP
                    tempSens.readTemp(tempUnit);
                    dt_showTextDirect("HUMIDITY");
                    hum = tempSens.readHum();
                    if(hum < 0) {
                        #ifdef IS_ACAR_DISPLAY
                        sprintf(buf, "--\x7f\x80", hum);
                        #else
                        sprintf(buf, "-- \x7f\x80", hum);
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
    
    dt_on();
    pt_on();
    lt_on();
}

static void doShowNetInfo()
{
    int number = 0;
    bool netDone = false;
    char macBuf[16];
    int maxMI = 2;

    #ifdef TC_HAVEMQTT
    maxMI = 3;
    #endif

    wifi_getMAC(macBuf);

    displayIP();

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !netDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(!(netDone = menuWaitForRelease())) {

                number++;
                if(number > maxMI) number = 0;
                switch(number) {
                case 0:
                    displayIP();
                    break;
                case 1:
                    dt_showTextDirect("WIFI");
                    dt_on();
                    switch(wifi_getStatus()) {
                    case WL_IDLE_STATUS:
                        pt_showTextDirect("IDLE");
                        lt_off();
                        break;
                    case WL_SCAN_COMPLETED:
                        pt_showTextDirect("SCAN");
                        lt_showTextDirect("COMPLETE");
                        lt_on();
                        break;
                    case WL_NO_SSID_AVAIL:
                        pt_showTextDirect("SSID NOT");
                        lt_showTextDirect("AVAILABLE");
                        lt_on();
                        break;
                    case WL_CONNECTED:
                        pt_showTextDirect("CONNECTED");
                        lt_off();
                        break;
                    case WL_CONNECT_FAILED:
                        pt_showTextDirect("CONNECT");
                        lt_showTextDirect("FAILED");
                        lt_on();
                        break;
                    case WL_CONNECTION_LOST:
                        pt_showTextDirect("CONNECTION");
                        lt_showTextDirect("LOST");
                        lt_on();
                        break;
                    case WL_DISCONNECTED:
                        pt_showTextDirect("DISCONNECTED");
                        lt_off();
                        break;
                    case 0x10000:
                    case 0x10003:   // AP-STA, treat as AP (=AP with conn.config'd but not connected)
                        pt_showTextDirect("AP MODE");
                        lt_off();
                        break;
                    case 0x10001:
                        pt_showTextDirect("OFF");
                        lt_off();
                        break;
                    //case 0x10002:     // UNKNOWN
                    default:
                        pt_showTextDirect("UNKNOWN");
                        lt_off();
                    }
                    pt_on();
                    break;
                case 2:
                    dt_showTextDirect("MAC");
                    dt_on();
                    pt_showTextDirect(macBuf, CDT_CLEAR|CDT_CORR6);
                    pt_on();
                    lt_off();
                    break;
                #ifdef TC_HAVEMQTT
                case 3:
                    dt_showTextDirect("HOMEASSISTANT");
                    dt_on();
                    if(useMQTT) {
                        pt_showTextDirect(mqttState() ? "CONNECTED" : "DISCONNECTED");
                    } else {
                        pt_showTextDirect("OFF");
                    }
                    pt_on();
                    lt_off();
                    break;
                #endif
                }

            }

        } else {

            mydelay(50);

        }

    }
}

/*
 * Show BTTFN network clients ##################################
 */
#ifdef TC_HAVEBTTFN
static void displayClient(int numCli, int number)
{
    uint8_t *ip;
    char *id;
    uint8_t type;
    
    if(!numCli) {
        dt_showTextDirect("NO CLIENTS");
        pt_off();
        lt_off();
        return;
    }

    if(number >= numCli) number = numCli - 1;

    if(bttfnGetClientInfo(number, &id, &ip, &type)) {
        dt_showTextDirect(id);
        presentTime.showHalfIPDirect(ip[0], ip[1], CDT_CLEAR);
        departedTime.showHalfIPDirect(ip[2], ip[3], CDT_CLEAR);
        pt_on();
        lt_on();
        #ifdef TC_DBG
        Serial.printf("BTTFN client type %d\n", type);
        #endif
    } else {
        dt_showTextDirect("EXPIRED");
        pt_off();
        lt_off();
    }
}

void doShowBTTFNInfo()
{
    int number = 0;
    bool netDone = false;
    int numCli = bttfnNumClients();
    int oldNumCli;

    oldNumCli = numCli;

    dt_on();

    displayClient(numCli, number);

    isEnterKeyHeld = false;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !netDone) {

        if(oldNumCli != numCli) {
            number = 0;
            displayClient(numCli, number);
            oldNumCli = numCli;
        }

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout

            if(!(netDone = menuWaitForRelease())) {

                if(numCli > 1) {
                    number++;
                    if(number >= numCli) number = 0;                    
                } else
                    number = 0;

                displayClient(numCli, number);

            }

        } else {

            mydelay(50);
            numCli = bttfnNumClients();

        }

    }
}
#endif

/*
 * Install default audio files from SD to flash FS #############
 */

void doCopyAudioFiles()
{
    bool doCancDone = false;
    bool newCanc = false;
    bool blinkSwitch;
    bool delIDfile = false;

    // Cancel/Copy
    dt_on();
    pt_on();
    lt_showTextDirect("CANCEL");
    departedTime.onBlink(2);
    blinkSwitch = true;

    timeout = 0;  // reset timeout

    // Wait for enter
    while(!checkTimeOut() && !doCancDone) {

        // If pressed
        if(checkEnterPress()) {

            timeout = 0;  // button pressed, reset timeout
            lt_on();
            blinkSwitch = false;

            if(!(doCancDone = menuWaitForRelease())) {

                newCanc = !newCanc;
  
                lt_showTextDirect(newCanc ? "PROCEED" : "CANCEL");
  
            }
  
        } else {
  
            mydelay(50);

            if(!blinkSwitch) {
                departedTime.onBlink(2);
                blinkSwitch = true;
            }
  
        }

    }

    lt_on();

    if(!doCancDone || !newCanc) return;

    if(!copy_audio_files()) {
        // If copy fails, re-format flash FS
        lt_showTextDirect("FORMATTING");
        formatFlashFS();            // Format
        rewriteSecondarySettings(); // Re-write alarm/reminder/ip/vol settings
        #ifdef TC_DBG 
        Serial.println("Re-writing general settings");
        #endif
        write_settings();           // Re-write general settings
        if(!copy_audio_files()) {   // Retry copy
            mydelay(3000);
        } else {
            delIDfile = true;
        }
    } else {
        delIDfile = true;
    }

    if(delIDfile)
        delete_ID_file();
    
    mydelay(2000);

    allOff();
    dt_showTextDirect("REBOOTING");
    dt_on();

    esp_restart();
}

/* *** Helpers ################################################### */

void start_file_copy()
{
    mp_stop();
    stopAudio();
  
    dt_showTextDirect("COPYING");
    pt_showTextDirect("FILES");
    lt_showTextDirect("PLEASE");
    dt_on();
    pt_on();
    lt_on();
    destinationTime.resetBrightness();
    presentTime.resetBrightness();
    departedTime.resetBrightness();
    
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

void file_copy_done()
{
    lt_showTextDirect("DONE");
}

void file_copy_error()
{
    lt_showTextDirect("ERROR");
}

/*
 * Wait for ENTER to be released
 * Returns 
 * - true if ENTER was long enough pressed to be considered HELD
 * - false if ENTER released before considered "held"
 */
static bool menuWaitForRelease()
{
    while(checkEnterPress()) {
        myloop();
        if(isEnterKeyHeld) {
            isEnterKeyHeld = false;
            return true;
        }
        delay(10);
    }

    return false;
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
void waitForEnterRelease()
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
        delay(5);
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
    audio_loop();
    enterkeyScan();   // >= 19ms
    audio_loop();
    wifi_loop();
    audio_loop();
    ntp_loop();
    audio_loop();
    #ifdef TC_HAVEGPS
    gps_loop();       // >= 12ms
    #endif
    #ifdef TC_HAVEBTTFN
    #ifdef TC_HAVEGPS
    audio_loop();
    #endif
    bttfn_loop();
    #endif
}
