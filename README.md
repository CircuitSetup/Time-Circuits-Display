# Time Circuits Display

![TCD Front](https://raw.githubusercontent.com/CircuitSetup/Time-Circuits-Display/master/Images/tcd_front2.jpg)


This Time Circuits Display has been meticulously reproduced to be as accurate as possible to the one seen in the Delorean Time Machine in the Back to the Future movies. The LED displays are custom made to the correct size for CircuitSetup. This includes the month 14 segment/3 character displays being closer together, and both the 7 & 14 segment displays being 0.6" high by 0.35" wide.

## Kits
[Time Circuits Display kits can be purchased here with or without 3d printed parts.](https://circuitsetup.us/product/complete-time-circuits-display-kit/)

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
- Automatic rotation of times gone to or displayed in the movie (optional rotation intervals, or off)
#### Present Time as a clock
- On-board Real Time Clock (RTC)
- Get accurate time from NTP (network time) when connected to a WiFi network
- Support for time zones and automatic DST
- Alarm function (sounds customizable with an SD card)
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
- Support for playing a custom sound on the hour
- SD card support for custom audio files
- 24-hour clock mode for non-Americans
- Night mode (displays off or dimmed) & ability to automatically turn on/off by Present Time
- Option to make time travels persistent over reboots (dates entered stay there instead of rotating)
- Built-in installer for default audio files in addition to OTA firmware updates

## Basic Operation (v2.0)
#### Entering Dates & Time Traveling
1. Enter a date in the format mmddyyyyhhmm, mmddyyyy. or just hhmm to maintain the current date (hh should be in 24 hour format for PM times)
1. Press the Enter button (the white button under the other keypad lights and above "CLEAR") to enter the date as the Destination Time
1. Hold down "0" (for 2 seconds) on the keypad to time travel
1. Hold down "9" to return to the time you came from

#### The Keypad Menu
The menu is controlled by "pressing" or "holding" the ENTER key on the keypad.

A "press" is shorter than 2 seconds, a "hold" is 2 seconds or longer.
Data entry is done by pressing the keypad's number keys.

The menu is involked by holding the ENTER button.

[For more details on the Keypad Menu, see here.](https://github.com/CircuitSetup/Time-Circuits-Display/wiki/8.-WiFi-Connection-&-TCD-Settings#the-keypad-menu)

#### The Web Config Portal
[See details here on connecting your TCD to your home network](https://github.com/CircuitSetup/Time-Circuits-Display/wiki/8.-WiFi-Connection-&-TCD-Settings)

#### Setting the Present Time
This can be done in several ways:
- Enter a date/time, and time travel to that time (see above)
OR
1. Hold down the Enter button to enter the keypad menu
1. Press the Enter button once to highlight the Present Time display ("RTC" will show in the Destination Time display)
1. Hold down the Enter button to select the Present Time display
1. Press the Enter button to select the field you would like to change for the Date/Time or use the keypad to start entering values
1. To change the month, enter the month's corresponding number
1. Press the Enter button until "SAVE" appears to save and exit the menu
OR
- If [connected to a WiFi network](https://github.com/CircuitSetup/Time-Circuits-Display/wiki/8.-WiFi-Connection-&-TCD-Settings#connecting-to-your-wifi-network), time will be fetched from the internet (NTP server is set in the web config portal)

#### Changing the Destination Time & Last Time Departed
To manually change the Destination Time & Last Time Departed you can either enter shuffle the dates through time traveling or set the dates through the keypad menu:
1. Hold down the Enter button to enter the keypad menu
1. The Destination Time is highlighted first - Press the Enter button 2 more times to highlight the Last Time Departed display 
1. Hold down the Enter button to select the display you wish to change
1. Press the Enter button to select the field you would like to change for the Date/Time or use the keypad to start entering values
1. To change the month, enter the month's corresponding number
1. Press the Enter button until "SAVE" appears to save and exit the menu

By default, the setting **"Make time travels persistent"** in the web config portal is set to yes. This means:
- A user-programmed *Destination Time* is always stored in flash memory, and retrieved from there after a power-loss. It can be programmed through the keypad menu, or ahead of a time travel by typing mmddyyyy/mmddyyyyhhmm/hhmm plus ENTER. In both cases, the time is stored in flash memory and retrieved upon power-on.
- *Last Time Departed* as displayed at any given point is always stored in flash memory, and retrieved upon power-on.
- *Present Time*, be it actual present time or "fake" after time travelling, will continue to run while the device is not powered via the small battery on the control board.

**If time travels are non-persistent:**
- A user-programmed *Destination Time* is only stored to flash memory when programmed through the keypad menu, but not if entered ahead of a time travel (ie outside of the keypad menu, just by typing mmddyyyy/mmddyyyyhhmm/hhmm plus ENTER).
- A user-programmed *Last Time Departed* is only stored to flash memory when programmed through the keypad menu, but not if the result of a time travel.
- *Present Time* is always reset to actual present time upon power-up.

If you want your device to display exactly the same after a power loss, choose persistent (and set the Time-rotation Interval to 0). 

If you want to display your favorite *Destination Time* and *Last Time Departed* upon power-up, and not have time travels overwrite them in flash memory, choose "non-persistent", and program your times through the keypad menu (and set the Time-rotation Interval to 0). Those times will never be overwritten in flash memory by later time travels. Note, however, that the times displayed might actually change due to time travels.

Note that a non-zero Time-rotation Interval will force the device to cycle through the list of pre-programmed times, regardless of your time travel persistence setting. This cycling will, however, paused for 30 minutes if entered a new destination time and/or travelled in time.

#### Setting the Alarm
- Hold ENTER to invoke main menu
- Press ENTER until "ALA-RM" is shown
- Hold ENTER
- Press ENTER to toggle the alarm on and off, hold ENTER to proceed
- Then enter the hour and minutes
- The menu is left automatically after entering the minute. "SAVE" is displayed briefly.

Under normal operation (ie outside of the menu):
- Holding "1" enables the alarm (dot in the Present Time's minute field will light up)
- Holding "2" disables it

Note that the alarm is recurring, ie it rings every day at the programmed time, unless disabled. Also note, as mentioned, that the alarm is by default relative to your actual *Present Time*, not the time displayed (eg after a time travel). It can, however, be configured to be based on the time displayed, in the web Config Portal.
