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

/*
 * 
 * Needs ESP32 Arduino framework: https://github.com/espressif/arduino-esp32
 *
 * Library dependencies:
 * - OneButton: https://github.com/mathertel/OneButton
 *   (Tested with 2.0.4)
 * - ESP8266Audio: https://github.com/earlephilhower/ESP8266Audio
 *   (1.9.7 and later for esp-arduino 2.x; 1.9.5 for 1.x)
 * - WifiManager (tablatronix, tzapu; v0.16 and later) https://github.com/tzapu/WiFiManager
 *   (Tested with 2.1.12beta)
 * - Keypad ("by Community; Mark Stanley, Alexander Brevig): https://github.com/Chris--A/Keypad
 *   (Tested with 3.1.1)
 * 
 * 
 * Detailed installation and compilation instructions are here:
 * https://github.com/CircuitSetup/Time-Circuits-Display/wiki/9.-Programming-&-Upgrading-the-Firmware-(ESP32)
 */

/*  Changelog 
 *  2022/10/11 (A10001986)
 *    - IMPORTANT BUGFIX: Due to some (IMHO) compiler idiocy and my sloppyness, in 
 *      this case presenting itself in trusting Serial output instead of checking 
 *      the actual result of a function, the entire leap-year-detection was de-funct. 
 *      IOW: The clock didn't support leap years for time travels and some other
 *      functions.
 *    - New and now centralized logic to keep RTC within its supported time span 
 *      when we and our descendants cross over to 2100 or fun loving folks set their 
 *      RTC to years <1900 or >2099.
 *    - Throw out more unused code from DateTime class
 *    - Use Sakamoto's method for day-of-week determination
 *    - Clarification: The clock only supports the Gregorian Calendar, of which it
 *      pretends to have been used since year 1. The Julian Calendar is not taken
 *      into account. As a result, some years that, in the Julian Calendar, were leap 
 *      years between years 1 and 1582 in most of today's Europe, 1700 in DK/NO/NL
 *      (except Holland and Zeeland), 1752 in the British Empire, 1753 in Sweden, 
 *      1760 in Lorraine, 1872 in Japan, 1912 in China, 1915 in Bulgaria, 1917 in 
 *      the Ottoman Empire, 1918 in Russia and Serbia and 1923 in Greece, are 
 *      normal years in the Gregorian one. As a result, dates do not match in those 
 *      two calender systems, the Julian calendar is currently 13 days behind. 
 *      I wonder if Doc's TC took all this into account. Then again, he wanted to
 *      see Christ's birth on Dec 25, 0. Luckily, he didn't actually try to travel
 *      to that date. Assuming a negative roll-over, he might have ended up in
 *      eternity.
 *  2022/10/08 (A10001986)
 *    - Integrate cut-down version of RTCLib's DateTime to reduce bloat
 *    - Remove RTCLib dependency; add native RTC support for DS3231 and PCF2129 RTCs
 *  2022/10/06 (A10001986)
 *    - Add unit selector for temperature in Config Portal
 *  2022/10/05 (A10001986)
 *    - Important: The external time travel trigger button is now no longer
 *      on IO14, but IO27. This will require some soldering on existing TC
 *      boards, see https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *      Also, externally triggered time travels will now include the speedo
 *      sequence as part of the time travel sequence, if a speedo is
 *      connected and activated in the Config Portal. If, in the Config Portal,
 *      "Play complete time travel sequence" is unchecked, only the re-entry
 *      part will be played (as before).
 *    - Add trigger signal for future external props (IO14). If you don't
 *      have any compatible external props connected (which is likely since 
 *      those do not yet exist), please leave this disabled in the Config 
 *      Portal as it might delay the time travel sequence unnecessarily.
 *    - Activate support code for temperature sensor. This is for home setups
 *      with a speedo display connected; the speedo will show the ambient
 *      temperature as read from this sensor while idle.
 *    - [Add support for GPS for speed and time. This is yet inactive as it
 *      is partly untested.]
 *  2022/09/23 (A10001986)
 *    - Minor fixes
 *  2022/09/20 (A10001986)
 *    - Minor bug fixes (speedo)
 *    - [temperature sensor enhanced and fixed; inactive]
 *  2022/09/12 (A10001986)
 *    - Fix brightness logic if changed in menu, and night mode activated
 *      afterwards.
 *    - No longer call .save() on display when changing brightness in menu
 *    - [A10001986 wallclock customization: temperature sensor; inactive]
 *  2022/09/08-10 (A10001986)
 *    - Keypadi2c: Custom delay function to support uninterrupted audio during
 *      key scanning; i2c is now read three times in order to properly
 *      debounce.
 *    - Keypad/menu: Properly debounce enter key in menu (where we cannot
 *      use the keypad lib).
 *    - Menu: Volume setter now plays demo sound with delay (1s)
 *    - Added WiFi-off timers for power saving. After a configurable number of
 *      minutes, WiFi is powered-down. There are separate options for WiFi
 *      in station mode (ie when connected to a WiFi network), and in AP mode
 *      (ie when the device acts as an Access Point).
 *      The timer starts at power-up. To re-connect/re-enable the AP, hold
 *      '7' on the keypad for 2 seconds. WiFi will be re-enabled for the
 *      configured amount of minutes, then powered-down again.
 *      For NTP re-syncs, the firmware will automatically reconnect for a
 *      short period of time.
 *      Reduces power consumption by 0.2W.
 *    - Added CPU power save: The device reduces the CPU speed to 80MHz if
 *      idle and while WiFi is powered-down. (Reducing the CPU speed while
 *      WiFi is active leads to WiFi errors.)
 *      Reduces power consumption by 0.1W.
 *    - Added option to re-format the flash FS in case of FS corruption.
 *      In the Config Portal, write "FORMAT" into the audio file installer
 *      section instead of "COPY". This forces the flash FS to be formatted,
 *      all settings files to be rewritten and the audio files (re-)copied
 *      from SD. Prerequisite: A properly set-up SD card is in the SD card
 *      slot (ie one that contains all original files from the "data" folder
 *      of this repository)
 *  2022/09/07 (A10001986)
 *    - Speedo module re-written to make speedo type configurable at run-time
 *    - WiFi Menu: Params over Info
 *    - time: end autopause regardless of autoInt setting (avoid overflow)
 *  2022/09/5-6 (A10001986)
 *    - Fix TC settings corruption when changing WiFi settings
 *    - Format flash file system if mounting fails
 *    - Reduce WiFi transmit power in AP mode (to avoid power issues with volume pot if not at minimum)
 *    - Nightmode: Displays can be individually configured to be dimmed or switched off in night mode
 *    - Fix logic screw-up in autoTimes, changed intervals to 5, 10, 15, 30, 60.
 *    - More Config Portal beauty enhancements
 *    - Clockdisplay: Remove dependency on settings.h
 *    - Fix static ip parameter handling (make sure strings are 0-terminated)
 *    [- I2C-Speedo integration; still inactive]
 *  2022/08/31 (A10001986)
 *    - Add some tool tips to Config Portal
 *  2022/08/30 (A10001986)
 *    - Added delay for externally triggered time-travel, configurable in Config Portal
 *    - Added option to make ext. triggered time-travel long (or short as before)
 *    - Added some easter eggs
 *    - Fix compilation for LittleFS
 *    - Added a little gfx to Config Portal
 *  2022/08/29 (A10001986)
 *    - Auto-Night-Mode added
 *  2022/08/28 (A10001986)
 *    - Cancel enter-animation correctly when other anim is initiated
 *    - Shutdown sound for fake power off
 *    - Minor tweaks to long time travel display "disruption"
 *    - Timetravel un-interruptible by enter key
 *    - Re-do hack to show version on Config Portal
 *    - Add GPL-compliant (C) info
 *  2022/08/26 (A10001986)
 *    - Attempt to beautify the Config Portal by using checkboxes instead of
 *      text input for boolean options
 *  2022/08/25 (A10001986)
 *    - Add default sound file installer. This installer is for initial installation  
 *      or upgrade of the software. Put the contents of the data folder on a
 *      FAT formatted SD card, put this card in the slot, reboot the clock,
 *      and either go to the "INSTALL AUDIO FILES" menu item in the keypad menu,  
 *      or to the "Setup" page on the Config Portal (see bottom). 
 *      Note that this only installs the original default files. It is not meant 
 *      for custom audio files substituting the default files. Custom audio files 
 *      reside on the SD card and will be played back from there. 
 *  2022/08/24 (A10001986)
 *    - Intro beefed up with sound
 *    - Do not interrupt time travel by key presses
 *    - Clean up code for default settings
 *    - Prepare for LittleFS; switch not done yet. I prefer SPIFFS as long as it's
 *      around, and LittleFS doesn't support wear leveling.
 *    - AutoTimes sync'd with movies
 *  2022/08/23 (A10001986)
 *    - Allow a static IP (plus gateway, subnet mask, dns) to be configured.
 *      All four IP address must be valid. IP settings can be reset to DHCP
 *      by holding down ENTER during power-up until the white LED goes on.
 *    - F-ified most constant texts (pointless on ESP32, but standard)
 *  2022/08/22 (A10001986)
 *    - New long time travel sequence (only for keypad-timetravel, not for 
 *      externally triggered timetravel)
 *    - Hourly sound now respects the "RTC vs presentTime" setting for the alarm
 *    - Fix bug introduced in last update (crash when setting alarm)
 *    - Audio: Less logging; fix pot resolution for esp32 2.x; reduce "noise
 *      reduction" to 4 values to make knob react faster
 *    - Network info now functional in AP mode
 *    - Proper check for day validity in clockdisplay
 *  2022/08/21 (A10001986)
 *    - Added software volume: Volume can now be set by the volume knob, or by
 *      setting a value in the new keymap Volume menu.
 *    - Value check for settings entered on WiFi setup page
 *  2022/08/20 (A10001986)
 *    - Added a little intro display upon power on; not played at "fake" power on.
 *    - Added menu item to show software version
 *    - Fixed copy/paste error in WiFi menu display; add remaining WiFi stati.
 *    - Fixed compilation for A-Car display
 *    - Displays off during boot 
 *  2022/08/19 (A10001986)
 *    - Network keypad menu: Add WiFi status information
 *    - audio: disable mixer, might cause static after stopping sound playback
 *    - audio cleanup
 *    - clean up sound/animation delay definitions
 *    - audio: vol knob delivers inconsistent values, do some "noise reduction"
 *    - clean up clockdisplay, add generic text routine, scrap unused stuff
 *  2022/08/18 (A10001986)
 *    - Destination time/date can now be entered in mmddyyyy, mmddyyyyhhmm or hhmm
 *      format.
 *    - Sound file "hour.mp3" is played hourly on the hour, if the file exists on 
 *      the SD card; disabled in night mode
 *    - Holding "3" or "6" plays sound files "key3.mp3"/"key6.mp3" if these files
 *      exist on the SD card
 *    - Since audio mixing is a no-go for the time being, remove all unneccessary 
 *      code dealing with this.
 *    - Volume knob is now polled during play back, allowing changes while sound
 *      is playing
 *    - Fix auto time rotation pause logic at menu return
 *    - [Fix crash when saving settings before WiFi was connected (John)]
 *  2022/08/17 (A10001986)
 *    - Silence compiler warnings
 *    - Fix missing return value in loadAlarm
 *  2022/08/16 (A10001986)
 *    - Show "BATT" during booting if RTC battery is depleted and needs to be 
 *      changed
 *    - Pause autoInterval-cycling when user entered a valid destination time
 *      and/or initiated a time travel
 *  2022/08/15 (A10001986)
 *    - Time logic re-written. RTC now always keeps real actual present
 *      time, all fake times are calculated off the RTC time. 
 *      This makes the device independent of NTP; the RTC can be manually 
 *      set through the keypad menu ("RTC" is now displayed to remind the 
 *      user that he is actually setting the *real* time clock).
 *    - Alarm base can now be selected between RTC (ie actual present
 *      time, what is stored in the RTC), or "present time" (ie fake
 *      present time).
 *    - Fix fake power off if time rotation interval is non-zero
 *    - Correct some inconsistency in my assumptions on A-car display
 *      handling
 *  2022/08/13 (A10001986)
 *    - Changed "fake power" logic : This is no longer a "button" to  
 *      only power on, but a switch. The unit can now be "fake" powered 
 *      and "fake" powered off. 
 *    - External time travel trigger: Connect active-low button to
 *      io14 (see tc_global.h). Upon activation (press for 200ms), a time
 *       travel is triggered. Note that the device only simulates the 
 *      re-entry part of a time travel so the trigger should be timed 
 *      accordingly.
 *    - Fix millis() roll-over errors
 *    - All new sounds. The volume of the various sound effects has been
 *      normalized, and the sound quality has been digitally enhanced.
 *    - Make keypad more responsive
 *    - Fix garbled keypad sounds in menu
 *    - Fix timeout logic errors in menu
 *    - Make RTC usable for eternity (by means of yearOffs)
 *  2022/08/12 (A10001986)
 *    - A-Car display support enhanced (untested)
 *    - Added SD support. Audio files will be played from SD, if
 *      an SD is found. Files need to reside in the root folder of
 *      a FAT-formatted SD and be named 
 *      - "startup.mp3": The startup sound
 *      - "enter.mp3": The sound played when a date was entered
 *      - "baddate.mp3": If a bad date was entered
 *      - "timetravel.mp3": A time travel was triggered
 *      - "alarm.mp3": The alarm sound
 *      - "nmon.mp3": Night mode is enabled
 *      - "nmoff.mp3": Night mode is disabled*      
 *      - "alarmon.mp3": The alarm was enabled
 *      - "alarmoff.mp3": The alarm was disabled      
 *      Mp3 files with 128kpbs or below recommended. 
 *  2022/08/11 (A10001986)
 *    - Integrate a modified Keypad_I2C into the project in order 
 *      to fix the "ghost" key presses issue by reducing i2c traffic 
 *      and validating the port status data by reading the value
 *      twice.
 *  2022/08/10 (A10001986)
 *    - Added "fake power on" facility. Device will boot, setup 
 *      WiFi, sync time with NTP, but not start displays until
 *      an active-low button is pressed (connected to io13, see 
 *      tc_global.h)
 *  2022/08/10 (A10001986)
 *    - Nightmode now also reduced volume of sound (except alarm)
 *    - Fix autoInterval array size
 *    - Minor cleanups
 *  2022/08/09 (A10001986)
 *    - Fix "animation" (ie. month displayed a tad delayed)
 *    - Added night mode; enabled by holding "4", disabled by holding "5"
 *    - Fix for flakey i2c connection to RTC (check data and retry)
 *      Sometimes the RTC chip loses sync and no longer responds, this
 *      happens rarely and is still under investigation.
 *    - Quick alarm enable/disable: Hold "1" to enable, "2" to disable
 *    - If alarm is enabled, the dot in present time's minute field is lit
 *    - Selectable "persistent" time travel mode (WiFi Setup page):
 *        If enabled, time travel is persistent, which means all times
 *        changed during a time travel are saved to EEPROM, overwriting 
 *        user programmed times. In persistent mode, the fake present time  
 *        also continues to run during power loss, and is NOT reset to 
 *        actual present time upon restart.
 *        If disabled, user programmed times are never overwritten, and
 *        time travels are not persistent. Present time will be reset
 *        to actual present time after power loss.
 *    - Alarm data is now saved to file system, no longer to EEPROM
 *      (reduces wear on flash memory)
 *  2022/08/03-06 (A10001986)
 *    - Alarm function added
 *    - 24-hour mode added for non-Americans (though not authentic at all)
 *    - Keypad menu item to show IP address added
 *    - Configurable WiFi connection timeouts and retries
 *    - Keypad menu re-done
 *    - "Present time" is a clock (not stale) after time travel
 *    - Return from Time Travel (hold "9" for 2 seconds)
 *    - Support for time zones and automatic DST
 *    - More stable sound playback
 *    - various bug fixes
 */

