#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

/**
 * @file MockEnv.h
 * @brief Estado simulado y espías (spies) compartidos para ejecutar pruebas
 *        unitarias nativas en PC sin ESP32-C3 ni pantalla física SSD1306.
 */

struct MockEnvState {
    uint32_t nowMs;
    uint8_t  pinLevels[64];
    uint8_t  pinModes[64];

    // Espía de I2C (Wire)
    uint32_t wireBeginCalls;
    int      wireLastSda;
    int      wireLastScl;
    uint32_t wireClockHz;

    // Espía de U8g2 / SSD1306
    bool     u8g2BeginResult;
    uint32_t u8g2BusClockHz;
    uint32_t clearBufferCount;
    uint32_t sendBufferCount;
    int      lastSendFCmd;
    std::vector<int> sendFHistory;
    std::vector<std::string> drawnStrings;
    std::vector<const uint8_t *> drawnXBMs;

    void reset() {
        nowMs = 1000; // Arranca en 1000 ms para probar timestamps relativos fácilmente
        for (size_t i = 0; i < 64; i++) {
            pinLevels[i] = 1; // HIGH por defecto (INPUT_PULLUP sin pulsar)
            pinModes[i]  = 0;
        }
        wireBeginCalls   = 0;
        wireLastSda      = -1;
        wireLastScl      = -1;
        wireClockHz      = 0;
        u8g2BeginResult  = true;
        u8g2BusClockHz   = 0;
        clearBufferCount = 0;
        sendBufferCount  = 0;
        lastSendFCmd     = -1;
        sendFHistory.clear();
        drawnStrings.clear();
        drawnXBMs.clear();
    }

    bool hasDrawnText(const std::string &needle) const {
        for (size_t i = 0; i < drawnStrings.size(); i++) {
            if (drawnStrings[i].find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool hasDrawnXBM(const uint8_t *bitmapPtr) const {
        for (size_t i = 0; i < drawnXBMs.size(); i++) {
            if (drawnXBMs[i] == bitmapPtr) {
                return true;
            }
        }
        return false;
    }
};

inline MockEnvState &mockEnv() {
    static MockEnvState instance;
    return instance;
}
