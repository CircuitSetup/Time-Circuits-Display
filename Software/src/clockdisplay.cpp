#include "clockdisplay.h"

#include <Wire.h>

#include "tc_font.h"

clockDisplay::clockDisplay(uint8_t address)
    : _address(address), _buffer{0}, _colonVisible(true), _lastToggle(0) {}

void clockDisplay::begin()
{
    sendCommand(kCommandSystemSetup | 0x01);   // Enable oscillator
    setBrightness(15);
    sendCommand(kCommandDisplaySetup | 0x01);  // Display on, no blinking

    clearBuffer();
    _colonVisible = true;
    renderStaticFrame();
    pushBuffer();

    _lastToggle = millis();
}

void clockDisplay::update()
{
    uint32_t now = millis();
    if(now - _lastToggle >= 1000) {
        _lastToggle = now;
        _colonVisible = !_colonVisible;
        applyColon();
        pushBuffer();
    }
}

void clockDisplay::setBrightnessLevel(uint8_t level)
{
    setBrightness(level & 0x0F);
}

void clockDisplay::clearBuffer()
{
    for(uint8_t i = 0; i < kBufferSize; ++i) {
        _buffer[i] = 0;
    }
}

void clockDisplay::renderStaticFrame()
{
    clearBuffer();

    // Month "SEP"
    _buffer[kMonthPos] = alphaSegments('S');
    _buffer[kMonthPos + 1] = alphaSegments('E');
    _buffer[kMonthPos + 2] = alphaSegments('P');

    // Day with PM indicator
    _buffer[kDayPos] = makeNumber(kStaticDay);
    _buffer[kDayPos] |= kPmMask;
    _buffer[kDayPos] &= static_cast<uint16_t>(~kAmMask);

    // Year 1885
    _buffer[kYearPos] = makeNumber(kStaticYear / 100);
    _buffer[kYearPos + 1] = makeNumber(kStaticYear % 100);

    // Time 09:06 PM
    _buffer[kHourPos] = makeNumber(kStaticHour);
    _buffer[kMinutePos] = makeNumber(kStaticMinute);

    enableIndicators();
    applyColon();
}

void clockDisplay::pushBuffer()
{
    Wire.beginTransmission(_address);
    Wire.write(0x00);

    for(uint8_t i = 0; i < kBufferSize; ++i) {
        Wire.write(static_cast<uint8_t>(_buffer[i] & 0xFF));
        Wire.write(static_cast<uint8_t>(_buffer[i] >> 8));
    }

    Wire.endTransmission();
}

void clockDisplay::applyColon()
{
    if(_colonVisible) {
        _buffer[kYearPos] |= kColonMask;
    } else {
        _buffer[kYearPos] &= static_cast<uint16_t>(~kColonMask);
    }
}

void clockDisplay::enableIndicators()
{
    _buffer[kIndicatorPos] |= kIndicatorMask;
}

void clockDisplay::setBrightness(uint8_t level)
{
    sendCommand(kCommandBrightness | (level & 0x0F));
}

void clockDisplay::sendCommand(uint8_t command)
{
    Wire.beginTransmission(_address);
    Wire.write(command);
    Wire.endTransmission();
}

uint16_t clockDisplay::makeNumber(uint8_t value) const
{
    uint8_t tens = value / 10;
    uint8_t ones = value % 10;

    uint16_t segments = digitSegments(tens);
    segments |= static_cast<uint16_t>(digitSegments(ones)) << 8;

    return segments;
}

uint8_t clockDisplay::digitSegments(uint8_t value) const
{
    if(value < 10) {
        return numDigs[(value + '0') - 32];
    }
    return 0;
}

uint16_t clockDisplay::alphaSegments(char value) const
{
    if(value < 32 || value >= 127) {
        return 0;
    }
    return alphaChars[value - 32];
}

