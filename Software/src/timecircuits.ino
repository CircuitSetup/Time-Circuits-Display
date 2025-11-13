#include <Arduino.h>
#include <Wire.h>

#include "clockdisplay.h"

constexpr uint8_t DEPARTED_TIME_ADDR = 0x71;
constexpr uint8_t DIM_BUTTON_PIN = 26;  // GPIO where the dim button is connected

constexpr uint8_t kBrightnessLevels[] = {15, 11, 8, 4, 1};
constexpr size_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);
constexpr uint32_t kDimDebounceDelayMs = 50;

uint8_t g_brightnessIndex = 0;

void handleDimmerButton();

clockDisplay departedDisplay(DEPARTED_TIME_ADDR);

void setup()
{
    Wire.begin();

    pinMode(DIM_BUTTON_PIN, INPUT_PULLUP);

    departedDisplay.begin();
    departedDisplay.setBrightnessLevel(kBrightnessLevels[g_brightnessIndex]);
}

void loop()
{
    handleDimmerButton();
    departedDisplay.update();

    delay(10);
}

void handleDimmerButton()
{
    static uint32_t lastDebounceTime = 0;
    static int lastButtonReading = HIGH;
    static int buttonState = HIGH;

    int reading = digitalRead(DIM_BUTTON_PIN);
    if(reading != lastButtonReading) {
        lastDebounceTime = millis();
    }

    uint32_t now = millis();
    if(now - lastDebounceTime > kDimDebounceDelayMs) {
        if(reading != buttonState) {
            buttonState = reading;

            if(buttonState == LOW) {
                g_brightnessIndex = (g_brightnessIndex + 1) % kBrightnessLevelCount;
                departedDisplay.setBrightnessLevel(kBrightnessLevels[g_brightnessIndex]);
            }
        }
    }

    lastButtonReading = reading;
}

