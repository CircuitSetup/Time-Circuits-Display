#ifndef _TC_TIME_H
#define _TC_TIME_H

#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "time.h"

#define EEPROM_SIZE 128 //for stored settings
#define SECONDS_IN 0   // SQW Monitor 1Hz from the DS3231
#define STATUS_LED 2  // Status LED

#define DEST_TIME_ADDR 0x71
#define DEST_TIME_EEPROM 0x10  
#define PRES_TIME_ADDR 0x72
#define PRES_TIME_EEPROM 0x18
#define DEPT_TIME_ADDR 0x74
#define DEPT_TIME_EEPROM 0x20

#define AUTOINTERVAL_ADDR 0x00  // eeprom autoInterval save location
extern uint8_t autoInterval;
extern const uint8_t autoTimeIntervals[5];

extern clockDisplay destinationTime;
extern clockDisplay presentTime;
extern clockDisplay departedTime;

extern RTC_DS3231 rtc;

extern dateStruct destinationTimes[8];
extern dateStruct departedTimes[8];
extern int8_t autoTime;

extern void time_setup();
extern void time_loop();
extern bool getNTPTime();
extern bool connectToWifi();
extern void doGetAutoTimes();
extern bool checkTimeOut();
extern void RTCClockOutEnable();
extern bool isLeapYear(int year);
extern int daysInMonth(int month, int year);

#endif