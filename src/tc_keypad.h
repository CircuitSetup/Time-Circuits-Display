#ifndef _TC_KEYPAD_H
#define _TC_KEYPAD_H

#include <Arduino.h>
#include <Keypad.h>
#include <Keypad_I2C.h>

#include "tc_menus.h"
#include "tc_audio.h"

#define KEYPAD_ADDR 0x20
#define BUTTON_HOLD_TIME 3000
#define WHITE_LED 17 //GPIO that white led is connected to
#define ENTER_BUTTON 16 //GPIO that enter key is connected to

void keypadEvent(KeypadEvent key);
extern void keypad_setup();
extern char get_key();
extern boolean isKeyHeld(char key);
void recordKey(char key);
boolean isEnterKeyPressed();
boolean isEnterKeyHeld();
extern void keypadLoop();

extern byte hold;
extern char key;

extern boolean keyPressed;
extern boolean menuFlag;

#endif