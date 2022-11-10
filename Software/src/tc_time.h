/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Time and Main Controller
 *
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

#ifndef _TC_TIME_H
#define _TC_TIME_H

#include "rtc.h"
#include "clockdisplay.h"
#ifdef TC_HAVEGPS
#include "gps.h"
#endif
#ifdef TC_HAVESPEEDO
#include "speeddisplay.h"
#ifdef TC_HAVETEMP
#include "tempsensor.h"
#endif
#endif

#define AUTONM_NUM_PRESETS 4

extern unsigned long  powerupMillis;

extern uint16_t lastYear;

extern bool couldDST;

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;
#ifdef TC_HAVESPEEDO
extern bool useSpeedo;
extern speedDisplay speedo;
#ifdef TC_HAVETEMP
extern bool useTemp;
#endif
#endif

extern tcRTC rtc;

#define NUM_AUTOTIMES 11
extern dateStruct destinationTimes[NUM_AUTOTIMES];
extern dateStruct departedTimes[NUM_AUTOTIMES];
extern int8_t     autoTime;

// These block various events
extern bool FPBUnitIsOn;
extern bool startup;
extern bool timeTraveled;
extern int  timeTravelP0;
extern int  timeTravelP1;
extern int  timeTravelP2;
extern int  specDisp;

// Time Travel difference to RTC
extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern bool timetravelPersistent;

#ifdef FAKE_POWER_ON
extern bool waitForFakePowerButton;
#endif

void time_boot();
void time_setup();
void time_loop();
void timeTravel(bool doComplete, bool withSpeedo = false);
void resetPresentTime();
void pauseAuto();
bool checkIfAutoPaused();

bool      isLeapYear(int year);
int       daysInMonth(int month, int year);
DateTime  myrtcnow();
uint64_t  dateToMins(int year, int month, int day, int hour, int minute);
void      minsToDate(uint64_t total, int& year, int& month, int& day, int& hour, int& minute);
uint32_t  getHrs1KYrs(int index);
uint8_t   dayOfWeek(int d, int m, int y);
void      correctYr4RTC(uint16_t& year, int16_t& offs);

#ifdef FAKE_POWER_ON
void  fpbKeyPressed();
void  fpbKeyLongPressStop();
#endif

void  pwrNeedFullNow(bool force = false);

#ifdef TC_HAVEGPS
bool gpsHaveFix();
void gps_loop();
#endif

bool  parseTZ(char *tz, int currYear, bool doparseDST = true);
int   timeIsDST(int year, int month, int day, int hour, int mins, int& currTimeMins);

void  ntp_loop();
void  ntp_short_loop();

#endif
