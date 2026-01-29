/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Keypad Menu handling
 *
 * Based on code by John Monaco, Marmoset Electronics
 * https://www.marmosetelectronics.com/time-circuits-clock
 * -------------------------------------------------------------------
 * License: MIT NON-AI
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
 * The menu is controlled by "pressing" or "holding" the ENTER key on the 
 * keypad.
 *
 * A "press" is shorter than 2 seconds, a "hold" is 2 seconds or longer.
 * 
 * Since 3.11, keys 2 (up), 5 (select), 8 (down) and 9 (cancel) are also
 * a way of navigating the keypad menu (unless numerical input is awaited).
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
 *     - show currently registered BTTF clients ("BTTF CLIENTS"),
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
 *     Commands 3xx (00-19; 99) also allow configuring the volume; using
 *     a rotary encoder for volume requires to set a fixed level (and thereby
 *     disabling the built-in volume knob)
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
 * How to find out the IP address, WiFi status, MAC address, HA Status
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
#define MODE_END  13
#define MODE_MAX  MODE_END

#define FIELD_MONTH   0
#define FIELD_DAY     1
#define FIELD_YEAR    2
#define FIELD_HOUR    3
#define FIELD_MINUTE  4

uint8_t autoInterval = DEF_AUTOROTTIMES;
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

int  keypadMode = 0;
bool isYearUpdate = false;

static clockDisplay* displaySet;

// File copy progress
static bool fcprog = false;
static unsigned long fcstart = 0;

// Our generic timeout when waiting for buttons
#define MENUTIMEOUT 60000   // 60 seconds
static unsigned long timeout = 0;

static const char *StrSaving = "SAVING";
static const char *alarmWD[10] = {
      #ifdef IS_ACAR_DISPLAY
      "MON-SUN", "MON-FRI", "SAT-SUN",
      #else
      "MON -SUN", "MON -FRI", "SAT -SUN",
      #endif
      "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"
};

static bool menuSelect(int& number, int mode_min, DateTime& dtu, DateTime& dtl);
static void menuShow(int number);
static void setUpdate(int number, int field, uint16_t dflags = 0);
static bool setField(int& number, uint8_t field, int year = 0, int month = 0, uint16_t dflags = 0);
static void showCurVolHWSW(bool blink);
static void showCurVol(bool blink, bool doComment);
static void doSetVolume();
static void displayMSfx(int msfx, bool blink, bool doFolderChk);
static void doSetMSfx();
static void doSetAlarm();
static void displayAI(int interval, bool blink, bool doComment);
static void doSetAutoInterval();
static bool doSetBrightness(clockDisplay* displaySet);
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
static void doShowSensors();
#endif
static void displayIP();
static void doShowNetInfo();
static void doShowBTTFNInfo();
static bool menuWaitForRelease();
static bool checkEnterPress();
static bool waitForMenuControl(bool &wasEnter, bool& dirDown, bool& wasQuit, bool& wasSelect);
static void prepareInput(int number);

static void resetTimeOut();
static bool checkTimeOut();

static void waitAudioDoneMenu();
static void menuDelay(unsigned long mydel);
static void menuLoops();
//static void menuShortDelay(unsigned long mydel);

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

