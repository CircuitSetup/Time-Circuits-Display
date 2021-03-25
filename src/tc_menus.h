#ifndef _TC_MENUS_H
#define _TC_MENUS_H

#include <Arduino.h>
#include <Wire.h>

#include "clockdisplay.h"
#include "tc_keypad.h"
#include "tc_time.h"

#define buttonDown 16          // Button SET MENU'
#define buttonUp 17            // Button +
//#define ENTER_BUTTON 15           // Button -
#define maxTime 240            // maximum time in seconds before timout during setting and no button pressed, max 255 seconds

extern uint8_t timeout;

extern void menu_setup();
extern void enter_menu();
void displayHighlight(int& number);
void displaySelect(int& number);
void setUpdate(uint16_t& number, int field);
void brtButtonUpDown(clockDisplay* displaySet);
void buttonUpDown(uint16_t& number, uint8_t field, int year, int month);
bool loadAutoInterval();
void saveAutoInterval();
void autoTimesButtonUpDown();
void doGetBrightness(clockDisplay* displaySet);
void waitForButtonSetRelease();

extern void animate();
extern void allLampTest();
extern void allOff();

#endif