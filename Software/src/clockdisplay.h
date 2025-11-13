#pragma once

#include <Arduino.h>

class clockDisplay {
public:
    explicit clockDisplay(uint8_t address);

    void begin();
    void update();
    void setBrightnessLevel(uint8_t level);

private:
    static constexpr uint8_t kBufferSize = 8;
    static constexpr uint8_t kStaticMonth = 9;   // September
    static constexpr uint8_t kStaticDay = 7;
    static constexpr uint16_t kStaticYear = 1885;
    static constexpr uint8_t kStaticHour = 9;    // 09:06 PM
    static constexpr uint8_t kStaticMinute = 0;

    static constexpr uint8_t kCommandSystemSetup = 0x20;
    static constexpr uint8_t kCommandDisplaySetup = 0x80;
    static constexpr uint8_t kCommandBrightness = 0xE0;

    static constexpr uint8_t kMonthPos = 0;
    static constexpr uint8_t kDayPos = 3;
    static constexpr uint8_t kYearPos = 4;
    static constexpr uint8_t kHourPos = 6;
    static constexpr uint8_t kMinutePos = 7;
    static constexpr uint8_t kIndicatorPos = 1;

    static constexpr uint16_t kAmMask = 0x0080;
    static constexpr uint16_t kPmMask = 0x8000;
    static constexpr uint16_t kColonMask = 0x8080;
    static constexpr uint16_t kIndicatorMask = 0x0007;

    void clearBuffer();
    void renderStaticFrame();
    void pushBuffer();
    void applyColon();
    void enableIndicators();
    void setBrightness(uint8_t level);
    void sendCommand(uint8_t command);

    uint16_t makeNumber(uint8_t value) const;
    uint8_t digitSegments(uint8_t value) const;
    uint16_t alphaSegments(char value) const;

    uint8_t _address;
    uint16_t _buffer[kBufferSize];
    bool _colonVisible;
    uint32_t _lastToggle;
};