#define D_D 1
#define D_P 2
#define D_L 4
static void sw_sel(int w) {
    (w & D_D) ? dt_on() : dt_off();
    (w & D_P) ? pt_on() : pt_off();
    (w & D_L) ? lt_on() : lt_off();
}

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
    int y1, m1, d1, h1, mi1;
    int y2, m2, d2, h2, mi2;
    DateTime dtu, dtl;

    pwrNeedFullNow();

    isEnterKeyHeld = false;
    isEnterKeyPressed = false;

    destinationTime.setNightMode(false);
    presentTime.setNightMode(false);
    departedTime.setNightMode(false);
    destinationTime.resetBrightness();
    presentTime.resetBrightness();
    departedTime.resetBrightness();
    // Do not propagate through BTTFN, menu is TCD-private

    mpActive = mp_stop();
    stopAudio();

    flushDelayedSave();

    // start with first menu item
    menuItemNum = mode_min = MODE_ALRM;

    // Backup currently displayed times
    destinationTime.getToParms(y1, m1, d1, h1, mi1);
    departedTime.getToParms(y2, m2, d2, h2, mi2);
    // Load the custom times from NVM
    destinationTime.load();
    departedTime.load();

    // Load local time into present time
    myrtcnow(dtu);
    UTCtoLocal(dtu, dtl, 0);
    presentTime.setDateTime(dtl);

    // Show first menu item
    menuShow(menuItemNum);

    waitForEnterRelease();

    // menuSelect:
    // Wait for ENTER to cycle through main menu,
    // HOLDing ENTER selects current menu "item"
    // (Also sets displaySet to selected display, if applicable)
    if(!(menuSelect(menuItemNum, mode_min, dtu, dtl)))
        goto quitMenu;

    if(menuItemNum >= MODE_PRES && menuItemNum <= MODE_DEPT) {

        // Enter display times

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

        waitAudioDoneMenu();

        // Show a save message for a brief moment
        displaySet->showTextDirect(StrSaving);

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

            if(autoInterval) {
                autoInterval = 0;
                saveBeepAutoInterval();
            }

            nonRTCdisplayChanged = true;

        }

        // update the object
        displaySet->setFromParms(yearSet, monthSet, daySet, hourSet, minSet);

        // Save to NVM (regardless of persistence mode)
        displaySet->save();

        menuDelay(1000);

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
        if(!doSetBrightness(&presentTime)) {
            destinationTime.setBrightness(dtbri);
            goto quitMenu;
        }

        waitForEnterRelease();
        if(!doSetBrightness(&departedTime)) {
            destinationTime.setBrightness(dtbri);
            presentTime.setBrightness(ptbri);
            goto quitMenu;
        }

        // Now save
        dt_showTextDirect(StrSaving);
        sw_sel(D_D);

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

            // Update & write settings file
            sprintf(settings.destTimeBright, "%d", dtbri2);
            sprintf(settings.presTimeBright, "%d", ptbri2);
            sprintf(settings.lastTimeBright, "%d", ltbri2);
            
            saveBrightness();
            
        }

        menuDelay(1000);

    } else if(menuItemNum == MODE_NET) {   // Show network info

        allOff();
        waitForEnterRelease();

        // Show net info
        doShowNetInfo();

    } else if(menuItemNum == MODE_CLI) {   // Show bttfn clients

        allOff();
        waitForEnterRelease();

        // Show client info
        doShowBTTFNInfo();
 
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    } else if(menuItemNum == MODE_SENS) {   // Show sensor info

        allOff();
        waitForEnterRelease();

        doShowSensors();
    #endif

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
            }
            if(!isWcMode() || !WcHaveTZ2) {
                  departedTime.setFromParms(y2, m2, d2, h2, mi2);
            }
        }
    } else {
        if(!isWcMode() || !WcHaveTZ1) destinationTime.setFromStruct(&destinationTimes[autoTime]);
        if(!isWcMode() || !WcHaveTZ2) departedTime.setFromStruct(&departedTimes[autoTime]);
    }
    destShowAlt = depShowAlt = 0; // Reset TZ-Name-Animation

    // Done, turn off displays
    allOff();

    menuDelay(500);

    waitForEnterRelease();

    // Reset keypad key state and clear input buffer
    keypadMode = 0;
    resetKeypadState();
    discardKeypadInput();

    // Restore present time
    
    myrtcnow(dtu);
    UTCtoLocal(dtu, dtl, 0);
    
    if(stalePresent)
        presentTime.setFromStruct(&stalePresentTime[1]);
    else
        updatePresentTime(dtu, dtl);

    if(isWcMode()) {
        setDatesTimesWC(dtu);
    }

    // all displays on and show

    animate();

    // Restore night mode
    destinationTime.setNightMode(desNM);
    presentTime.setNightMode(preNM);
    departedTime.setNightMode(depNM);

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
 *  Cycle through main menu
 *  
 *  Returns
 *  - true if item selected
 *  - false if timeout
 *
 */
