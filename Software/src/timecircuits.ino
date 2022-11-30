/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us 
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
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
 * - ESP8266Audio: https://github.com/earlephilhower/ESP8266Audio
 *   (1.9.7 and later for esp-arduino 2.x; 1.9.5 for 1.x)
 * - WifiManager (tablatronix, tzapu; v0.16 and later) https://github.com/tzapu/WiFiManager
 *   (Tested with 2.1.12beta)
 * 
 * 
 * Detailed installation and compilation instructions are here:
 * https://github.com/CircuitSetup/Time-Circuits-Display/wiki/9.-Programming-&-Upgrading-the-Firmware-(ESP32)
 */

/*  Changelog
 *
  *  2022/11/22 (A10001986)
 *    - Audio: SPIFFS does not adhere to POSIX standards and returns a file object
 *      even if a file does not exist. Fix by work-around (SPIFFS only).
 *    - clockdisplay: lampTest(), as part of the display disruption sequence, might 
 *      be the reason for some red displays to go dark after time travel; reduce 
 *      the number of segments lit.
 *    - Rename "tempSensor" to "sensors" and add light sensor support. Three types
 *      are supported: TSL2561, BH1750, VEML7700/VEML6030 (VEML7700 only if no GPS 
 *      receiver is connected due to an i2c address conflict; VEML6030 must be configured
 *      for address 0x48, ie ADDR must be high, if GPS is connected at the same time). 
 *      The light sensor is solely used for switching the device into night mode. The 
 *      keypad menu as a new option to show the currently measured lux in order to 
 *      determine a suitable threshold to be configured in the Config Portal.
 *    - Display "--" on speedo instead of "NAN" if temperature cannot be read
 *    - Check more properly for GPS sensor (do a read test).
 *    - Main config file now (de)serialized using Dynamic JSON object due to its size
 *  2022/11/11 (A10001986)
 *    - Extended Sound on the Hour: User can now put "hour-xx.mp3" files for each
 *      hour on the SD card (hour-00.mp3, hour-01.mp3, ..., hour-23.mp3). If one of
 *      the files is missing, "hour.mp3" will be played (if it exists on the SD card)
 *    - Support comma on ADA-1270 7-segment display
 *  2022/11/10 (A10001986)
 *    - Minor optimizations (wifi)
 *    - Soft-reset the clock by entering 64738 and ENTER
 *  2022/11/08 (A10001986)
 *    - Allow time travel to (non-existing) year 0, so users can simulate the movie
 *      error (Dec 25, 0000).
 *    - RTC can no longer be set to a date below TCEPOCH (which is 2022 currently)
 *    - Adapt temperature sensor code to allow quickly adding other sensor types
 *    - Fix time travel time difference in case of a 9999->1 roll-over.
 *  2022/11/06 (A10001986)
 *    - Add short-cut to set alarm by typing 11hhmm+ENTER (weekday selection must
 *      still be done through keypad menu)
 *    - Change default time zone to UTC; folks without WiFi should not be bothered
 *      by unexpected DST changes.
 *    - Change default "time travel persistence" to off to avoid flash wear.
 *  2022/11/05 (A10001986)
 *    - Re-order keypad menu; alarm and volume are probably used more often than
 *      programming times for the displays or setting the RTC...
 *    - Add weekday selection for alarm
 *    - Add option to disable time travel sounds (in order to have other props play
 *      theirs. This is only useful in connection with compatible props, triggered by 
 *      IO14.)
 *    - Rename "tc_input.*" files to "input.*" for consistency reasons
 *  2022/11/03 (A10001986)
 *    - Reboot after audio file installation from keypad menu
 *    - Ignore held keys while in keypad menu
 *    - Optimize tc_input
 *  2022/11/02 (A10001986)
 *    - Re-order Config Portal options
 *    - Disable colon in night mode
 *    - Unify keypad and keypad_i2c into keypad_i2c; remove unused stuff, and further
 *      optimize code and improve key responsiveness and debouncing. The dependency 
 *      on Keypad is now history.
 *    - Include minimized version of OneButton library (renamed "TCButton"), thereby
 *      remove dependency on this library.
 *    - "tc_keypadi2c.*" renamed to tc_input.* to better reflect actual contents.
 *  2022/10/31 (A10001986)
 *    - Strengthen logic regarding when to try to resync time with NTP or GPS; if 
 *      there is no autoritative time, WiFi power saving timeout is set long enough 
 *      to avoid consecutive reconnects (and thereby frozen displays). This 
 *      essentially defeats the WiFi power saving feature, but accurate time is 
 *      more important than power saving. This is a clock after all.
 *    - Fix NTP update when WiFi is on power-save mode (ie off); WiFiManager's annoying 
 *      async WiFi scan when re-connecting and starting the Config Portal prohibited an 
 *      NTP update within the available time frame. Fixed by immediately re-triggering 
 *      an NTP request when an old packet timed out.
 *    - Do not start Config Portal when WiFi is reconnecting because of an NTP update.
 *      (This also fixes the issue above, but both measures are reasonable)
 *    - NTP: Give request packets a unique id in order to filter out outdated 
 *      responses; response packet timeout changed from 3 to 10 seconds.
 *  2022/10/29 (A10001986)
 *    - Added auto night-mode presets. There are currently four presets, for
 *      (hopefully) typical home, office and store setups. The times in the description
 *      define when the clock is in use, which might appear somewhat counter-intuitive 
 *      at first, but is easier to describe:
 *      Home: Mon-Thu 5pm-11pm, Fri 1pm-1am, Sat 9am-1am, Sun 9am-11pm
 *      Office (1): Mon-Fri 9am-5pm
 *      Office (2): Mon-Thu 7am-5pm, Fri 7am-2pm
 *      Shop: Mon-Wed 8am-8pm, Thu-Fri 8am-9pm, Sat 8am-5pm
 *      The old daily "start hour" and "end hour" setting still present, of course.
 *    - time_loop: Do not adjust time if DST flag is -1
 *  2022/10/27 (A10001986)
 *    - Minor fixes/enhancements (in time setup, etc)
 *  2022/10/26 (A10001986)
 *    - Enhancements to DST logic
 *    - Fine-tune GPS polling and RTC updating
 *    - Remove unused stuff
 *  2022/10/24 (A10001986)
 *    - Defer starting the Config Portal during boot: Starting with 2.0.13beta,
 *      WiFiManager triggers an async WiFi Scan when the CP is started, which 
 *      interferes with our NTP traffic during the boot process. Start CP after NTP 
 *      is done.
 *  2022/10/14-23 (A10001986)
 *    - Fix time travel with speedo: Added forgotten code after re-write.
 *    - Added support for MTK3333 based GPS receivers connected through i2c. These
 *      can act as a source of authoritative time (just like NTP), as well as for
 *      calculating current speed, to be displayed on a speedo display (which only
 *      really makes sense in a car/boat/whatevermoves).
 *    - Due to the system's time library's inability to handle some time zones
 *      correctly (eg Lord Howe and Iran) and its unawareness of the well-known facts  
 *      that NTP will roll-over in 2036 and unix time in 2038 (for which the system 
 *      as it stands is not prepared and will fail), all ties with time library were 
 *      cut, and a completely native time system was implemented, handling NTP/GPS, 
 *      time zones and DST calculation all by itself.
 *      This allows using this firmware with correct clocking until the year 9999 
 *      (at which point it rolls over to 1), with NTP support until around
 *      2150. Also, DST is now automatically switched in stand-alone mode
 *      (ie without NTP or GPS).
 *      For all this to work, it is essential that the user configures his time
 *      zone in the Config Portal, and, for GPS and stand-alone, adjusts the TC's
 *      RTC (real time clock) to current local time using the keypad menu.
 *    - General cleanup (declarations, header files, etc)
 *  2022/10/13 (A10001986)
 *    - Implement own DST management (TZ string parsing, DST time adjustments)
 *      This system is only active, if no authoritative time source is available.
 *      As long as NTP or GPS deliver time, we rely on their assessments.
 *    - Clean up declarations & definitions all over
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

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>

#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_settings.h"
#include "tc_time.h"
#include "tc_wifi.h"

void setup() 
{
    powerupMillis = millis();

    Serial.begin(115200);    

    // PCF8574 only supports 100kHz, can't go to 400 here - Wire.begin(-1, -1, 100000);
    Wire.begin();

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
    scanKeypad();
    ntp_loop();
    time_loop();
    audio_loop();
    wifi_loop();
}