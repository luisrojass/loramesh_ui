#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "MockEnv.h"

#define U8G2_R0 0
#define U8X8_PIN_NONE 255

static const uint8_t u8g2_font_5x7_tf[]  = {0};
static const uint8_t u8g2_font_6x10_tf[] = {0};

class U8G2 {
public:
    virtual ~U8G2() {}

    void setBusClock(uint32_t busClock) {
        mockEnv().u8g2BusClockHz = busClock;
    }

    bool begin() {
        return mockEnv().u8g2BeginResult;
    }

    void clearBuffer() {
        mockEnv().clearBufferCount++;
        mockEnv().drawnStrings.clear();
        mockEnv().drawnXBMs.clear();
    }

    void sendBuffer() {
        mockEnv().sendBufferCount++;
    }

    void sendF(const char *fmt, ...) {
        if (fmt == nullptr) return;
        va_list args;
        va_start(args, fmt);
        for (const char *p = fmt; *p != '\0'; ++p) {
            if (*p == 'c' || *p == 'a' || *p == 'd') {
                int val = va_arg(args, int);
                mockEnv().lastSendFCmd = val;
                mockEnv().sendFHistory.push_back(val);
            }
        }
        va_end(args);
    }

    void setDrawColor(uint8_t) {}
    void setFont(const uint8_t *) {}

    uint8_t getStrWidth(const char *s) {
        if (s == nullptr) return 0;
        return (uint8_t)(strlen(s) * 5U);
    }

    void drawStr(int, int, const char *s) {
        if (s != nullptr) {
            mockEnv().drawnStrings.push_back(std::string(s));
        }
    }

    void drawXBM(int, int, int, int, const uint8_t *bitmap) {
        mockEnv().drawnXBMs.push_back(bitmap);
    }

    void drawBox(int, int, int, int) {}
    void drawFrame(int, int, int, int) {}
    void drawHLine(int, int, int) {}
    void drawVLine(int, int, int) {}
    void drawPixel(int, int) {}
    void drawDisc(int, int, int) {}
    void drawCircle(int, int, int) {}
};

class U8G2_SSD1306_128X64_NONAME_F_HW_I2C : public U8G2 {
public:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C(int, int = U8X8_PIN_NONE) {}
};
