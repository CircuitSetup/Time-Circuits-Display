#include <Arduino.h>
#include <Wire.h>

#include "clockdisplay.h"

constexpr uint8_t DEPARTED_TIME_ADDR = 0x71;

clockDisplay departedDisplay(DEPARTED_TIME_ADDR);

void setup()
{
    Wire.begin();

    departedDisplay.begin();
}

void loop()
{
    departedDisplay.update();

    delay(10);
}