static bool menuSelect(int& number, int mode_min, DateTime& dtu, DateTime& dtl)
{
    bool wasEnter, dirDown, wasQuit, wasSelect;

    keypadKeyPressed = 0;
    resetKeypadState();
    keypadMode = 2;
    
    resetTimeOut();

    // Wait for enter
    while(!checkTimeOut()) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            resetTimeOut();

            if(wasQuit) break;
                
            if(wasSelect || (wasEnter && menuWaitForRelease())) {
                keypadMode = 0;
                return true;
            }

            if(wasEnter || !dirDown) {
                number++;
                if(number == MODE_MSFX && !haveSD) number++;
                #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
                if(number == MODE_SENS && !useLight && !useTemp) number++;
                #else
                if(number == MODE_SENS) number++;
                #endif
                if(number > MODE_MAX) number = wasEnter ? mode_min : MODE_MAX;
            } else {
                if(number > 0) number--;
                #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
                if(number == MODE_SENS && !useLight && !useTemp) number--;
                #else
                if(number == MODE_SENS) number--;
                #endif
                if(number == MODE_MSFX && !haveSD) number--;
            }

            if(number == MODE_PRES) {
                myrtcnow(dtu);
                UTCtoLocal(dtu, dtl, 0);
                presentTime.setDateTime(dtl);
            }

            // Show only the selected display, or menu item text
            menuShow(number);

        } else {

            menuDelay(50);

        }

    }

    keypadMode = 0;

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
        displaySet = &destinationTime;
        sw_sel(D_D);
        displaySet->setColon(true);
        displaySet->show();
        break;
    case MODE_PRES:  // Present Time (RTC)
        displaySet = &presentTime;
        dt_showTextDirect("SET CLOCK");
        sw_sel(D_P|D_D);
        displaySet->setColon(true);
        displaySet->show();
        break;
    case MODE_DEPT:  // Last Time Departed
        displaySet = &departedTime;
        sw_sel(D_L);
        displaySet->setColon(true);
        displaySet->show();
        break;
    case MODE_VOL:   // Software volume
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("VOLUME");
        sw_sel(D_D);
        #else
        dt_showTextDirect("VOL");    // "M" on 7seg no good, 2 lines
        pt_showTextDirect("UME");
        sw_sel(D_P|D_D);
        #endif
        break;
    case MODE_MSFX:   // Music Folder Number
        dt_showTextDirect("MUSIC");
        pt_showTextDirect("FOLDER");
        lt_showTextDirect("NUMBER");
        sw_sel(D_L|D_P|D_D);
        break;
    case MODE_ALRM:   // Alarm
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("ALARM");
        sw_sel(D_D);
        #else
        dt_showTextDirect("ALA");    // "M" on 7seg no good, 2 lines
        pt_showTextDirect("RM");
        sw_sel(D_P|D_D);
        #endif
        displaySet = &destinationTime;
        break;
    case MODE_AINT:  // Time Cycling inverval
        dt_showTextDirect("TIME");
        pt_showTextDirect("CYCLING");
        sw_sel(D_P|D_D);
        break;
    case MODE_BRI:  // Brightness
        dt_showTextDirect("BRIGHTNESS");
        sw_sel(D_D);
        break;
    case MODE_NET:  // Network info
        #ifdef IS_ACAR_DISPLAY
        dt_showTextDirect("NETWORK");
        sw_sel(D_D);
        #else
        dt_showTextDirect("NET");  // "W" on 7seg no good, 2 lines
        pt_showTextDirect("WORK");
        sw_sel(D_P|D_D);
        #endif
        break;
    #if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
    case MODE_SENS:
        dt_showTextDirect("SENSORS");
        sw_sel(D_D);
        break;
    #endif
    case MODE_LTS:  // last time sync info
        dt_showTextDirect("TIME SYNC");
        if(!lastAuthTime64) {
            pt_showTextDirect("NEVER");
            sw_sel(D_P|D_D);
        } else {
            uint64_t ago = (millis64() - lastAuthTime64) / 1000;
            if(ago > 24*60*60) {
                sprintf(buf, "%d DAYS", (uint32_t)(ago / (24*60*60)));
            } else if(ago > 60*60) {
                sprintf(buf, "%d HOURS", (uint32_t)(ago / (60*60)));
            } else if(ago > 60) {
                sprintf(buf, "%d MINS", (uint32_t)(ago / 60));
            } else {
                sprintf(buf, "%d SECS", (uint32_t)ago);
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
    case MODE_END:  // end
        dt_showTextDirect("END");
        sw_sel(D_D);
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
static void setUpdate(int number, int field, uint16_t dflags)
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
static void prepareInput(int number)
{
    // Write pre-set into buffer, reset index to 0
    // Upon first key input, the pre-set will be overwritten
    // This allows the user to keep the pre-set by pressing
    // enter
    sprintf(timeBuffer, "%d", number);
    resetTimebufIndices();
}

static void prepareForInput()
{
    isEnterKeyHeld = false;

    keypadKeyPressed = 0;
    resetKeypadState();
    keypadMode = 2;

    resetTimeOut();  // reset timeout
}

/*
 * Update a field of a display using user input
 *
 * number - a value we're updating (pre-set and result of input)
 * field - field being modified, this will be displayed as it is updated
 * year, month - for checking maximum day number, etc.
 *
 */
static bool setField(int& number, uint8_t field, int year, int month, uint16_t dflags)
{
    int upperLimit;
    int lowerLimit;
    int i;
    int setNum = 0, prevNum = number;
    int numVal = 0;
    int numChars = 2;
    bool someupddone = false;
    unsigned long blinkNow = 0;
    bool blinkSwitch = true;

    resetTimeOut();  // reset timeout

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
    keypadMode = 1;

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
            resetTimeOut();  // key pressed, reset timeout
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

    isYearUpdate = false;
    
    // Force keypad to send keys somewhere else but our buffer
    keypadMode = 0;

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
    #ifdef TC_JULIAN_CAL
    if(field == FIELD_DAY) {
        correctNonExistingDate(year, month, numVal);
    }
    #endif
    number = numVal;

    // Display (corrected) number for .5 seconds
    setUpdate(numVal, field, dflags);

    menuDelay(500);

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
            sw_sel(D_L|D_P|D_D);
        } else {
            dt_showTextDirect("SELECT");
            pt_showTextDirect("LEVEL");
            sw_sel(D_P|D_D);
        }
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
    int oldVol = curVolume;
    unsigned long playNow;
    bool triggerPlay = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    showCurVolHWSW(false);

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !volDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            if(blinkSwitch) {
                showCurVolHWSW(false);
            }

            volDone = (wasSelect || (wasEnter && menuWaitForRelease()));

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

        // Wait for enter
        while(!checkTimeOut() && !volDone) {

            scanKeypad();

            if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

                if(wasQuit) break;
                
                resetTimeOut();  // button pressed, reset timeout

                if(blinkSwitch) {
                    showCurVol(false, false);
                }

                volDone = (wasSelect || (wasEnter && menuWaitForRelease()));
                
                if(!volDone) {

                    if(dirDown) {
                        curVolume++;
                        if(curVolume == 20) curVolume = 0;
                    } else {
                        if(curVolume > 0) curVolume--;
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
                    play_file("/alarm.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
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

        // Save it (if changed)
        if(oldVol != curVolume) {
            saveCurVolume();
        }
        menuDelay(1000);

    } else {

        curVolume = oldVol;

    }
}

/*  
 *   Music Folder Number ########################################
 */

static void displayMSfx(int msfx, bool blink, bool doFolderChk, int& folderState)
{
    uint16_t flags = CDT_CLEAR;
    if(blink) flags |= CDT_BLINK;
    
    destinationTime.showSettingValDirect("FOLDER", msfx, flags);
    dt_on();

    if(doFolderChk) {
        folderState = mp_checkForFolder(msfx);
        switch(folderState) {
        case 1:
            sw_sel(D_D);
            break;
        case 0:
            pt_showTextDirect("NOT FOUND");
            sw_sel(D_P|D_D);
            break;
        case -1:
            pt_showTextDirect("NEEDS");
            lt_showTextDirect("PROCESSING");
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
            sw_sel(D_L|D_P|D_D);
        }
    }
}

static void doSetMSfx()
{
    bool msfxDone = false;
    int oldmsfx = musFolderNum, folderState, dummy;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    displayMSfx(musFolderNum, false, true, folderState);

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !msfxDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            if(blinkSwitch) {
                displayMSfx(musFolderNum, false, false, dummy);
            }

            msfxDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!msfxDone) {

                if(dirDown) {
                    musFolderNum++;
                    if(musFolderNum > 9)
                        musFolderNum = wasEnter ? 0 : 9;
                } else {
                    if(musFolderNum > 0) musFolderNum--;
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

        // Save it (if changed)
        if(oldmsfx != musFolderNum) {
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
        }

        menuDelay(1000);

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
    int  newAlarmHour = (alarmHour <= 23) ? alarmHour : 0;
    int  newAlarmMinute = (alarmMinute <= 59) ? alarmMinute : 0;
    int  newAlarmWeekday = alarmWeekday;
    #ifdef IS_ACAR_DISPLAY
    const char *almFmt = "%s     %02d%02d";
    #else
    const char *almFmt = "%s      %02d%02d";
    #endif
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    // On/Off
    sprintf(almBuf2, almFmt, "   " , newAlarmHour, newAlarmMinute);
    sprintf(almBuf, almFmt, newAlarmOnOff ? "ON " : "OFF", newAlarmHour, newAlarmMinute);
    displaySet->showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
    displaySet->on();

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            if(blinkSwitch) {
                displaySet->showTextDirect(almBuf, CDT_CLEAR|CDT_COLON);
            }

            alarmDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!alarmDone) {

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

            menuDelay(50);

        }

    }

    keypadMode = 0;

    if(!(alarmDone & (!wasQuit))) return;

    // Get hour
    setUpdate(newAlarmHour, FIELD_HOUR, CDD_FORCE24);
    prepareInput(newAlarmHour);
    waitForEnterRelease();
    if(!setField(newAlarmHour, FIELD_HOUR, 0, 0, CDD_FORCE24)) {
        waitAudioDoneMenu();    // (keypad; should not be necessary)
        return;
    }

    // Get minute
    setUpdate(newAlarmMinute, FIELD_MINUTE);
    prepareInput(newAlarmMinute);
    waitForEnterRelease();
    if(!setField(newAlarmMinute, FIELD_MINUTE)) {
        waitAudioDoneMenu();    // (keypad; should not be necessary)
        return;
    }

    // Weekday(s)
    waitForEnterRelease();
    displaySet->showTextDirect(getAlWD(newAlarmWeekday));
    displaySet->onBlink(2);
    blinkSwitch = true;

    prepareForInput();

    alarmDone = false;

    // Wait for enter
    while(!checkTimeOut() && !alarmDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout
            displaySet->on();
            blinkSwitch = false;

            alarmDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!alarmDone) {

                if(wasEnter || !dirDown) {
                    newAlarmWeekday++;
                    if(newAlarmWeekday > 9) newAlarmWeekday = wasEnter ? 0 : 9;
                } else {
                    if(newAlarmWeekday > 0) newAlarmWeekday--;
                }

                displaySet->showTextDirect(getAlWD(newAlarmWeekday));

            }

        } else {

            menuDelay(50);

            if(!blinkSwitch) {
                displaySet->onBlink(2);
                blinkSwitch = true;
            }

        }

    }

    keypadMode = 0;

    waitAudioDoneMenu();    // For keypad sounds
    allOff();

    if(alarmDone & (!wasQuit)) {

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

        menuDelay(1000);
    }
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
static void doSetAutoInterval()
{
    bool autoDone = false;
    int newAutoInterval = autoInterval;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    displayAI(autoTimeIntervals[newAutoInterval], false, true);
    dt_on();
    pt_on();
    lt_off();

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !autoDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            if(blinkSwitch) {
                displayAI(autoTimeIntervals[newAutoInterval], false, false);
            }

            autoDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!autoDone) {

                if(dirDown) {
                    newAutoInterval++;
                    if(newAutoInterval > 5)
                        newAutoInterval = wasEnter ? 0 : 5;
                } else {
                    if(newAutoInterval > 0)
                        newAutoInterval--;
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

        // Save it (if changed)
        if(autoInterval != newAutoInterval) {
            autoInterval = newAutoInterval;
            saveBeepAutoInterval();
        }

        // End pause if current setting != off
        if(autoTimeIntervals[autoInterval]) 
            endPauseAuto();

        menuDelay(1000);

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
    int8_t oldBri = displaySet->getBrightness();
    int8_t number;
    bool briDone = false;
    bool blinkSwitch = false;
    unsigned long blinkNow = millis();
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    number = oldBri;

    // turn on all the segments
    allLampTest();

    // Display "LVL"
    displayBri(displaySet, number, false);

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !briDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            if(blinkSwitch) {
                displayBri(displaySet, number, false);
            }

            briDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!briDone) {

                if(dirDown) {
                    number++;
                    if(number > 15) number = wasEnter ? 0 : 15;
                } else {
                    if(number > 0) number--;
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

    allLampTest();  // turn on all the segments

    keypadMode = 0;

    if(!(briDone & (!wasQuit))) {
        displaySet->setBrightness(oldBri);
        return false;
    }

    return true;
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
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

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

    prepareForInput();

    sensNow = millis();

    // Don't use timeOut here, user might want to keep
    // sensor data displayed for a longer period.

    // Wait for enter
    while(!luxDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            luxDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!luxDone) {
                if(maxIdx > 0) {
                    if(wasEnter || !dirDown) {
                        numIdx++;
                        if(numIdx > maxIdx) numIdx = wasEnter ? 0 : maxIdx;
                    } else {
                        if(numIdx > 0) numIdx--;
                    }
                    dt_showTextDirect("WAIT");
                    pt_showTextDirect("");
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
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    #ifdef TC_HAVEMQTT
    maxMI = 3;
    #endif

    wifi_getMAC(macBuf);

    displayIP();

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !netDone) {

        scanKeypad();

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            netDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!netDone) {

                if(wasEnter || !dirDown) {
                    number++;
                    if(number > maxMI) number = wasEnter ? 0 : maxMI;
                } else {
                    if(number > 0) number--;
                }
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
                    #ifdef IS_ACAR_DISPLAY
                    dt_showTextDirect("MQTT");
                    #else
                    dt_showTextDirect("HOMEASSISTANT");
                    #endif
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
        pt_off();
        lt_off();
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
    bool wasEnter, dirDown, wasQuit = false, wasSelect;

    oldNumCli = numCli;

    dt_showTextDirect("");
    dt_on();
    displayClient(numCli, number);

    prepareForInput();

    // Wait for enter
    while(!checkTimeOut() && !netDone) {

        scanKeypad();

        if(oldNumCli != numCli) {
            number = 0;
            displayClient(numCli, number);
            oldNumCli = numCli;
        }

        if(waitForMenuControl(wasEnter, dirDown, wasQuit, wasSelect)) {

            if(wasQuit) break;

            resetTimeOut();  // button pressed, reset timeout

            netDone = (wasSelect || (wasEnter && menuWaitForRelease()));

            if(!netDone) {

                if(numCli > 1) {
                    if(wasEnter || !dirDown) {
                        number++;
                        if(number >= numCli) number = 0;
                    } else {
                        if(number > 0) number--;
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

/*
 * Install default audio files from SD to flash FS #############
 */

void doCopyAudioFiles()
{
    bool delIDfile = false;

    if((!copy_audio_files(delIDfile)) && !FlashROMode) {
        // If copy fails because of a write error, re-format flash FS
        lt_showTextDirect("FORMATTING");
        formatFlashFS(false);        // Format
        rewriteSecondarySettings();  // Re-write secondary settings
        #ifdef TC_DBG 
        Serial.println("Re-writing general settings");
        #endif
        write_settings();            // Re-write general settings
        copy_audio_files(delIDfile); // Retry copy
    }

    if(delIDfile) {
        delete_ID_file();
    } else {
        menuDelay(3000);
    }
    
    menuDelay(2000);

    allOff();
    dt_showTextDirect("REBOOTING");
    dt_on();

    unmount_fs();
    delay(500);

    esp_restart();
}

/* *** Helpers ################################################### */

void start_file_copy()
{
    mp_stop();
    stopAudio();
  
    dt_showTextDirect("INSTALLING");
    pt_showTextDirect("SOUND PACK");
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

void file_copy_done(int err)
{
    lt_showTextDirect(err ? "ERROR" : "DONE");
}

/*
 * Wait for ENTER to be released
 * Returns 
 * - true if ENTER was pressed long enough to be considered HELD
 * - false if ENTER released before considered "held"
 */
static bool menuWaitForRelease()
{
    while(checkEnterPress()) {
        menuLoops();
        if(isEnterKeyHeld) {
            isEnterKeyHeld = false;
            return true;
        }
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
void waitForEnterRelease()
{
    while(1) {
        while(digitalRead(ENTER_BUTTON_PIN)) {
            menuDelay(50);
        }
        menuDelay(50);
        if(!digitalRead(ENTER_BUTTON_PIN))
            break;
    }
    
    isEnterKeyPressed = false;
    isEnterKeyHeld = false;
}

static void waitAudioDoneMenu()
{
    unsigned long startNow = millis();

    while(!checkAudioDone() && (millis() - startNow < 3000)) {
        menuDelay(50);
    }
}

static bool waitForMenuControl(bool &wasEnter, bool& dirDown, bool& wasQuit, bool& wasSelect)
{
    wasEnter = wasQuit = wasSelect = false;
    
    if(checkEnterPress()) {
        wasEnter = dirDown = true;
        return true;
    } else if(keypadKeyPressed) {
        char c = keypadKeyPressed;
        keypadKeyPressed = 0;
        switch(c) {
        case '1':
        case '2':
            dirDown = true;
            return true;
        case '7':
        case '8':
            dirDown = false;
            return true;
        case '4':
        case '5':
            wasSelect = true;
            return true;
        case '9':
            wasQuit = true;
            return true;
        }
    }

    return false;
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
    enterkeyScan();
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

/*
 * Special case for very short, non-recurring delays
 * Only calls essential loops while waiting
 */
/* 
static void menuShortDelay(unsigned long mydel)
{
    unsigned long startNow = millis();

    audio_loop();

    while(millis() - startNow < mydel) {
        ntp_short_loop();
        audio_loop();
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
        speedoUpdate_loop(true);  // GPS part: 6-12ms without delay, 8-13ms with delay
        audio_loop();
        #endif
    }
}
*/
