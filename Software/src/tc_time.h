/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
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
#include "speeddisplay.h"
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
#include "sensors.h"
#endif

#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Dash gauges
#define BTTFN_TYPE_VSR     4    // VSR
#define BTTFN_TYPE_AUX     5    // Aux (user custom device)
#define BTTFN_TYPE_REMOTE  6    // Futaba remote control
#define BTTFN_TYPE__MIN    1
#define BTTFN_TYPE__MAX    BTTFN_TYPE_REMOTE

#define AUTONM_NUM_PRESETS 4

#define BEEPM2_SECS 30
#define BEEPM3_SECS 60

#define REM_BRAKE 1900

extern bool showUpdAvail;

extern uint16_t lastYear;

extern bool couldDST[3];
extern bool haveWcMode;
extern bool WcHaveTZ1;
extern bool WcHaveTZ2;
extern uint8_t destShowAlt, depShowAlt;

extern bool syncTrigger;
extern unsigned long syncTriggerNow;
extern bool doAPretry;
extern bool deferredCP;

extern uint64_t lastAuthTime64;

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;

extern uint32_t ttinpin;
extern uint32_t ttoutpin;

extern int           doorSnd;
extern uint32_t      doorFlags;
extern unsigned long doorSndDelay;
extern unsigned long doorSndNow;
extern int           door2Snd;
extern uint32_t      door2Flags;
extern unsigned long door2SndDelay;
extern unsigned long door2SndNow;

extern bool playTTsounds;

extern bool useSpeedo;
extern bool useSpeedoDisplay;
extern speedDisplay speedo;
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
extern bool remoteKPAllowed;
#endif

extern tcRTC rtc;

extern int8_t        manualNightMode;
extern unsigned long manualNMNow;
extern bool          forceReEvalANM;

extern unsigned long ctDown;
extern unsigned long ctDownNow;

extern bool          ETTOcommands;

#define NUM_AUTOTIMES 11
extern const dateStruct destinationTimes[NUM_AUTOTIMES];
extern const dateStruct departedTimes[NUM_AUTOTIMES];
extern int8_t           autoTime;

extern bool       stalePresent;
extern dateStruct stalePresentTime[2];

// These block various events
extern bool FPBUnitIsOn;
extern int  specDisp;
extern int  autoIntAnimRunning;

extern uint32_t csf;
#define CSF_P0  0x01    // Time Travel sequence stages
#define CSF_P1  0x02
#define CSF_RE  0x04
#define CSF_P2  0x08
#define CSF_ST  0x10    // "startup" sequence running
#define CSF_OFF 0x20    // We are fake-off (if bit set)
#define CSF_MA  0x40    // Menu active = busy
#define CSF_NS  0x80    // No scan: Suppress WiFi Scan (so we don't have to abuse any other flag)
#define CSF_NM 0x100    // Night mode

// bttfn_loop() taskMask
#define BNLP_SK_MC      1   // skip MC socket poll
#define BNLP_SK_SP      2   // skip socket poll
#define BNLP_SK_NOTDATA 4   // skip sending out NOT_DATA
#define BNLP_SK_EXPIRE  8   // skip client expiry

extern uint32_t  mqttDisp;
#ifdef TC_HAVEMQTT
extern uint32_t  mqttOldDisp;
extern char *mqttMsg;
extern uint16_t mqttIdx;
extern int16_t  mqttMaxIdx;
extern bool     mqttST;
#endif

extern bool bttfnHaveClients;

// Time Travel difference to RTC
extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern bool timetravelPersistent;

extern bool MQTTPwrMaster; 
extern bool MQTTWaitForOn;

extern uint8_t beepMode;
extern bool beepTimer;
extern unsigned long beepTimeout;
extern unsigned long beepTimerNow;

void      time_boot();
void      time_setup();
void      time_loop();

int       timeTravelProbe(bool doComplete, bool withSpeedo, bool forceNoLead = false);
int       timeTravel(bool doComplete, bool withSpeedo, bool forceNoLead = false);

void      send_refill_msg();
void      send_wakeup_msg();

void      send_abort_msg();

void      setTTOUTpin(uint8_t val);
void      ettoPulseEnd();

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

#ifdef TC_HAVEMQTT
void      mqttFakePowerControl(bool);
void      mqttFakePowerOn();
void      mqttFakePowerOff();
#endif
void      fpbKeyPressed();
void      fpbKeyLongPressStop();

void      myCustomDelay_KP(int iter, unsigned long mydel);

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
void      allShowPattern();
void      allOff();

#ifdef TC_HAVEGPS
bool      gpsHaveFix();
bool      gpsMakePos(char *lat, char *lon);
bool      haveNavMode();
void      enableNavMode(bool onOff);
bool      toggleNavMode();
bool      isNavMode();
int       gpsGetDM();
void      setNavDisplayMode(int dm);
#endif

#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
void      speedoUpdate_loop(bool async);
#endif

void      mydelay(unsigned long mydel);

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

void      ntp_setup(bool doUseNTP, IPAddress& ntpServer, bool couldHaveNTP, bool ntpLUF);
void      ntp_loop();
void      ntp_short_loop();
int       ntp_status();

int       bttfnNumClients();
bool      bttfnGetClientInfo(int c, char **id, uint8_t **ip, uint8_t *type);
bool      bttfn_loop(uint32_t taskMask = 0);
bool      bttfn_loop_ex();
void      bttfn_notify_info();

#ifdef TC_HAVE_REMOTE
void      removeRemote();
void      removeKPRemote();
#endif

#endif
