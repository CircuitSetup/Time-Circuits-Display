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
 */


#ifndef _TC_GLOBAL_H
#define _TC_GLOBAL_H

//#define TC_DBG              // debug output on Serial

// Fake Power On:
// Attach an active-low switch to io13 or io14; firmware will start network and 
// sync time, but not enable displays until the switch is activated.
// The white led will flash for 0.5 seconds when the unit is ready to be "fake"
// powered on. De-activating the switch "fake" powers down the device (ie the
// displays are switched off, and no keypad input is accepted)
#define FAKE_POWER_ON         // Wait for button press on io13/io14 before starting displays
#define FAKE_POWER_BUTTON 13  // GPIO that fake power switch is connected to; 13 or 14

// External time travel
// If the defined pin goes low a time travel is triggered (eg when 88mph is reached)
// ATTN: We only simulate re-entry, ie the part of time travel that takes
// place in the new time. So the external time travel should be initiated when
// the present-time part of time travel is over.
#define EXTERNAL_TIMETRAVEL         // Initiate time travel is pin is activated
#define EXTERNAL_TIMETRAVEL_PIN 14  // GPIO that is polled (13 or 14, see fake power)

// Uncomment if month is 2 digits (7-seg), as in the original A-Car display.
//#define IS_ACAR_DISPLAY 

// EEPROM map
// We use 1(padded to 8) + 10*3 + 4 bytes of EEPROM space at 0x0. 
#define AUTOINTERVAL_PREF 0x00    // autoInterval save location   (1 byte, padded 8; unused)
#define DEST_TIME_PREF    0x08    // destination time prefs       (10 bytes)
#define PRES_TIME_PREF    0x12    // present time prefs           (10 bytes)
#define DEPT_TIME_PREF    0x1c    // departure time prefs         (10 bytes)
#define ALARM_PREF        0x26    // alarm prefs                  (4 bytes; only used if fs unavailable)


//#define TWPRIVATE       // A10001986's private customizations

#endif
