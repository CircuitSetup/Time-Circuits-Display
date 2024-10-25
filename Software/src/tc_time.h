/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Time and Main Controller
 *
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
#ifdef TC_HAVE_REMOTE
extern bool remoteAllowed;
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

#ifdef HAVE_STALE_PRESENT
extern bool       stalePresent;
extern dateStruct stalePresentTime[2];
#endif

// These block various events
extern bool FPBUnitIsOn;
extern bool startup;
extern bool timeTravelRE;
extern int timeTravelP0;
extern int timeTravelP1;
extern int timeTravelP2;
extern int specDisp;

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

void      time_boot();
void      time_setup();
void      time_loop();

void      timeTravel(bool doComplete, bool withSpeedo = false, bool forceNoLead = false);

void      send_refill_msg();
void      send_wakeup_msg();

void      bttfnSendFluxCmd(uint32_t payload);
void      bttfnSendSIDCmd(uint32_t payload);
void      bttfnSendPCGCmd(uint32_t payload);
void      bttfnSendVSRCmd(uint32_t payload);
void      bttfnSendAUXCmd(uint32_t payload);
void      bttfnSendRemCmd(uint32_t payload);

void      resetPresentTime();

bool      currentlyOnTimeTravel();
void      updatePresentTime(DateTime& dtu, DateTime& dtl);

void      pauseAuto();
bool      checkIfAutoPaused();
void      endPauseAuto(void);

#ifdef FAKE_POWER_ON
void      fpbKeyPressed();
void      fpbKeyLongPressStop();
#endif

void      myCustomDelay_KP(unsigned long mydel);

void      pwrNeedFullNow(bool force = false);

void      myrtcnow(DateTime& dt);

uint64_t  millis64();

void      enableWcMode(bool onOff);
bool      toggleWcMode();
bool      isWcMode();
void      setDatesTimesWC(DateTime& dt);

void      enableRcMode(bool onOff);
bool      toggleRcMode();
bool      isRcMode();

#ifdef TC_HAVE_RE
void      re_vol_reset();
#endif

void      flushDelayedSave();

void      animate(bool withLEDs = false);
void      allLampTest();
void      allOff();

#ifdef TC_HAVEGPS
bool      gpsHaveFix();
#endif
#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
void      gps_loop(bool withRotEnc = true);
#endif

void      mydelay(unsigned long mydel);
void      waitAudioDone();

uint8_t   dayOfWeek(int d, int m, int y);
int       daysInMonth(int month, int year);
bool      isLeapYear(int year);
uint32_t  getHrs1KYrs(int index);
#ifdef TC_JULIAN_CAL
void      correctNonExistingDate(int year, int month, int& day);
#endif
uint8_t*  e(uint8_t *, uint32_t, int);
void      correctYr4RTC(uint16_t& year, int16_t& offs);
int       mins2Date(int year, int month, int day, int hour, int mins);
bool      parseTZ(int index, int currYear, bool doparseDST = true);
int       timeIsDST(int index, int year, int month, int day, int hour, int mins, int& currTimeMins);
void      UTCtoLocal(DateTime &dtu, DateTime& dtl, int index);
void      LocalToUTC(int& ny, int& nm, int& nd, int& nh, int& nmm, int index);

void      ntp_loop();
void      ntp_short_loop();

int       bttfnNumClients();
bool      bttfnGetClientInfo(int c, char **id, uint8_t **ip, uint8_t *type);
bool      bttfn_loop();

#endif
