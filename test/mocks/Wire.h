#pragma once
#include <stdint.h>
#include "MockEnv.h"

class TwoWireMock {
public:
    bool begin(int sda = -1, int scl = -1) {
        mockEnv().wireBeginCalls++;
        mockEnv().wireLastSda = sda;
        mockEnv().wireLastScl = scl;
        return true;
    }

    void setClock(uint32_t frequency) {
        mockEnv().wireClockHz = frequency;
    }
};

inline TwoWireMock &getMockWireInstance() {
    static TwoWireMock instance;
    return instance;
}

#define Wire getMockWireInstance()
