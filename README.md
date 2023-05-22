# Time Circuits Display

| ![TCD Front](https://user-images.githubusercontent.com/76924199/200327688-cfa7b1c2-abbd-464d-be6d-5d295e51056e.jpg) |
|:--:|
| *This very one is now in the Universal Studios [BTTF Escape Room](https://www.universalorlando.com/web/en/us/things-to-do/entertainment/universals-great-movie-escape) (Orlando)* |


The Time Circuits Display has been meticulously reproduced to be as accurate as possible to the one seen in the Delorean Time Machine in the Back to the Future movies. The LED displays are custom made to the correct size for CircuitSetup. This includes the month 14 segment/3 character displays being closer together, and both the 7 & 14 segment displays being 0.6" high by 0.35" wide.

## Kits
[Time Circuits Display kits can be purchased here with or without 3d printed parts or aluminum enclosures.](https://circuitsetup.us/product/complete-time-circuits-display-kit/)

[View the instructions for assembling your CircuitSetup.us TCD Kit](https://github.com/CircuitSetup/Time-Circuits-Display/wiki)

### Kit Parts
- 3x Segment displays in (Red, Green, Yellow)
- ESP32 powered control board
- TRW style number keypad (must be disassembled to use number keys, pcb and rubber backing)
- Gels for displays (Red, Green, Yellow)
- Mounting hardware (37x screws for display enclosure to outer enclosure, display PCB to enclosures, speaker mount, keypad mount, 4x small screws for keypad PCB to bezel mount)
- Connection wiring (3x 4p JST-XH for display I2C, 1 7p JST-XH for keypad)
- Square keypad LED lenses (2x white, 1x red/amber/green)
- Small 4ohm 3W speaker
- Display stickers
- 3d printed enclosures and keypad housing OR Aluminum enclosures (optional)

## Features
#### Complete Time Circuits functionality with movie sound effects
- Enter any date via keypad
- Time travel to the Destination Time with display effects
- Return from time travel
- Automatic rotation of times displayed in the movie (optional rotation intervals, or off)
#### Present Time as a clock
- On-board Real Time Clock (RTC)
- Get accurate time from NTP (network time) when connected to a WiFi network
- Support for time zones and automatic DST
- Alarm function (sounds customizable with a SD card)
- Play a sound every hour (optional)
#### Support for external power on/off & other prop control
- Trigger an external time travel event on other props
- Trigger a time travel event (on the TCD) from another prop
- Fake power on & off - TCD will boot, setup WiFi, sync time with NTP, but not start/turn off displays until an active-low button is pressed (useful for turning on/off the displays with a TFC drive switch)
#### Other features
- Settings interface web portal
- Keypad menu for adjusting various settings, viewing IP address, and WiFi status
- Configurable WiFi connection timeouts and retries
- Set a static IP
- Optional power-up intro with sound
- SD card support for custom audio files
- 24-hour clock mode for non-Americans
- Night mode (displays off or dimmed) & ability to automatically turn on/off by Present Time
- Option to make time travels persistent over reboots (dates entered stay there instead of rotating)
- Built-in installer for default audio files in addition to OTA firmware updates


## Short summary of first steps

The first step is to establish access to the Config Portal in order to configure your clock.

As long as the device is unconfigured, as is the case with a brand new clock, or later if it for some reason fails to connect to a configured WiFi network, it starts in "access point" mode, i.e. it creates a WiFi network of its own named "TCD-AP".

- Power up the clock and wait until it shows a time (which is probably wrong).
- Connect your computer or handheld device to the WiFi network "TCD-AP".
- Navigate your browser to http://timecircuits.local or http://192.168.4.1 to enter the Config Portal.
 
If you want your clock to connect to your WiFi network, click on "Configure WiFi". The bare minimum is to select an SSID (WiFi network name) and a WiFi password. Note that the device requests an IP address via DHCP, unless you entered valid data in the fields for static IP addresses (IP, gateway, netmask, DNS). (If the device is inaccessible as a result of wrong static IPs, hold ENTER when powering it up until the white LED lits; static IP data will be deleted and the device will return to DHCP.) After saving the WiFi network settings, the device reboots and tries to connect to your configured WiFi network. If that fails, it will again start in access point mode.

The next step is to set the clock's ... clock (and time zone).

If your clock is connected to a WiFi network with internet access, it will receive time information through NTP (network time protocol). If the clock shows a wrong time initially, don't worry: This is due to a wrong time zone.

If the internet is inaccessible, please set your local time through the [keypad menu](#how-to-set-the-real-time-clock-rtc). 

In both cases it is, again, important to set the clock's time zone. This is done in the Config Portal, so read on.

### The Config Portal

The Config Portal is accessible exclusively through WiFi. As outlined above, if the device is not connected to a WiFi network, it creates its own WiFi network (named "TCD-AP"), to which your WiFi-enabled hand held device or computer first needs to connect in order to access the Config Portal.

If the operating system on your handheld or computer supports Bonjour (or "mDNS"), you can connect to the Config Portal by directing your browser to http://timecircuits.local. (mDNS is supported on Windows 10 version TH2 (1511) [other sources say 1703] and later, Android 13 and later, MacOS, iOS)

If that fails, the way to connect to the Config Portal depends on whether the clock is in access point mode or not. 
- If it is in access point mode (and your handheld/computer is connected to the WiFi network "TCP-AP"), navigate your browser to http://192.168.4.1 
- If the device is connected to your WiFi network, you need to find out its IP address first: Hold ENTER on the keypad for 2 seconds, then repeatedly press ENTER until "NET-WORK" is shown, then hold ENTER for 2 seconds. The device will then show its current IP address. Then, on your handheld or computer, navigate to http://a.b.c.d (a.b.c.d being the IP address as shown on the display) in order to enter the Config Portal.

In the main menu, click on "Setup" to configure your clock, first and foremost your time zone. If the time zone isn't set correctly, the clock might show a wrong time, and DST (daylight saving) will not be switched on/off correctly.

| ![The_Config_Portal](https://user-images.githubusercontent.com/76924199/235524035-afa58901-8115-42d6-a896-f65d4a7a56c2.png) |
|:--:| 
| *The Config Portal's Setup page* |

A full reference of the Config Portal is [here](https://github.com/CircuitSetup/Time-Circuits-Display/wiki/Appendix:-The-Config-Portal-&-Sensor-Wiring#appendix-a-the-config-portal).

## Basic Operation

*Present time* is a clock and normally shows the actual local present time, as received from the network or set up through the [keypad menu](#how-to-set-the-real-time-clock-rtc).

*Destination time* and *Last time departed* are stale. These, by default, work like in the movie: Upon a time travel, *present time* becomes *last time departed*, and *destination time* becomes *present time*.

The clock only supports the [Gregorian Calendar](https://en.wikipedia.org/wiki/Gregorian_calendar), of which it pretends to have been used since year 1. The [Julian Calendar](https://en.wikipedia.org/wiki/Julian_calendar) is not taken into account. Therefore, some years that, in the then-used Julian Calendar, were leap years between years 1 and 1582 most of today's Europe (and its Spanish colonies in the Americas), 1700 in DK/NO/NL (except Holland and Zeeland), 1752 in the British Empire (and therefore today's USA, Canada and India), 1753 in Sweden, 1760 in Lorraine, 1872 in Japan, 1912 in China, 1915 in Bulgaria, 1917 in the Ottoman Empire, 1918 in Russia and Serbia and 1923 in Greece, are normal years in the Gregorian one. As a result, dates do not match in those two calender systems, the Julian calendar is currently 13 days behind. I wonder if Doc's TC took all this into account. (Then again, he wanted to see Christ's birth on Dec 25, 0. Luckily, he didn't actually try to travel to that date. Assuming a negative roll-over, he might have ended up in eternity.)

Neither the Gregorian nor the Julian Calendar know a "year 0"; 1AD followed after 1BC. Nevertheless, it is possible to travel to year 0. In good old Hollywood tradition, I won't let facts and science stand in the way of an authentic movie experience.

If "REPLACE BATTERY" is shown upon boot, the onboard CR2032 battery is depleted and needs to be replaced. Note that, for technical reasons, "REPLACE BATTERY" will also show up the very first time you power-up the clock *after* changing the battery. You can, of course, disregard that message in this case.

### Time-cycling

"Time cycling" is a kind of "decorative" mode in which the device cycles through a list of pre-programmed *destination* and *last time departed* times. These pre-programmed times match the dates/times of all time-travels that take place in the three movies.

Time-cycling is enabled by setting the "Time-cycling Interval" in the Config Portal or the [keypad menu](#how-to-select-the-time-cycling-interval) to a non-zero value. The device will then cycle through named list every 5th, 10th, 15th, 30th or 60th minute. 

Time-cycling will, if enabled, change the *Destination* and *Last Time Departed* displays regardless of the times already displayed, for instance as a result from an earlier time travel. Triggering a time-travel will, however, pause time-cycling for 30 minutes.

Set the interval to OFF (0) to disable Time-cycling.

### World Clock mode

World Clock (WC) mode is another kind of "decorative" mode where the red and yellow displays show not some stale times, but current time in other time zones. These time zones need to be configured in Config Portal. At least one time zone (for either the red or yellow display) must be configured in order to use WC mode. Optionally, also names for cities/locations for these time zones can be entered in the Config Portal; if a name for a time zone is configured, this name and the time will alternately be shown; if no name is configured, time will be shown permanently. Note that names can only contain letters a-z, numbers 0-9, space and minus.

WC mode is toggled by typing "112" followed by ENTER. 

For logical reasons, WC mode will be automatically disabled in some situations:

- Time travel. The time travel sequence runs like in non-WC mode: If a time travel is triggered while WC mode is enabled (and no new destination time was entered before), the currently shown *Destination Time* will be the travel target, and the *Last Time Departed* display will show your formerly current time. However: Both *Destination Time* as well as *Last Time Departed* become stale after the time travel as per the nature of the sequence.
- After entering a destination time. WC mode is disabled at this point, because your new *Destination Time* would be overwritten otherwise.

#### WC/RC hybrid mode

[Room Condition (RC) mode](#room-condition-mode-temperaturehumidity-sensor) can be enabled together with WC mode. In that case, only one timezone is used, and the other display shows the temperature. If there is a time zone configured for the red display, the temperature will be shown in the yellow display. If there no time zone for the red display, the temperature will be shown there, and the yellow display will show time for the time zone you set up for the yellow display.

To toggle WC/RC hybrid mode, type "113" followed by ENTER. 

### Common usage scenarios

####  &#9654; I want my clock to work like in the movie

In this case, head to the Config Portal and
- set the *Time Cycling Interval* to OFF
- check or uncheck *Make time travels persistent* depending on whether you care about keeping your times across reboots

Note that *actual* time travel is not supported.

#### 	&#9654; I want my clock to show/cycle movie times

In this case, head to the Config Portal and
- set the *Time Cycling Interval* to the desired interval
- uncheck *Make time travels persistent*

Time-travelling will interrupt the cycling of movie times for 30 minutes.

#### 	&#9654; I want my clock to always show my favorite *Destination* and *last time departed* times

In this case, head to the Config Portal and
- set the *Time Cycling Interval* to OFF
- uncheck *Make time travels persistent*

Then enter the [keypad menu](#how-to-enter-datestimes-for-the-destination-and-last-time-departed-displays) and set your favorite *Destination* and *Last time departed* times.

Note that time-travelling will naturally lead to the displays showing other times. After a reboot, your times will be displayed again.

### Keypad reference

In the following, "pressing" means briefly pressing a key, "holding" means keeping the key pressed for 2 seconds or longer.

mm = month (01-12, 2 digits); dd = day (01-31, 2 digits); yyyy = year (4 digits); hh = hour (00-23, 2 digits); MM = minute (00-59, 2 digits)

<table>
    <tr>
     <td align="center" colspan="2">Keypad reference: Destination time programming<br>(&#9166; = ENTER key)</td>
    </tr>
    <tr>
     <td align="center">mmddyyyyhhMM&#9166;</td>
     <td align="center">Set complete date/time for <a href="#time-travel">Time Travel</a></td>
    </tr>
    <tr>
     <td align="center">mmddyyyy&#9166;</td>
     <td align="center">Set date for <a href="#time-travel">Time Travel</a></td>
    </tr>
    <tr>
     <td align="center">hhMM&#9166;</td>
     <td align="center">Set time for <a href="#time-travel">Time Travel</a></td>
    </tr>
</table>

<table>
    <tr>
     <td align="center" colspan="2">Keypad reference: Special key sequences<br>(&#9166; = ENTER key)</td>
    </tr>
    <tr>
     <td align="center">11&#9166;</td>
     <td align="center">Show current <a href="#how-to-set-up-the-alarm">alarm</a> time/weekday</td>
    </tr>
    <tr>
     <td align="center">11hhMM&#9166;</td>
     <td align="center">Set <a href="#how-to-set-up-the-alarm">alarm</a> to hh:MM</td>
    </tr>
    <tr>
     <td align="center">44&#9166;</td>
     <td align="center"><a href="#count-down-timer">Timer</a>: Show remaining time</td>
    </tr>
    <tr>
     <td align="center">44MM&#9166;</td>
     <td align="center"><a href="#count-down-timer">Timer</a>: Set timer to MM minutes</td>
    </tr>
    <tr>
     <td align="center">77&#9166;</td>
     <td align="center"><a href="#yearlymonthly-reminder">Reminder</a>: Display reminder</td>
    </tr>
    <tr>
     <td align="center">777&#9166;</td>
     <td align="center"><a href="#yearlymonthly-reminder">Reminder</a>: Display time until reminder</td>
    </tr>
    <tr>
     <td align="center">770&#9166;</td>
     <td align="center"><a href="#yearlymonthly-reminder">Reminder</a>: Delete reminder</td>
    </tr>
    <tr>
     <td align="center">77mmdd&#9166;</td>
     <td align="center"><a href="#yearlymonthly-reminder">Reminder</a>: Program reminder</td>
    </tr>
    <tr>
     <td align="center">77mmddhhMM&#9166;</td>
     <td align="center"><a href="#yearlymonthly-reminder">Reminder</a>: Program reminder</td>
    </tr>
    <tr>
     <td align="center">111&#9166;</td>
     <td align="center">Toggle <a href="#room-condition-mode-temperaturehumidity-sensor">Room Condition mode</a></td>
    </tr>
    <tr>
     <td align="center">112&#9166;</td>
     <td align="center">Toggle <a href="#">World Clock mode</a></td>
    </tr>
    <tr>
     <td align="center">113&#9166;</td>
     <td align="center">Synchronously toggle <a href="#">World Clock mode</a> and <a href="#room-condition-mode-temperaturehumidity-sensor">Room Condition mode</a></td>
    </tr>
    <tr>
     <td align="center">222&#9166;</td>
     <td align="center"><a href="#the-music-player">Music Player</a>: Shuffle off</td>
    </tr>
    <tr>
     <td align="center">555&#9166;</td>
     <td align="center"><a href="#the-music-player">Music Player</a>: Shuffle on</td>
    </tr>
    <tr>
     <td align="center">88&#9166;</td>
     <td align="center"><a href="#the-music-player">Music Player</a>: Show currently played song number</td>
    </tr>
    <tr>
     <td align="center">888&#9166;</td>
     <td align="center"><a href="#the-music-player">Music Player</a>: Go to song 0</td>
    </tr>
    <tr>
     <td align="center">888xxx&#9166;</td>
     <td align="center"><a href="#the-music-player">Music Player</a>: Go to song xxx</td>
    </tr>
    <tr>
     <td align="center">000&#9166;</td>
     <td align="center">Disable <a href="#beep-on-the-second">beep</a> sound</td>
    </tr>
    <tr>
     <td align="center">001&#9166;</td>
     <td align="center">Enable <a href="#beep-on-the-second">beep</a> sound</td>
    </tr>
   <tr>
     <td align="center">002&#9166;</td>
     <td align="center">Enable <a href="#beep-on-the-second">beep</a> sound (30 seconds)</td>
    </tr>
   <tr>
     <td align="center">003&#9166;</td>
     <td align="center">Enable <a href="#beep-on-the-second">beep</a> sound (60 seconds)</td>
    </tr>
    <tr>
     <td align="center">64738&#9166;</td>
     <td align="center">Reboot the device</td>
    </tr>
</table>

<table>
    <tr>
     <td align="center" colspan="3">Keypad reference: Holding keys for 2 seconds</td>
    </tr>
    <tr>
     <td align="center">1<br>Toggle <a href="#how-to-set-up-the-alarm">Alarm</a> on/off</td>
     <td align="center">2<br><a href="#the-music-player">Music Player</a>: Previous song</td>
     <td align="center">3<br><a href="#additional-custom-sounds">Play "key3.mp3"</a></td>
    </tr>
    <tr>
     <td align="center">4<br>Toggle <a href="#night-mode">Night mode</a> on/off</td>
     <td align="center">5<br><a href="#the-music-player">Music Player</a>: Play/Stop</a></td>
     <td align="center">6<br><a href="#additional-custom-sounds">Play "key6.mp3"</a></td>
    </tr>
    <tr>
     <td align="center">7<br><a href="#wifi-power-saving-features">Re-enable WiFi</a></td>
     <td align="center">8<br><a href="#the-music-player">Music Player</a>: Next song</td>
     <td align="center">9<br><a href="#time-travel">Return from Time Travel</a></td>
    </tr>
    <tr>
     <td align="center"></td>
     <td align="center">0<br><a href="#time-travel">Time Travel</a></td>
     <td align="center"></td>
    </tr>
</table>

## Time travel

To travel through time, hold "0" for 2 seconds. The *destination time*, as shown in the red display, will be your new *present time*, the old *present time* will be the *last time departed*. The new *present time* will continue to run like a normal clock.

Before holding "0", you can also first quickly set a new destination time by entering a date on the keypad: mmddyyyy, mmddyyyyhhMM or hhMM, then press ENTER. While typing, there is no visual feedback, but the date is then shown on the *destination time* display after pressing ENTER.

To travel back to actual present time, hold "9" for 2 seconds.

Beware that the alarm function, by default, is based on the real actual present time, not the time displayed. This can be changed in the Config Portal.

### Persistent / Non-persistent time travels

On the Config Portal's "Setup" page, there is an option item named "Make time travels persistent". The default is off.

If time travels are persistent
- a user-programmed *destination time* is always stored in flash memory, and retrieved from there after a reboot. It can be programmed through the keypad menu, or ahead of a time travel by typing mmddyyyyhhMM/mmddyyyy/hhMM plus ENTER. In both cases, the time is stored in flash memory and retrieved upon power-up/reboot.
- *last time departed* as displayed at any given point is always stored in flash memory, and retrieved upon power-up/reboot.
- *present time*, be it actual present time or "fake" after time travelling, will continue to run while the device is not powered, as long as its battery lasts, and displayed on power-up/reboot.

If time travels are non-persistent
- a user-programmed *destination time* is only stored to flash memory when programmed through the keypad menu, but not if entered ahead of a time travel (ie outside of the keypad menu, just by typing mmddyyyyhhMM/mmddyyyy/hhMM plus ENTER).
- a user-programmed *last time departed* is only stored to flash memory when programmed through the keypad menu, but not if the result of a time travel.
- *present time* is always reset to actual present time upon power-up/reboot.

If you want your device to display exactly the same after a power loss, choose persistent (and disable [Time-cycling](#time-cycling)).

If you want to display your favorite *destination time* and *last time departed* upon power-up, and not have time travels overwrite them in flash memory, choose "non-persistent", and program your times through the [keypad menu](#how-to-enter-datestimes-for-the-destination-and-last-time-departed-displays) (and disable [Time-cycling](#time-cycling)). Those times will never be overwritten in flash memory by later time travels. Note, however, that the times displayed might actually change due to time travels.

Note that Time-cycling, if enabled, will force the device to cycle through the list of pre-programmed times, regardless of your time travel persistence setting. So, if Time-cycling is enabled, the only effect of persistence is that *Present Time* is kept at what it was before vs. reset to actual present time after a power loss.

Persistent time travels, if done often, will cause [Flash Wear](#flash-wear).

## Beep on the second

In the movies, the Time Circuits emit a "beep" sound every second, which is only really audible in the scene in which Doc explains to Marty how the time machine works. The firmware supports that beep, too.

The beep can be permanently disabled, permanently enabled, or enabled for 30 or 60 seconds
- after a destination time is entered (and ENTER is pressed),
- upon triggering a time travel,
- after switching on the clock (real power-up or fake power-up).

The different modes are selected by typing 000 (disabled), 001 (enabled), 002 (enabled for 30 secs) or 003 (enabled for 60 secs), followed by ENTER. The power-up default is selected in the Config Portal.

Since the hardware only has one audio channel, the beep is suppressed whenever other sounds are played-back.

## Night mode

In night-mode, by default, the *destination time* and *last time departed* displays are switched off, the *present time* display is dimmed to a minimum, and the volume of sound playback is reduced (except the alarm). Apart from considerably increasing the displays' lifetime, night-mode reduces the power consumption of the device from around 4.5W to around 2.5W.

You can configure the displays' behavior in night-mode in the Config Portal: They can individually be dimmed or switched off in night-mode.

#### Manually switching to night-mode

To toggle night-mode on/off manually, hold "4".

#### Scheduled night-mode

In the Config Portal, a schedule for night-mode can be programmed. You can choose from currently four time schedule presets, or a daily schedule with selectable start and end hours.

The presets are for (hopefully) typical home, office and shop setups, and they assume the clock to be in use (ie night-mode off) at the following times:
- Home: Mon-Thu 5pm-11pm, Fri 1pm-1am, Sat 9am-1am, Sun 9am-11pm
- Office (1): Mon-Fri 9am-5pm
- Office (2): Mon-Thu 7am-5pm, Fri 7am-2pm
- Shop: Mon-Wed 8am-8pm, Thu-Fri 8am-9pm, Sat 8am-5pm

The *daily* schedule works by entering start and end in the text fields below. The clock will go into night-mode at the defined start hour (xx:00), and return to normal operation at the end hour (yy:00). 

Night mode schedules are always based on actual local present time.

#### Sensor-controlled night-mode

You can also connect a light sensor to the device. Four sensor types/models are supported: TSL2561, BH1750, VEML7700/VEML6030, LTR303/LTR329, connected through i2c with their respective default slave address. The VEML7700 can only be connected if no GPS receiver is connected at the same time; the VEML6030 needs its address to be set to  0x48 if a GPS receiver is present at the same time. All these sensor types are readily available on breakout boards from Adafruit or Seeed (Grove). Only one light sensor can be used at the same time. *Note: You cannot connect the sensor chip directly to the TCD control board; most sensors need at least a power converter/level-shifter.* This is why I exclusively used Adafruit or Seeed breakouts ([TSL2561](https://www.adafruit.com/product/439) or [here](https://www.seeedstudio.com/Grove-Digital-Light-Sensor-TSL2561.html), [BH1750](https://www.adafruit.com/product/4681), [VEML7700](https://www.adafruit.com/product/4162), [LTR303](https://www.adafruit.com/product/5610)), which all allow connecting named sensors to the 5V the TCD board operates on. For wiring information, see [here](#appendix-b-sensor-wiring).

If the measured lux level is below or equal the threshold set in the Config Portal, the device will go into night-mode. To view the currently measured lux level, use the [keypad menu](#how-to-view-sensor-info).

If both a schedule is enabled and the light sensor option is checked in the Config Portal, the sensor will overrule the schedule only in non-night-mode hours; ie it will never switch off night-mode when night-mode is active according to the schedule.

Switching on/off night-mode manually deactivates any schedule and the light sensor for 30 minutes. Afterwards, a programmed schedule and/or the light sensor will overrule the manual setting.

## Count-down timer

The firmware features a simple count-down timer. This timer can count down from max 99 minutes and plays a sound when expired.

To set the timer to MM minutes, type 44MM and press ENTER. Note that for minutes, 2 digits must be entered. To cancel a running timer, enter 4400 and press ENTER.

The check the remaining time, type 44 and press ENTER.

## Yearly/monthly reminder

The reminder is similar to the alarm, the difference being that the reminder is yearly or monthly, not daily. 

To program a yearly reminder, enter 77mmddhhMM and press ENTER. For example: 7705150900 sets the reminder to May 15 9am. Now a reminder sound will play every year on May 15 at 9am.

To program a monthly reminder, enter 7700ddhhMM and press ENTER. For example: 7700152300 sets the reminder to the 15th of each month, at 11pm.

You can also leave out the hhMM part; in that case the time remains unchanged from a previous setting, unless both hour and minute were 0 (zero), in which case the reminder time is set to 9am.

Note that, as usual, all fields consist of two digits, and hours are entered in 24-hour notation.

Type 77 followed by ENTER to display the programmed reminder, 770 to delete it, and 777 to display the days/hours/minutes until the next reminder.

The reminder only plays a sound file. The current sound-pack contains a default file; if your SD contains a file named "reminder.mp3", this will played instead of the default file.

Note: While the alarm and the sound-on-the-hour adhere to the "Alarm base is real present time" setting in the config portal, the Reminder does not. It is always based on real local time.

## SD card

Preface note on SD cards: For unknown reasons, some SD cards simply do not work with this device. For instance, I had no luck with a Sandisk Ultra 32GB card. If your SD card is not recognized, check if it is formatted in FAT32 format (not exFAT!). Also, the size must not exceed 32GB (as larger cards cannot be formatted with FAT32).

The SD card, apart from being used to [install](#audio-file-installation) the default audio files, can be used for substituting default sounds, some additional custom sounds, and for music played back by the [Music player](#the-music-player).

Note that the SD card must be inserted before powering up the clock. It is not recognized if inserted while the clock is running. Furthermore, do not remove the SD card while the clock is powered.

### Sound file substitution

The provided audio files ("sound-pack") are, after [proper installation](#audio-file-installation), integral part of the firmware and stored in the device's flash memory. 

These sounds can be substituted by your own sound files on a FAT32-formatted SD card. These files will be played back directly from the SD card during operation, so the SD card has to remain in the slot. The built-in [Audio file installer](#audio-file-installation) cannot be used to replace default sounds in the device's flash memory with custom sounds.

Your replacements need to be put in the root (top-most) directory of the SD card, be in mp3 format (128kbps max) and named as follows:
- "alarm.mp3". Played when the alarm sounds.
- "alarmon.mp3". Played when enabling the alarm
- "alarmoff.mp3". Played when disabling the alarm
- "nmon.mp3". Played when manually enabling night mode
- "nmoff.mp3". Played when manually disabling night mode
- "reminder.mp3". Played when the reminder is due.
- "timer.mp3". Played when the count-down timer expires.

The following sounds are time-sync'd to display action. If you decide to substitute these with your own, be prepared to lose synchronicity:
- "enter.mp3". Played when a date was entered and ENTER was pressed
- "baddate.mp3". Played when a bad (too short or too long) date was entered and ENTER was pressed
- "intro.mp3": Played during the power-up intro
- "travelstart.mp3". Played when a time travel starts.
- "timetravel.mp3". Played when re-entry of a time travel takes place.
- "shutdown.mp3". Played when the device is fake "powered down" using an external switch (see below)
- "startup.mp3". Played when the clock is connected to power and finished booting

If you intend to use the very same SD card that you used for installing the default sound files, please remove the files from the sound-pack from the SD card first.

### Additional Custom Sounds

The firmware supports some additional user-provided sound effects, which it will load from the SD card. If the respective file is present, it will be used. If that file is absent, no sound will be played.

- "hour.mp3": Will be played every hour, on the hour. This feature is disabled in night mode.
- "hour-xx.mp3", xx being 00 through 23: Sounds-on-the-hour for specific hours that will be played instead of "hour.mp3". If a sound for a specific hour is not present, "hour.mp3" will be played, if that one exists.
- "key3.mp3" and/or "key6.mp3": Will be played if you hold the "3"/"6" key for 2 seconds.
- "ha-alert.mp3" will be played when a [HA/MQTT](#home-assistant--mqtt) message is received.

"hour.mp3"/"hour-xx.mp3", "key3.mp3", "key6.mp3" and "ha-alert.mp3" are not provided here. You can use any mp3, with 128kpbs or less.

## The Music Player

The firmware contains a simple music player to play mp3 files located on the SD card. In order to be recognized, your mp3 files need to be organized in music folders named *music0* through *music9* and their filenames must only consist of three-digit numbers, starting at 000.mp3, in consecutive order. No numbers should be left out. Each folder can hold 1000 files (000.mp3-999.mp3). *The maximum bitrate is 128kpbs.*

The folder number is 0 by default, ie the player starts searching for music in folder *music0*. This folder number can be changed in the [keypad menu](#how-to-select-the-music-folder-number).

To start and stop music playback, hold 5. Holding 2 jumps to the previous song, holding 8 to the next one.

By default, the songs are played in order, starting at 000.mp3, followed by 001.mp3 and so on. By entering 555 and pressing ENTER, you can switch to shuffle mode, in which the songs are played in random order. Enter 222 followed by ENTER to switch back to consecutive mode.

Entering 888 followed by ENTER re-starts the player at song 000, and 888xxx (xxx = three-digit number) jumps to song #xxx.

See [here](#keypad-reference) for a list of controls of the music player.

While the music player is playing music, most sound effects are disabled/muted, such as keypad sounds, sound-on-the-hour, sounds for switching on/off the alarm and night-mode. Initiating a time travel stops the music player, as does entering the keypad menu. The alarm will sound as usual and thereby stop the music player.

## The keypad menu
 
The keypad menu is an additional way to configure your clock; it only involves the three displays and the keypad. It is controlled by "pressing" or "holding" the ENTER key on the keypad.

A "press" is shorter than 2 seconds, a "hold" is 2 seconds or longer.

The menu is invoked by holding the ENTER button.

*Note that if the keypad menu is active at a time when the alarm, the reminder, the count-down timer or sound-on-the-hour are due, those events will be missed and no sounds are played.*

Data entry, such as for dates and times, is done through the keypad's number keys and works as follows: Whenever a data entry is requested, the field for that data is lit (while the rest of the display is dark) and a pre-set value is shown. If you want to keep that pre-set, press ENTER to proceed to next field. Otherwise press a digit on the keypad; the pre-set is then overwritten by the value entered. 2 digits can be entered (4 for years), upon which the new value is stored and the next field is activated. You can also enter less than 2 digits (4 for years) and press ENTER when done with the field. Note that a month needs to be entered numerically (1-12), and hours need to be entered in 24-hour notation (0-23), regardless of 12-hour or 24-hour mode as per the Config Portal setting.

After invoking the keypad menu, the first step is to choose a menu item. The available items are  
- install the default audio files ("INSTALL AUDIO FILES")
- set the alarm ("ALA-RM"),
- set the audio volume (VOL-UME),
- set the Music Player folder number ("MUSIC FOLDER NUMBER")
- select the Time-cycling Interval ("TIME-CYCLING"),
- select the brightness for the three displays ("BRIGHTNESS"),
- show network information ("NET-WORK"),
- set the internal Real Time Clock (RTC) ("SET RTC"),
- enter dates/times for the *Destination* and *Last Time Departed* displays,
- show light/temperature/humidity sensor info (if such a sensor is connected) ("SENSORS"),
- show when time was last sync'd with NTP or GPS ("TIME SYNC"),
- quit the menu ("END").
 
Pressing ENTER cycles through the list, holding ENTER selects an item.

#### How to install the default audio files:

- Hold ENTER to invoke main menu
- If the SD card holds the files of the sound-pack archive from this repo, "INSTALL AUDIO FILES" is shown as the first menu item. See the [Audio file installation](#audio-file-installation) section.
- Hold ENTER to proceed
- Press ENTER to toggle between "CANCEL" and "PROCEED"
- Hold ENTER to proceed. If "PROCEED" was chosen, the audio files fill be installed and the clock will reboot.
 
#### How to set up the alarm:

- Hold ENTER to invoke main menu
- (Currently, the alarm is the first menu item; otherwise press ENTER repeatedly until "ALA-RM" is shown)
- Hold ENTER
- Press ENTER to toggle the alarm on and off, hold ENTER to proceed
- Then enter the hour and minutes. This works as described above.
- Choose the weekday(s) by repeatedly pressing ENTER
- Hold ENTER to finalize your weekday selection. "SAVING" is displayed briefly.

When the alarm is set and enabled, the dot in the present time's minute field will light up. 

Under normal operation (ie outside of the menu), holding "1" toggles the alarm on/off.

The alarm time can also quickly be set by typing 11hhMM (eg. 110645 for 6:45am, or 112300 for 11:00pm) and pressing ENTER, just like when setting a time travel destination time. (The weekday selection has still to be done via the keypad menu.) Typing 11 followed by ENTER shows the currently set time and weekday selection briefly.

Note that the alarm is recurring, ie it rings at the programmed time, unless disabled. Also note, as mentioned, that the alarm is by default relative to your actual *present time*, not the time displayed (eg after a time travel). It can, however, be configured to be based on the time displayed, in the Config Portal.

*Important: The alarm will not sound when the keypad menu is active at the programmed alarm time.*

#### How to set the audio volume:

Basically, and by default, the device uses the hardware volume knob to determine the desired volume. You can change this to a fixed-level setting as follows:
- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "VOL-UME" is shown
- Hold ENTER
- Press ENTER to toggle between "USE VOLUME KNOB" and "FIXED LEVEL"
- Hold ENTER to proceed
- If you chose "FIXED LEVEL/"SW", you can now select the desired level by pressing ENTER repeatedly. There are 20 levels available. The volume knob is now ignored.
- Hold ENTER to save and quit the menu

#### How to select the music folder number:

In order for this menu item to show up, an SD card is required.

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "MUSIC FOLDER NUMBER" is shown
- Hold ENTER, "NUM" is displayed
- Press ENTER repeatedly to cycle through the possible values. The message "NOT FOUND" appears if either the folder itself or 000.mp3 in that very folder is not present.            
- Hold ENTER to select the value shown and exit the menu ("SAVING" is displayed briefly)

If shuffle was enabled before, the new folder is also played in shuffled order.

Note that the Music Folder Number is saved in a config file on the SD card.

#### How to select the Time-cycling Interval:

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "TIME-CYCLING" is shown
- Hold ENTER, "INTERVAL" is displayed
- Press ENTER repeatedly to cycle through the possible Time-cycling Interval values.
- A value of 0 disables automatic time cycling ("OFF").
- Non-zero values make the device cycle through a number of pre-programmed times. The value means "minutes" (hence "MIN-UTES") between changes.              
- Hold ENTER to select the value shown and exit the menu ("SAVING" is displayed briefly)
 
#### How to adjust the display brightness:

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "BRIGHTNESS" is shown
- Hold ENTER, the displays show all elements, the top-most display says "LVL"
- Press ENTER repeatedly to cycle through the possible levels (1-15)
- Hold ENTER to use current value and proceed to next display
- After the third display, "SAVING" is displayed briefly and the menu is left automatically.
 
#### How to find out the IP address and WiFi status:

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "NET-WORK" is shown
- Hold ENTER, the displays show the IP address
- Repeatedly press ENTER to cycle between IP address, WiFi status, MAC address (in station mode) and Home Assistant connection status.
- Hold ENTER to leave the menu

#### How to set the Real Time Clock (RTC):

Adjusting the RTC is useful if you can't use NTP for time keeping, and really helpful when using GPS. Always set your actual local present time here; if you want to display some other time, use the Time Travel function. Note: The time you entered will be overwritten if/when the device has access to authoritative time such as via NTP or GPS. For DST (daylight saving) and GPS, it is essential that you also set the correct time zone in the [Config Portal](#the-config-portal).

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "SET RTC" is displayed and the *Present Time* display shows a date and time 
- Hold ENTER until the *Present Time* display goes off except for the first field to enter data into
- The field to enter data into is shown (exclusively), pre-set with its current value
- Data entry works as described [above](#the-keypad-menu); remember that months need to be entered numerically (01-12), and hours in 24-hour notation (0-23).
- After entering data into all fields, the data is saved and the menu is left automatically.

#### How to enter dates/times for the *Destination* and *Last Time Departed* displays:

Note that when entering dates/times into the *destination time* or *last time departed* displays, the Time-cycling Interval is automatically set to 0. Your entered date/time(s) are shown until overwritten by time travels.

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until the desired display is the only one lit and shows a date and time
- Hold ENTER until the display goes off except for the first field to enter data into
- The field to enter data into is shown (exclusively), pre-set with its current value
- Data entry works as described [above](#the-keypad-menu); remember that months need to be entered numerically (01-12), and hours in 24-hour notation (0-23).
- After entering data into all fields, the data is saved and the menu is left automatically.

#### How to view sensor info

- Hold ENTER to invoke main menu
- Press ENTER repeatedly until "SENSORS" is shown. If that menu item is missing, a light or temperature sensor was not detected during boot.
- Hold ENTER
- Now the currently measured lux level or temperature is displayed.
- Press ENTER to toggle between light sensor and temperature sensor info (if both are connected)
- Hold ENTER to exit the menu

Note: Sometimes, light sensors report a lux value of -1. This is mostly due to the fact that all the supported sensors are adjusted for indoor usage and might overload in broad daylight. Also, some sensors might have issues with halogen lamps (reportedly TSL2561), and most sensors also "overload" if too much IR light is directed at them, for instance from surveillance cameras.

#### How to leave the menu:
 
 - While the menu is active, press ENTER repeatedly until "END" is displayed.
 - Hold ENTER to leave the menu

## Fake power Switch 

You probably noticed that the device takes longer to boot than would be required to re-create the movie experience where Doc turns the knob and the Time Circuits immediately turn on. As a remedy, the firmware supports a fake "power switch": 

If corresponding option is enabled in the Config Portal (*Use fake power switch*), the device will power-up, initialize everything, but stay quiet and dark. Only when the fake "power switch" is activated, the device will visually "power up". Likewise, you can also fake "power off" the device using this switch. Fake "off" disables the displays, all audio (except the alarm) and the keypad. Just like in the movie.

On Control Boards V1.3 and later, there is a dedicated header labeled "Fake PWR" to connect the switch to. The pins to be shortened by the switch are labeled "GND" and "PWR Trigger".

On earlier Control Boards (1.2 and below), the switch needs shorten the pins labeled "IO13" and "GND" as shown here:

![access_to_io13](https://user-images.githubusercontent.com/76924199/194283241-3bee5fef-f51a-4b1a-a158-ed92292bcf32.jpg)

Here is a close-up of a v1.2 control board; Headers have been soldered to the io ports

![io13neu](https://user-images.githubusercontent.com/76924199/212369689-f945dcf1-55f9-41e9-8fd7-fc0dbc69906c.jpg)

Note that the switch actually needs to be a switch with a maintained contact; the pins need to remain connected for as long as the device is fake-switched-on.

In order to use the Fake Power Switch, check **Use fake power switch** in the Config Portal.

## External Time Travel Trigger

As mentioned above, a time travel can be triggered by holding "0" on the keypad. Since this doesn't really allow for an authentic movie-like experience, the firmware also supports an external trigger, such as a button switch or even another prop to trigger a time travel. Note that, unlike the [Fake Power Switch](#fake-power-switch), this trigger must be a momentary toggle.

On Control Boards V1.3 and later, there is a dedicated header for the button labeled "Time Travel". The button needs to shorten pins "TT IN" and "GND".

Unfortunately, there is no header and no break out for IO27 on TC control boards V1.2 and below. Some soldering is required. The button needs to be connected to the two marked pins in the image below:

![nodemcuio27](https://user-images.githubusercontent.com/76924199/194284838-635419f9-5eb7-4480-8693-2bf7cfc7c744.jpg)

Luckily, there is a row of solder pads right next to the socket on the control board, where a pin header or cable can easily be soldered on:

![tcboard_io27](https://user-images.githubusercontent.com/76924199/194284336-2fe9fa9b-d5e5-49f5-b1cd-b0fd2abdff53.jpg)

In order to trigger a time-travel sequence on the Time Circuits, "TT IN"/IO27 and GND must be shortened for at least 200ms and then opened; the time travel is triggered upon release of the button. If the button is pressed for 3000ms (3 seconds), a ["Return from Time Travel"](#time-travel) is triggered.

The Config Portal allows configuring a delay for matching/synchronizing the TCD to another prop. The delay, if any, starts running after the button is released. The time travel sequence starts after the delay has expired.

## Speedometer

The firmware supports a speedometer display connected via i2c (slave address 0x70) as part of the time travel sequence. Unfortunately, CircuitSetup's upcoming Speedo display is yet in the design stage, so you are on your own to build one for the time being.

What you need is a box, the LED segment displays and a HT16K33-based PCB that allows accessing the LED displays via i2c. There are various readily available LED segment displays with suitable i2c break-outs from Adafruit and Seeed (Grove) that can be used as a basis. Adafruit [878](https://www.adafruit.com/product/878), [1270](https://www.adafruit.com/product/1270) and [1911](https://www.adafruit.com/product/1911), as well as Grove 0.54" 14-segment [2-digit](https://www.seeedstudio.com/Grove-0-54-Red-Dual-Alphanumeric-Display-p-4031.html) or [4-digit](https://www.seeedstudio.com/Grove-0-54-Red-Quad-Alphanumeric-Display-p-4032.html) alphanumeric displays are supported. (The product numbers vary with color, the numbers here are the red ones.)

| [![Watch the video](https://img.youtube.com/vi/opAZugb_W1Q/0.jpg)](https://youtu.be/opAZugb_W1Q) |
|:--:|
| Click to watch the video |

The speedo display shown in this video is based on a fairly well-designed stand-alone replica I purchased on ebay. I removed the electronics inside and wired the LED segments to an Adafruit i2c backpack (from the Adafruit 878 product) and connected it to the TCD. Yes, it is really that simple. (The switch was made by me, see [here](https://github.com/realA10001986/Time-Circuits-Display/wiki/Time-Circuits-Switch), it uses the [Fake Power Switch](#fake-power-switch) feature of the TCD.)

In order to use the Speedometer display, select the correct display type in the Config Portal. There are two special options in the Speedo Display Type drop-down: *Ada 1911 (left tube)* and *Ada 878 (left tube)*. These two can be used if you connect only one 2-digit-tube to the respective Adafruit i2c backpack.

Since the I2C bus is already quite long from the control board to the last display in the chain, I recommend soldering another JST XH 4pin plug onto the control board (there are two additional i2c break-outs available), and to connect the speedometer there.

## GPS receiver

The firmware supports an MT(K)3333-based GPS receiver, connected through i2c (slave address 0x10). The CircuitSetup-designed speedo display will have such a chip built-in, but since this gadget is not yet available, in the meantime, you can use alternatives such as the Adafruit Mini GPS PA1010D (product id [4415](https://www.adafruit.com/product/4415)) or the Pimoroni P1010D GPS Breakout ([PIM525](https://shop.pimoroni.com/products/pa1010d-gps-breakout?variant=32257258881107)). The GPS receiver can be used as a source of authoritative time (like NTP), and speed of movement (to be displayed on a [speedo display](#speedometer)).

GPS receivers receive signals from satellites, but in order to do so, they need to be "tuned in" (aka get a "fix"). This "tuning" process can take a long time; after first power up, it can take 30 minutes or more for a receiver to be able to determine its position. In order to speed up this process, modern GPS receivers have special "assisting" features. One key element is knowledge of current time, as this helps identifying satellite signals quicker. So, in other words, initially, you need to tell the receiver, what it is supposed to tell you. However, as soon as the receiver has received satellite signals for 15-20 minutes, it saves the data it collected to its battery-backed memory and will find a fix within seconds after power-up in the future.

For using GPS effectively as a long-term source of accurate time, it is therefore essential, that 
- the TimeCircuit's RTC (real time clock) is initially [set to correct local time](#how-to-set-the-real-time-clock-rtc), 
- the correct time zone is defined in the Config Portal,
- the GPS receiver has a battery
- and has been receiving data for 15-20 mins at least once a month.

If/as long as the GPS receiver has a fix and receives data from satellites, the dot in the present time's year field is lit.

In order to use the GPS receiver as a source of time, no special configuration is required. If it is detected during boot, it will be used.

#### GPS for speed

One nice feature of GPS is that the receiver can deliver current speed of movement. If the Time Circuits are, for instance, mounted in a car or on a boat, and a [speedo display](#speedometer) is connected, this display will be just that: A real speedometer.

| [![Watch the video](https://img.youtube.com/vi/wbeXZJaDLa8/0.jpg)](https://youtu.be/wbeXZJaDLa8) |
|:--:|
| Click to watch the video |

In order to use the GPS receiver for speed, check *Display GPS speed* in the Config Portal.

## Room Condition Mode, Temperature/humidity sensor

The firmware supports connecting a temperature/humidity sensor for "room condition mode"; in this mode, *destination* and *last departed* times are replaced by temperature and humidity (if applicable), respectively. To toggle between normal and room condition mode, enter "111" and press ENTER. 

![rcmode](https://user-images.githubusercontent.com/76924199/208133653-f0fb0a38-51e4-4436-9506-d841ef1bfa6c.jpg)

Room condition mode can be used together with [World Clock mode](#world-clock-mode); if both are enabled, only one alternative time and only temperature is shown. To toggle RC and WC mode simultaniously, type "113" and press ENTER.

Temperature on speedometer display: Unless you do time travelling on a regular basis, the [speedo display](#speedometer) is idle most of the time in a typical home setup. To give it more of a purpose, the firmware can display ambient temperature on the speedo while idle.

#### Hardware

In order to use a temperature/humidity sensor, no special configuration is required. If a sensor is detected by the firmware during boot, it will be used.

Seven sensor types are supported: MCP9808 (i2c address 0x18), BMx280 (0x77), SI7021 (0x40), SHT40 (0x44), TMP117 (0x49), AHT20/AM2315C (0x38), HTU31D (0x41). All of those are readily available on breakout boards from Adafruit or Seeed (Grove). The BMP280 (unlike BME280), MCP9808 and TMP117 work as pure temperature sensors, the others for temperature and humidity. For wiring information, see [here](#appendix-b-sensor-wiring).

*Note: You cannot connect the sensor chip directly to the TCD control board; most sensors need at least a power converter/level-shifter.* This is why I exclusively used Adafruit or Seeed breakouts ([MCP9808](https://www.adafruit.com/product/1782), [BME280](https://www.adafruit.com/product/2652), [SI7021](https://www.adafruit.com/product/3251), [SHT40](https://www.adafruit.com/product/4885), [TMP117](https://www.adafruit.com/product/4821), [AHT20](https://www.adafruit.com/product/4566), [HTU31D](https://www.adafruit.com/product/4832)), which all allow connecting named sensors to the 5V the TCD board operates on.

## Home Assistant / MQTT

The TCD supports the MQTT protocol version 3.1.1 for the following features:

### Display messages on TCD

The TCD can subscribe to a user-configured topic and display messages received for this topic on the *Destination Time* display. This can be used to display the status of other HA/MQTT devices, for instance alarm systems. If the SD card contains a file names "ha-alert.mp3", this file will be played upon reception of a message.

Only ASCII messages are supported, the maximum length is 255 characters.

### Control the TCD via MQTT

The TCD can - to a limited extent - be controlled through messages sent to topic **bttf/tcd/cmd**. Support commands are
- TIMETRAVEL: Start a time travel
- RETURN: Return from time travel
- BEEP_ON: Enables the *annoying beep*(tm)
- BEEP_OFF: Disables the *annoying beep*(tm)
- BEEP_30, BEEP_60: Set the beep modes as described [here](#beep-on-the-second)
- ALARM_ON: Enable the alarm
- ALARM_OFF: Disable the alarm
- NIGHTMODE_ON: Enable manual night mode
- NIGHTMODE_OFF: Disable manual night mode
- MP_PLAY: Starts the Music Player
- MP_STOP: Stops the Music Player
- MP_NEXT: Jump to next song
- MP_PREV: Jump to previous song
- MP_SHUFFLE_ON: Enables shuffle mode in Music Player
- MP_SHUFFLE_OFF: Disables shuffle mode in Music Player

### Trigger a time travel on other devices

Upon a time travel, the TCD can send messages to topic **bttf/tcd/pub**. This can be used to control other props wirelessly, such as Flux Capacitor, SID, etc. The timing is identical to the wired protocol, see [here](#controlling-other-props). TIMETRAVEL is sent when IO14 goes high, ie with a lead time (ETTO LEAD) of 5 seconds. REENTRY is sent when the re-entry sequence starts (ie when IO14 goes low). Note that network traffic has some latency, so timing might not be as exact as a wired connection.

When the [alarm](#how-to-set-up-the-alarm) sounds, the TCD can also send "ALARM" to **bttf/tcd/pub**.

### Setup

In order to connect to a MQTT network, a "broker" (such as [mosquitto](https://mosquitto.org/), [EMQ X](https://www.emqx.io/), [Cassandana](https://github.com/mtsoleimani/cassandana), [RabbitMQ](https://www.rabbitmq.com/), [Ejjaberd](https://www.ejabberd.im/), [HiveMQ](https://www.hivemq.com/) to name a few) must be present in your network, and its address needs to be configured in the Config Portal. The broker can be specified either by domain or IP (IP preferred, spares us a DNS call). The default port is 1883. If a different port is to be used, append a ":" followed by the port number to the domain/IP, such as "192.168.1.5:1884". 

If your broker does not allow anonymous logins, a username and password can be specified.

If you want your TCD to display messages as described above, you also need to specify the topic in the respective field.

If you want your TCD to publish messages to bttf/tcd/pub (ie if you want to notify other devices about the timetravel and/or the alarm), check the respective option.

Limitations: MQTT Protocol version 3.1.1; TLS/SSL not supported; ".local" domains (MDNS) not supported; maximum message length 255 characters; server/broker must respond to PING (ICMP) echo requests. For proper operation with low latency, it is recommended that the broker is on your local network. Note that using HA/MQTT will disable WiFi power saving (as described below).

## WiFi power saving features

The Config Portal offers two options for WiFi power saving, one for AP-mode (ie when the device acts as an access point), one for station mode (ie when the device is connected to a WiFi network). Both options do the same: They configure a timer after whose expiry WiFi is switched off; the device is no longer transmitting or receiving data over WiFi. 

The timers can be set to 0 (which disables them; WiFi is never switched off; this is the default), or 10-99 minutes. 

The reason for having two different timers for AP-mode and for station mode is that if the device is used in a car, it might act as an access point, while at home it is most probably connected to a WiFi network as a client. Since in a car, WiFi will most likely not be used on a regular basis, the timer for AP mode can be short (eg 10 minutes), while the timer for station mode can be disabled.

After WiFi has been switched off due to timer expiry, it can be re-enabled by holding "7" on the keypad for approx. 2 seconds, in which case the timers are restarted (ie WiFi is again switched off after timer expiry).

Note that if your configured WiFi network was not available when the clock was trying to connect, it will end up in AP-mode. Holding "7" in that case will trigger another attempt to connect to your WiFi network.

## Controlling other props

The device can tell other props about a time travel, and in essence act as a "master controller" for several props. It does so via IO14 (labeled "TT OUT" on Control Boards 1.3 and later), see diagram below.

```
|<---------- speedo acceleration --------->|                         |<-speedo de-acceleration->|
0....10....20....................xx....87..88------------------------88...87....................0
                                           |<--Actual Time Travel -->|
                                           |  (Display disruption)   |
                                      TT starts                      Reentry phase
                                           |                         |
             |<---------ETTO lead--------->|                         |
             |                                                       |
             |                                                       |
             |                                                       |
     IO14: LOW->HIGH                                           IO14: HIGH->LOW
 ```

"ETTO lead", ie the lead time between IO14 going high and the actual start of a time travel is defined as 5000ms (ETTO_LEAD_TIME). In this window of time, the prop can play its pre-time-travel (warm-up/acceleration/etc) sequence. The sequence inside the time "tunnel" follows after that lead time, and when IO14 goes LOW, the re-entry into the destination time takes place.

If external gear is connected to IO14 and you want to use this control feature, check **Use compatible external props** in the Config Portal.

## Flash Wear

Flash memory has a somewhat limited life-time. It can be written to only between 10.000 and 100.000 times before becoming unreliable. The firmware writes to the internal flash memory when saving settings and other data. Every time you change settings through the keypad menu or the Config Portal, data is written to flash memory. The same goes for changing alarm settings (including enabling/disabling the alarm), and time travelling if time travels are [persistent](#persistent--non-persistent-time-travels).

In order to reduce the number of write operations and thereby prolong the life of your clock, it is recommended
- to uncheck the option *[Make time travels persistent](#persistent--non-persistent-time-travels)* in the Config Portal,
- to use a good-quality SD card and to check ["Save alarm/volume settings on SD"](#-save-alarmvolume-settings-on-sd) in the Config Portal; alarm and volume settings are then stored on the SD card (which also suffers from wear but is easy to replace). If you want to swap the SD card but preserve your alarm/volume settings, go to the Config Portal while the old SD card is still in place, uncheck the *Save alarm/volume settings on SD* option, click on Save and wait until the clock has rebooted. You can then power down the clock, swap the SD card and power-up again. Then go to the Config Portal, change the option back on and click on Save. Your settings are now on the new SD card.

## [Appendix: The Config Portal & Sensor Wiring](https://github.com/CircuitSetup/Time-Circuits-Display/wiki/Appendix:-The-Config-Portal-&-Sensor-Wiring)