#include <Arduino.h>
#include "clockdisplay.h"
#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_time.h"
#include "tc_wifi.h"
#include "tc_settings.h"

void setup() 
{
    Serial.begin(115200);    

    Wire.begin();
    // scan();
    Serial.println();
    
    time_boot();
    settings_setup();
    wifi_setup();
    audio_setup();

    menu_setup();
    keypad_setup();
    time_setup();
}


void loop() 
{
    keypad_loop();
    get_key();
    time_loop();
    wifi_loop();
    audio_loop();
}

// For testing I2C connections and addresses
/*
void scan() 
{
    Serial.println(" Scanning I2C Addresses");
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < 128; i++) {
        Wire.beginTransmission(i);
        uint8_t ec = Wire.endTransmission(true);
        if (ec == 0) {
            if (i < 16) Serial.print('0');
            Serial.print(i, HEX);
            cnt++;
        } else
            Serial.print("..");
        Serial.print(' ');
        if ((i & 0x0f) == 0x0f) Serial.println();
    }
    Serial.print("Scan Completed, ");
    Serial.print(cnt);
    Serial.println(" I2C Devices found.");
}

bool i2cReady(uint8_t adr) 
{
    uint32_t timeout = millis();
    bool ready = false;
    while ((millis() - timeout < 100) && (!ready)) {
        Wire.beginTransmission(adr);
        ready = (Wire.endTransmission() == 0);
    }
    return ready;
}
*/