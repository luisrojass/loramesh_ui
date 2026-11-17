#pragma once
#include <stdint.h>
#include "MockEnv.h"

#ifndef LOW
#define LOW 0x0
#endif
#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef INPUT
#define INPUT 0x01
#endif
#ifndef OUTPUT
#define OUTPUT 0x03
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x05
#endif
#ifndef INPUT_PULLDOWN
#define INPUT_PULLDOWN 0x09
#endif

inline uint32_t millis() {
    return mockEnv().nowMs;
}

inline void delay(uint32_t ms) {
    mockEnv().nowMs += ms;
}

inline void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 64) {
        mockEnv().pinModes[pin] = mode;
    }
}

inline int digitalRead(uint8_t pin) {
    if (pin < 64) {
        return mockEnv().pinLevels[pin];
    }
    return HIGH;
}
