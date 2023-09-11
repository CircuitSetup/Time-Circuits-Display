/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
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
 * 
 * Needs ESP32 Arduino framework: https://github.com/espressif/arduino-esp32
 *
 * Library dependencies:
 * - ESP8266Audio: https://github.com/earlephilhower/ESP8266Audio
 *   (1.9.7 and later for esp-arduino 2.x; 1.9.5 for 1.x)
 * - WifiManager (tablatronix, tzapu) https://github.com/tzapu/WiFiManager
 *   (Tested with 2.0.13beta and 2.0.15-rc1)
 * - ArduinoJSON >= 6.19: https://arduinojson.org/v6/doc/installation/
 * 
 * 
 * Detailed installation and compilation instructions are here:
 * https://github.com/CircuitSetup/Time-Circuits-Display/wiki/9.-Programming-&-Upgrading-the-Firmware-(ESP32)
 */

/*  Changelog
 *
 *  2023/08/14 (A10001986)
 *    - (Remove historic and pointless casts for atoi)
 *  2023/08/04 (A10001986)
 *    - Minor cleanup
 *  2023/08/03 (A10001986)
 *    - Fix "animation" and version string on A-Car displays
 *    - Add compile time options (tc_global.h) for
 *      - showing BTTF3-like animation when entering a dest time
 *        (and only then; also not when RC mode is active)
 *      - reversing AM and PM, like seen in some BTTF2/3 scenes
 *  2023/07/31 (A10001986)
 *    - Don't show "REPLACE BATT" on uninitialized clocks
 *    - Remove old unneeded stuff
 *  2023/07/25 (A10001986)
 *    - Loop for adding WiFi parameters
 *  2023/07/24 (A10001986)
 *    - Truncate beep sound data (for bin size)
 *    - Reduce/shrink debug output (for bin size)
 *  2023/07/22 (A10001986)
 *    - BTTFN: Add client device type; let others query IP of given type
 *    - Do some minor binary size crunching
 *  2023/07/20 (A10001986)
 *    - Make "beep" ever so slightly shorter
 *    - Hack to display "0" on CircuitSetup's speedo's fake third digit
 *      (Enabled by compile-time-option, see tc_global.h)
 *  2023/07/17 (A10001986)
 *    - Try to make keypad sounds more immediate
 *  2023/07/11 (A10001986)
 *    - Clean up
 *  2023/07/09 (A10001986)
 *    - BTTFN: Provide some status info to BTTFN clients
 *    - Config Portal: Rename some options for simplification/briefness
 *    - Internal flag-cleanup/fix
 *  2023/07/07 (A10001986)
 *    - Block WiFi power-saving as long as BTTFN clients are present
 *    - GPS speed: Increase update rate to twice per second for smoother speedo display.
 *      Also, add CP option "Quick GPS updates" for peripherals that poll the TCD for 
 *      speed (such as SID). If neither this nor the option "Display GPS speed" is set, 
 *      GPS rate is every 5 seconds; if either is set, twice per second.
 *    - Extend mere "network polling" into "BTTF network" ("BTTFN"): TCD now not only 
 *      answers to polling requests (time, temp, lux, speed), but also sends notifications
 *      to known clients about time timetravel and alarm. Those notifications are only
 *      sent, if MQTT is disabled or "send commands for other props" is unchecked.
 *    - BTTFN: Show connected clients in keypad menu (hostname, IP). This helps finding 
 *      out the IP address of other props when they are connected to the TCD's AP.
 *  2023/06/27 (A10001986)
 *    - Fix isIp()
 *  2023/06/26 (A10001986)
 *    - Network polling: Add response marker
 *  2023/06/23 (A10001986)
 *    - Add network data polling facility (time, speed, temperature), in preparation for
 *      other bttf props
 *  2023/06/19 (A10001986)
 *    - Fake power: Turn on keypad LEDs in sync with displays
 *    - Re-do beep sound (better sync'd under ESP32-Arduino 2.0.9)
 *    - Display "updating" and stop audio upon OTA fw update
 *  2023/06/12 (A10001986)
 *    - Don't show audio installer if in FlashROMode
 *  2023/06/06 (A10001986)
 *    - Avoid sound distortion on newly initialized clocks by saving lastYear in setup(),
 *      and skip NTP/GPS update during startup sequence. (Writing to flash is slow enough
 *      to cause sound disturbances)
 *    - Reduce read access to NVM by caching
 *  2023/06/02 (A10001986)
 *    - Display Title & Artist from ID3 tags when typing 55ENTER to display the 
 *      currently played song. (ID3v2 only)
 *  2023/05/30 (A10001986)
 *    - MusicPlayer: 55ENTER shows currently played song (changed this from 88 to avoid
 *      going to song 0 by accident)
 *    - MusicPlayer: Add "auto-renamer". User can now put his mp3-files with original
 *      names (eg "Dire Straits - Money For Nothing.mp3") in the musicX folders on the 
 *      SD and they will be automatically renamed to "000.mp3" and so on. File names are 
 *      sorted alphabetically before renaming (in order to keep some kind of order; 
 *      without sorting, the order would be more or less random).
 *      This also works if files are added to a music folder that already contains some
 *      files with numerical file names.
 *      The process can take up to 10 minutes for 1000 files, perhaps even longer. Mac
 *      users are advised to delete the "._"-files before putting the SD back into the
 *      TCD, as removing these files speeds up the process.
 *      Remember: 128kbps maximum bit rate. Recoding from higher bit rates is still up
 *      to the user. Tools to do this batch-wise are plenty, for Mac and Windows MediaHuman 
 *      Audio Converter (https://www.mediahuman.com/download3.html), for example.
 *    * Switched to esp32-arduino 2.0.9 for pre-compiled binary. If you compile and upload 
 *      using the IDE, settings will be lost and audio files will have to be re-installed.
 *      If upgrading via the Config Portal's "Update" feature, everything stays. 
 *    * Updated WiFiManager to 2.0.16rc2 for pre-compiled binary.
 *    * The source code now includes a subset of the ESP8266Audio library version 1.9.5
 *      with some patches. The reason for this is that ESP8266Audio 1.9.7 is somehow 
 *      broken (delays playback for some reason, spits out i2s errors) and I don't know 
 *      what the future holds for this library. I need a stable basis with predictable 
 *      behavior.
 *  2023/05/23 (A10001986)
 *    - Keypad: 440ENTER deletes timer (4400 still does, too; added for uniformity)
 *  2023/05/22 (A10001986)
 *    - 77mmdd sets reminder to month/day, leaving time unchanged (unless hr and min are 
 *      zero, in which case it sets the reminder to 9am)  
 *    - MusicPlayer: 88 shows currently played song
 *    - 11, 44, 77: If alarm/timer/reminder is unset/off, play regular enter sound, not
 *      the "error" one
 *    - If Music Player is active, show "ERROR" for bad/invalid input (since the player
 *      should not be interrupted) (Exception: Programming a Destination Time)
 *  2023/05/21 (A10001986)
 *    - Minor optimizations
 *  2023/05/20 (A10001986)
 *    - Fix beep modes 2 and 3 with time travel
 *  2023/05/19 (A10001986)
 *    - More internal optimizations (reference DateTime instead of copying it over 
 *      function calls; don't read RTC too often, update dt instead, etc)
 *  2023/05/18 (A10001986)
 *    - Internal optimizations (data entry doesn't show leading 0; blink logic; Audio, 
 *      ClockDisplay: Get rid of bools, use flags; etc)
 *  2023/05/17 (A10001986)
 *    - Add yearly/monthly reminder: Type 77mmddhhMM to set a timer that will play a 
 *      sound yearly on given date, or, if the month is 00, every month on given day. 
 *      77 displays current reminder, 770 deletes it, 777 displays the days/hours/mins 
 *      until the next reminder.
 *      Requires new sound-pack. Reminder always based on real local time.
 *    - Night mode no longer adheres to "Alarm base is real present time" setting. It 
 *      is ALWAYS based on real present time now.
 *    - Display colon in special displays where hours and minutes are used as such.
 *    - Fix ee1-regression
 *  2023/05/15 (A10001986)
 *    - Allow both world clock and room condition mode at the same time.
 *      If both on, only temp is shown, either in red display (if no TZ for red display 
 *      was configured) or in yellow display (regardless of a TZ for that display).
 *      113+ENTER toggles both RC/WC-mode (synchronously).
 *  2023/05/13 (A10001986)
 *    - MQTT: Increase reconnect-attempt-interval over time
 *  2023/05/12 (A10001986)
 *    - Music Player: Fix going to song# when player is off
 *    - MQTT: Add async ping to server before trying to connect. This avoids
 *      "frozen" displays and audio interruptions but requires that the server
 *      actually answers to ping (ICMP) requests.
 *  2023/05/11 (A10001986)
 *    - MQTT: Make (re)connection/subscription async on MQTT protocol level
 *    - MQTT: Limit re-connection attempts.
 *  2023/05/10 (A10001986)
 *    - Detect "APSTA" WiFi Mode correctly for Keypad Menu
 *    - Optimize menu code; add "blinking" of editable data
 *    - Minor optimizations for TZ parsing and DST checks
 *  2023/05/05 (A10001986)
 *    - Publish "ALARM" via MQTT
 *    - Fix compilation if GPS is to be left out
 *  2023/05/04 (A10001986)
 *    - Ignore MQTT messages with a length of zero; fix long string handling
 *  2023/05/03 (A10001986)
 *    - Add missing characters to 7-seg font
 *    - JS optimization for CP
 *    - Fix MQTT message scrolling
 *  2023/05/02 (A10001986)
 *    - [Pre-compiled binary: Patch WiFiManager::HTTPSend (avoid duplication of String)]
 *    - HA/MQTT: Publish "REENTRY" for external props; fix error in topic scanning;
 *      subscribe two topics at a time.
 *    - time_loop(): Move less timing critical stuff to when there is no half-
 *      second switch.
 *  2023/05/01 (A10001986)
 *    - HA/MQTT changes: More commands supported; commands can be in lower case; 
 *      no commands while keypad menu or sequences are active;
 *    - Using MQTT now disables WiFi power save
 *    - Disable modem sleep to avoid delays in CP and MQTT
 *  2023/04/29 (A10001986)
 *    - BETA: Add HomeAssitant/MQTT 3.1.1 support. MTQQ code by Nicholas O'Leary; adapted,
 *      optimized and minimized by me. Only unencrypted traffic, no TLS/SSL support.
 *      Used in three ways:
 *      1) User can send messages to configurable subscribed topic, which are displayed 
 *      on Destination Time display. Only ASCII text messages supported, no UTF8 or 
 *      binary payload. If the SD card contains "ha-alert.mp3", it will be played upon
 *      reception of a message.
 *      2) User can send commands to TCD (topic "bttf/tcd/cmd"), for example TIMETRAVEL
 *      or RETURN (as in "return from time travel").
 *      3) TCD can trigger time travel via MTQQ (topic "bttf/tcd/pub"). This works like
 *      the "external time travel" for wired props.
 *      Broker can be configured using IP or domain, optionally with :xxxx for port
 *      number ("192.168.3.5:1234"); the default port is 1883.  User and password are 
 *      optional; format is "user" (for no password) or "user:pass".
 *      Configuring a WiFi power save timeout disables HA/MQTT.
 *      Broker is strongly recommended to be in same local network; network delays
 *      can cause sound and other timing issues.
 *    - Add "homeassitant" menu item to keypad "network" menu to view connection
 *      status.
 *    - Heavily optimized Config Portal. It grew to big for the available memory, so
 *      some options had to be removed. Also, the nice title graphics had to go, it's 
 *      now the Twin Pine Mall all the way.
 *  2023/04/25 (A10001986)
 *    - Minor optimizations (keypad, gps)
 *  2023/04/23 (A10001986)
 *    - Further simplify keypad code (and fix [unlikely] problem with several keys
 *      pressed at the exact same time)
 *  2023/04/22 (A10001986)
 *    - Simplify keypad handling
 *    - Global license is now MIT (with exception). Possibly GPL-infe[c|s]ted parts
 *      all finally rewritten/removed.
 *  2023/04/21 (A10001986)
 *    - Shrink the header gfx in CP to reduce page data size. WiFiManager builds the
 *      page blob by concatenating Strings, lots of Strings. The big header was one
 *      of the first Strings (hence "header"), which apparently lead to massive heap
 *      fragmentation when repeatedly appending other Strings to it, and as a result, 
 *      sometimes to a blank page in the browser, assumingly caused by a memory issue 
 *      somewhere down the line.
 *  2023/04/20 (A10001986)
 *    - Add "city" (or location name) for World Clock time zones; will be displayed
 *      for 3 seconds every 10 seconds. Keep empty to have the time displayed the
 *      whole time (like before).
 *    - Add tiny delay between BMx820 sensor read-outs
 *  2023/04/18 (A10001986)
 *    - Properly respect beepMode 1
 *    - Better "beep" (less hissing)
 *  2023/04/17 (A10001986)
 *    - Remove deprecated EEPROM stuff
 *  2023/04/16 (A10001986)
 *    - Better beep mode change feedback
 *    - Randomize lamptest() for time travel
 *  2023/04/15 (A10001986)
 *    - Properly restore dest/dep times on quitting the menu
 *  2023/04/13 (A10001986)
 *    - Add beep modes: 000 disables beep, 001 enables beep, 002 and 003 enable beep
 *      for 30/60 seconds after entering a destination time and/or upon initiating a 
 *      time travel. If a time travel is triggered by an external button for which a 
 *      delay is configured, the beep starts when pressing the button and ends 30/60
 *      seconds after the re-entry.
 *  2023/04/12 (A10001986)
 *    - Make "periodic reconnection attempts" a config option; might be undesired in
 *      car setups.
 *    - Don't reconnect WiFi for NTP if mp3 is played back or user has used the keypad
 *      within the last 2 minutes.
 *  2023/04/11 (A10001986)
 *    - Add World Clock mode. User can now configure a separate time zone for each the 
 *      red and yellow displays. 112+ENTER toggles World Clock (WC) mode. Red/yellow 
 *      displays will show the time in the configured time zones. Entering a destination 
 *      time, time travel and some other actions disable WC mode.
 *  2023/04/09 (A10001986)
 *    - Revisit WiFi reconnection logic: Support case where WiFi network was inaccessible
 *      during power-up. See comments tc_time.cpp for details.
 *    - Add keypad menu item "TIME SYNC", shows when last time sync (NTP/GPS) was done.
 *  2023/04/06 (A10001986) [CS 2.7 Release]
 *    - Audio: Re-do beep; remove all traces of (obsolete) MIXER; Short fx are now 
 *      played without re-scanning the analog input during play-back. Reason: Pot 
 *      tolerance led to audible "distortions" with very short sounds.
 *  2023/04/05 (A10001986)
 *    - WiFi: Reconnect for NTP also if previously in AP-mode, if a WiFi network 
 *      is configured to connect to. This helps for unstable WiFi networks (or ones 
 *      which are disabled in a schedule), if the power-saving timers are non-zero. 
 *      The TCD might be unable to (re)connect to a previously configured network when 
 *      attempting to sync time via NTP and end up in AP mode. However, after the power-
 *      saving timers expire, an new connection attempt might succeed.
 *      So, if your WiFi network is somewhat unstable, set the WiFi power save timers
 *      (both for station as well as AP mode) so non-zero; this causes re-connects for
 *      NTP allowing time synchonization even if some of those reconnecting attempts
 *      fail (because the WiFi network is not found at some attempts).
 *    - Reset max tx power when connecting to WiFi network (might have been reduced
 *      when falling-back to AP mode earlier)
 *    Note that some WiFi activity (key exchange? Re-scan?) might cause distortions 
 *    in mp3 play back. I experienced this when connecting the TCD to a "personal 
 *    hotspot" on an iPhone; every 2 or 3 minutes exactly, there is a small distortion.
 *    Does not happen when the TCD is connected to my home network.
 *  2023/04/04 (A10001986)
 *    - NTP: Do not overwrite previous packet age in case of a bad(outdated) new packet
 *  2023/04/01 (A10001986)
 *    - Fix beep: Off in nightmode and when unit is fake-powered off
 *    - Fix CB1.3 LED logic: On only if unit is NOT fake powered off, and NOT
 *      in night mode.
 *    - Add beep option to setup page
 *  2023/03/28 (A10001986)
 *    - Add option to keep temperature display on (and dimmed) in night mode
 *  2023/03/27 (A10001986)
 *    - Add (annoying) beep sound. Enabled/disabled by 000+ENTER. Disabled by default.
 *      Requires new sound-pack.
 *  2023/03/16 (A10001986)
 *    - Fixes for CircuitSetup's speedo display
 *  2023/03/05 (A10001986)
 *    - External time travel button: When held for 3 seconds, a "return from time
 *      travel" is triggered. This brings a change in external time travel triggering:
 *      Up until now, a time travel was issued when the button was pressed for 200ms, 
 *      regardless of when it was released. Now, the tt will be triggered upon release 
 *      of the button, as long it is released in under 3000ms. If a delay is 
 *      configured, it, too, will start running upon release.
 *  2023/02/21 (A10001986)
 *    - Prepare for TCD CB 1.3 with switchable LEDs. LEDs are off when fake power
 *      is off, and in night mode.
 *  2023/01/28 (A10001986)
 *    - PCF2129 RTC: Fix obvious copy/paste error; add OTP refresh
 *  2023/01/26 (A10001986)
 *    - GPS: Code optimizations; quicker time-sync if GPS has valid time
 *  2023/01/25 (A10001986)
 *    - GPS speed: Fix tt sequence/sound timing when counting up from GPS speed
 *    - GPS speed: Soften count-down from 88 back to current GPS speed
 *    - Turn speedo off when rebooting
 *  2023/01/23 (A10001986)
 *    - GPS speed: Reduce delay between reading speed and displaying it
 *    - GPS time fix: Fractions are of variable size, not always 3 digits
 *  2023/01/22 (A10001986)
 *    - Minor fix (reset flags after audio file installation; better question mark)
 *  2023/01/21 (A10001986)
 *    - Audio installer change: The installer is now invoked automatically during
 *      boot. If the user cancels, the installer is still available as the first menu 
 *      item in the keypad menu. It was, however, removed from the Config Portal.
 *      The installer now removes the ID file after successfully copying the files
 *      so that it is not shown again on reboot.
 *    - Show hint to install audio files during boot if files not present on flash
 *      FS
 *    - Return from tt: Don't stop music player if we were on actual present time
 *      already.
 *  2023/01/20 (A10001986)
 *    - Add count-down timer function: To start, type 44xxENTER, xx being the number
 *      of minutes (2 digits). Type 44ENTER to see the time remaining.
 *    - Update for sound-pack: Add "timer.mp3" sound, changed "alarm.mp3" sound.
 *  2023/01/19 (A10001986)
 *    - CP: All sub-pages are now uniform (redo logo on Update-page)
 *  2023/01/18 (A10001986)
 *    - Invalidate EEPROM contents after migrating to flash FS
 *    - Patch the living daylights out of the CP's WiFi Config Page (new JS with 
 *      corrected logic for Password-placeholder, uniform look, etc)
 *    - Fix regression on WiFi Config page (clicking on SSID-Link)
 *  2023/01/16-17 (A10001986)
 *    - Use files for clock states instead of the EEPROM API
 *    - Some more CP eye-candy, plus an easter egg
 *  2023/01/15 (A10001986)
 *    - More style for the Config Portal
 *  2023/01/14 (A10001986)
 *    - Another NM logic fix (when forced if min == 0)
 *  2023/01/13 (A10001986)
 *    - Minor fixes and enhancement (settings, alarm, etc)
 *  2023/01/12 (A10001986)
 *    - Disable NM and stop music player before copying audio files
 *    - More NM logic fixes
 *  2023/01/11 (A10001986)
 *    - Fix minor logic error in loadCurVolume()
 *    - Really disable night-mode when entering keypad menu
 *    - Fix LR303/329 light sensor lux calculation; sensor now officially supported
 *  2023/01/10 (A10001986)
 *    - Simplify debug and other output (printf)
 *    - GPS: Check NMEA length before other tests
 *  2023/01/09 (A10001986)
 *    - Settings: Add some more checks for validity, code simplifications
 *    - Clockdisplay: Make loadLastYear() return impossible values on error to force 
 *      comparisons to fail
 *    - [Prepare Flash-RO (read-only) mode]
 *  2023/01/07 (A10001986)
 *    - Change "Time-rotation" to "Time-cycling" in CP
 *    - Re-order keypad menu (SET RTC before other displays)
 *    - Replace double by float almost everywhere, precision not needed
 *  2023/01/06 (A10001986)
 *    - Flash wear awareness week continues: Add option to save alarm and volume
 *      settings on SD card. Folks who often change their alarm or volume settings
 *      are advised to check/enable this.
 *      Music folder number was removed from Config Portal, and is now always saved 
 *      on the SD card.
 *    - Code optimizations (mainly settings and rtc)
 *  2023/01/05 (A10001986)
 *    - Keypad menu: Save settings only if they were changed to reduce flash wear.
 *    - Fix "8" being written to typing buffer despite being held
 *    - Fix restoring volume in case of timeout in keypad menu
 *  2023/01/04 (A10001986)
 *    - Minor enhancements for Keypad menu (Music folder number: Show info if /musicX 
 *      or /musicX/000.mp3 is not found on SD; Alarm: Blink on/off to indicate which
 *      field is to be edited; etc); code simplifications.
 *  2023/01/02 (A10001986)
 *    - Night-mode: Treat daily schedule like other schedules with regard to hourly
 *      checks; this fixes returning to nm after manual override.
 *    - Add 11+ENTER short-cut to quickly show alarm settings
 *    - Alarm times are ALWAYS shown in 24-hour mode, regardless of Config Portal
 *      24hr setting.
 *  2023/01/01 (A10001986)
 *    - Have a go at wobbling volume, again: New "noise reduction" from apparently
 *      totally unstable analog input; knob now always mutes when turned fully off; 
 *      new fixed-level table with better granularity at lower volumes, also it now
 *      has 20 levels.
 *    - Audio: Add mp3 music player. Music files must reside on the SD card in /musicX 
 *      (X being 0-9) folder(s), be named 000.mp3 up to 999.mp3 (starting at 000.mp3 
 *      in each folder), and be of max 128kpbs. Up to ten folders (music0-music9) are 
 *      supported; the folder number is selected in the Config Portal or the keypad 
 *      menu (default: 0, hence /music0).
 *      Keypad layout changed as follows:
 *      Holding 1 now toggles the alarm on/off (formerly: Switch alarm on)
 *      Holding 4 now toggles night-mode on/off (formerly: Switch nm on)
 *      Holding 2 goes to previous song (formerly: Switch alarm off)
 *      Holding 5 starts/stops the mp3 player (formerly: Switch nm off)
 *      Holding 8 goes to next song
 *      222+ENTER    disables shuffle (played in order 000->999)
 *      555+ENTER    enables shuffle (played in random order)
 *      888+ENTER    goes to song 000
 *      888xxx+ENTER goes to song xxx
 *    - Audio: Skip ID3 tags before starting playback. This fixes a freeze when
 *      attempting to play mp3 files with large tags, eg cover art.
 *    - Increase SD/SPI frequency to 16Mhz (with option to reduce back to 4Mhz in
 *      Config Portal)
 *    - Light sensor: Allowed lux level range is now 0-50000
 *  2022/12/26 (A10001986)
 *    - Fix reading BME280 sensor (burst read all data registers for consistency)
 *    - Disable WiFi-power-save when user initiates firmware update
 *    - Display "wait" if Flash-FS is being formatted upon boot
 *    - Add support for LTR3xx light sensor [untested]
 *    - [All supported temperature sensors verified working]
 *  2022/12/19 (A10001986)
 *    - Changed read logic for Si7021 and SHT4x; fix typo in TMP117 code path
 *    - Run MCP9808 in higher resolution mode, scrap sensor shut-down
 *    - Restrict allowed chars in NTP server and hostname fields in Config Portal
 *    - Updated WiFiManager to 2.0.15-rc1 in pre-compiled binary
 *  2022/12/18 (A10001986)
 *    - Audio files installer in keypad menu: If copy fails, re-format flash FS,
 *      re-write settings, and retry copy. (Same can be done by writing "FORMAT"
 *      instead of "COPY" in Config Portal)
 *  2022/12/17 (A10001986)
 *    - "Animate" room condition mode display (imitate delayed month)
 *    - Sensors keypad menu: Read sensors only every 3rd second to avoid self-
 *      heating
 *    - Add mDNS support: Access the Config Portal via http://timecircuits.local (the
 *      hostname is configurable in the Config Portal)
 *    - Disable WiFiManager's "capitive portal" in order to avoid server-redirects to 
 *      IP address.
 *  2022/12/16 (A10001986)
 *    - Add support for SI7021, SHT40, TMP117, AHT20/AM2315C, HTU31D temperature sensors 
 *    - Add reading humidity from BME280, SI7021, SHT40, AHT20, HTU31D sensors 
 *    - Add "room condition" mode, where destination and departed time are replaced
 *      by temperature and humidity, respectively. Toggle normal and rc mode by entering
 *      "111" and pressing ENTER. Sensor values are updated every 30 seconds in this
 *      mode (otherwise every 2 minutes).
 *    - Add humidity display to SENSORS keypad menu
 *    - Add MAC address display to "Network" keypad menu. Only the "station mode" MAC
 *      is shown, ie the MAC address of the ESP32 when it is connected to a WiFi
 *      network. For some reason there are up to four MAC addresses, used in different
 *      network modes (AP, STA, etc). The digit "6" as part of the MAC is shown using
 *      the "modern"/common segment pattern here to distinguish it from "b".
 *    - Fix formatting bug in tc_font.h leading to font missing one character
 *  2022/12/02 (A10001986)
 *    - Add support for BMx820 sensor (temperature only).
 *    - Modify former "light sensor" keypad menu to not only show measured lux level
 *      from a light sensor, but also current ambient temperature as measured by
 *      a connected temperature sensor. Rename menu to "Sensors" accordingly.
 *    - Add temperature offset value. User can program an offset that will be added
 *      to the measured temperature for compensating sensor inaccuracies or suboptimal
 *      sensor placement. In order to calibrate the offset, use the keypad menu 
 *      "SENSORS" since the temperature shown there is not rounded (unlike what is
 *      shown on a speedo display if it has less than three digits).
 *  2022/11/22 (A10001986) [2.4]
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

    // PCF8574 only supports 100kHz, can't go to 400 here.
    Wire.begin(-1, -1, 100000);
    //Wire.begin();
    
    Serial.println();
    
    time_boot();
    settings_setup();
    wifi_setup();
    audio_setup();
    keypad_setup();
    time_setup();
}


void loop() 
{
    keypad_loop();
    audio_loop();
    scanKeypad();
    ntp_loop();
    audio_loop();
    time_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    #ifdef TC_HAVEBTTFN
    bttfn_loop();
    #endif
}