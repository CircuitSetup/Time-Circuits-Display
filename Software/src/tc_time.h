/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 *
 * Time and Main Controller
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

#ifndef _TC_TIME_H
#define _TC_TIME_H

#include "rtc.h"
#include "clockdisplay.h"
#ifdef TC_HAVEGPS
#include "gps.h"
#endif
#ifdef TC_HAVESPEEDO
#include "speeddisplay.h"
#endif
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
#include "sensors.h"
#endif

#define AUTONM_NUM_PRESETS 4

#define BEEPM2_SECS 30
#define BEEPM3_SECS 60

extern unsigned long powerupMillis;

extern uint16_t lastYear;

extern bool couldDST[3];
extern bool haveWcMode;
extern bool WcHaveTZ1;
extern bool WcHaveTZ2;
extern uint8_t destShowAlt, depShowAlt;

extern bool syncTrigger;
extern bool doAPretry;

extern uint64_t lastAuthTime64;
extern uint64_t millisEpoch;

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;
#ifdef TC_HAVESPEEDO
extern bool useSpeedo;
extern speedDisplay speedo;
#endif
extern bool useTemp;
extern bool haveRcMode;
#ifdef TC_HAVETEMP
extern tempSensor tempSens;
extern bool tempUnit;
#endif
extern bool useLight;
#ifdef TC_HAVELIGHT
extern lightSensor lightSens;
#endif

extern tcRTC rtc;

extern int8_t        manualNightMode;
extern unsigned long manualNMNow;
extern bool          forceReEvalANM;

extern unsigned long ctDown;
extern unsigned long ctDownNow;

#define NUM_AUTOTIMES 11
extern const dateStruct destinationTimes[NUM_AUTOTIMES];
extern const dateStruct departedTimes[NUM_AUTOTIMES];
extern int8_t     autoTime;

// These block various events
extern bool FPBUnitIsOn;
extern bool startup;
extern bool timeTravelRE;
extern int  timeTravelP0;
extern int  timeTravelP1;
extern int  timeTravelP2;
extern int  specDisp;

extern uint8_t  mqttDisp;
#ifdef TC_HAVEMQTT
extern uint8_t  mqttOldDisp;
extern char mqttMsg[256];
extern uint16_t mqttIdx;
extern int16_t  mqttMaxIdx;
extern bool     mqttST;
#endif

extern bool bttfnHaveClients;

// Time Travel difference to RTC
extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern bool timetravelPersistent;

#ifdef FAKE_POWER_ON
extern bool waitForFakePowerButton;
#endif

extern uint8_t beepMode;
extern bool beepTimer;
extern unsigned long beepTimeout;
extern unsigned long beepTimerNow;

void time_boot();
void time_setup();
void time_loop();
void timeTravel(bool doComplete, bool withSpeedo = false);
void resetPresentTime();
void pauseAuto();
bool checkIfAutoPaused();
void endPauseAuto(void);

void enableWcMode(bool onOff);
bool toggleWcMode();
bool isWcMode();

void enableRcMode(bool onOff);
bool toggleRcMode();
bool isRcMode();

void animate(bool withLEDs = false);
void allLampTest();
void allOff();

bool      isLeapYear(int year);
int       daysInMonth(int month, int year);
void      myrtcnow(DateTime& dt);
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

int   mins2Date(int year, int month, int day, int hour, int mins);
bool  parseTZ(int index, int currYear, bool doparseDST = true);
int   getTzDiff();
int   timeIsDST(int index, int year, int month, int day, int hour, int mins, int& currTimeMins);

void  setDatesTimesWC(DateTime& dt);

void  ntp_loop();
void  ntp_short_loop();

#ifdef TC_HAVEBTTFN
int  bttfnNumClients();
bool bttfnGetClientInfo(int c, char **id, uint8_t **ip, uint8_t *type);
void bttfn_loop();
#endif

#endif
