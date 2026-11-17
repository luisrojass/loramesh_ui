#include "LoRaMeshUIWidgets.h"
#include <stdio.h>
#include <string.h>

namespace LoRaMeshUIWidgets {

void drawSignalBars(U8G2 &u8g2, uint8_t x, uint8_t y, int16_t rssi) {
    // Escala de 0 a 4 barras según nivel de RSSI en LoRa SF7
    uint8_t bars = 0;
    if (rssi > -115 && rssi != 0) {
        if (rssi >= -70)      bars = 4;
        else if (rssi >= -85) bars = 3;
        else if (rssi >= -98) bars = 2;
        else                  bars = 1;
    }

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t barH = (i + 1) * 2;
        uint8_t bx   = x + (i * 3);
        uint8_t by   = y + (8 - barH);
        if (i < bars) {
            u8g2.drawBox(bx, by, 2, barH);
        } else {
            u8g2.drawPixel(bx, y + 7);
            u8g2.drawPixel(bx + 1, y + 7);
        }
    }
}

void drawBatteryIcon(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t batteryPct) {
    if (batteryPct > 100) batteryPct = 100;
    // Cuerpo de la batería: 12x7 px + borne positivo de 1x3 px
    u8g2.drawFrame(x, y, 12, 7);
    u8g2.drawBox(x + 12, y + 2, 1, 3);

    uint8_t fillW = (uint8_t)((batteryPct * 8) / 100);
    if (fillW > 0) {
        u8g2.drawBox(x + 2, y + 2, fillW, 3);
    }
}

void drawTopStatusBar(U8G2 &u8g2, const MeshNetworkStatus_t &net) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x7_tf);

    // 1. Izquierda: ID del Nodo (ej. "N02")
    char buf[24];
    snprintf(buf, sizeof(buf), "N%02u", net.my_node_id);
    u8g2.drawStr(0, 8, buf);

    // Ícono de actividad LoRa (TX ↑, RX ↓, Relay ⇄)
    if (net.radio_state == RADIO_TX) {
        u8g2.drawXBM(17, 1, 7, 7, LoRaMeshUIIcons::ICON_TX_7x7);
    } else if (net.radio_state == RADIO_RX) {
        u8g2.drawXBM(17, 1, 7, 7, LoRaMeshUIIcons::ICON_RX_7x7);
    } else if (net.radio_state == RADIO_MESH_REPEAT) {
        u8g2.drawXBM(17, 1, 7, 7, LoRaMeshUIIcons::ICON_RELAY_7x7);
    }

    // 2. Centro: Nodos activos en la malla y estado del Gateway Redmi 9
    snprintf(buf, sizeof(buf), "M:%u/%u GW:%s",
             net.active_nodes_count,
             net.total_mesh_nodes,
             net.gateway_online ? "OK" : "--");
    u8g2.drawStr(28, 8, buf);

    // 3. Derecha: Barras RSSI + Batería gráfica
    drawSignalBars(u8g2, 99, 1, net.last_rssi);
    drawBatteryIcon(u8g2, 114, 1, net.battery_pct);

    // Línea divisoria horizontal bajo la barra de estado (y = 10)
    u8g2.drawHLine(0, 10, LoRaMeshUITheme::SCREEN_WIDTH);
}

void drawBottomPaginationBar(U8G2 &u8g2, uint8_t activePage, uint8_t totalPages, const char *rightStatusText) {
    u8g2.setDrawColor(1);
    u8g2.drawHLine(0, LoRaMeshUITheme::BOTTOM_BAR_Y - 1, LoRaMeshUITheme::SCREEN_WIDTH);

    // Dibuja los círculos de paginación estilo Meshtastic: [ ● ○ ○ ○ ]
    uint8_t startX = 4;
    uint8_t centerY = 60;
    for (uint8_t i = 0; i < totalPages; i++) {
        uint8_t cx = startX + (i * 8);
        if (i == activePage) {
            u8g2.drawDisc(cx, centerY, 2);
        } else {
            u8g2.drawCircle(cx, centerY, 2);
        }
    }

    // Texto de estado a la derecha (ej. "Rx: 3s" o "EN REPOSO")
    if (rightStatusText != nullptr && rightStatusText[0] != '\0') {
        u8g2.setFont(u8g2_font_5x7_tf);
        uint8_t textW = u8g2.getStrWidth(rightStatusText);
        u8g2.drawStr(LoRaMeshUITheme::SCREEN_WIDTH - textW, 63, rightStatusText);
    }
}

void drawSensorMetricRow(U8G2 &u8g2, uint8_t y, const char *label, const char *valueText, bool isOk) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, y, label);
    u8g2.drawStr(52, y, valueText);

    // Badge [OK] o [ER] a la derecha
    const char *statusBadge = isOk ? "[OK]" : "[ER]";
    uint8_t bw = u8g2.getStrWidth(statusBadge);
    u8g2.drawStr(LoRaMeshUITheme::SCREEN_WIDTH - bw, y, statusBadge);
}

void drawNodeBadge(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, const char *label, bool isOnline, bool isHighlighted) {
    constexpr uint8_t h = 11;
    u8g2.setFont(u8g2_font_5x7_tf);

    if (isHighlighted) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(x, y, w, h);
        u8g2.setDrawColor(0);
    } else {
        u8g2.setDrawColor(1);
        if (isOnline) {
            u8g2.drawFrame(x, y, w, h);
        } else {
            // Marco punteado simple para nodos desconectados
            u8g2.drawHLine(x, y, w);
            u8g2.drawHLine(x, y + h - 1, w);
        }
    }

    uint8_t tw = u8g2.getStrWidth(label);
    uint8_t tx = x + (w > tw ? (w - tw) / 2 : 1);
    u8g2.drawStr(tx, y + 8, label);
    u8g2.setDrawColor(1);
}

void drawThresholdBar(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t h, float currentVal, float maxVal, float thresholdVal) {
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x, y, w, h);

    if (maxVal <= 0.0f) return;
    float clamped = currentVal;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > maxVal) clamped = maxVal;

    uint8_t fillW = (uint8_t)((clamped / maxVal) * (w - 2));
    if (fillW > 0) {
        u8g2.drawBox(x + 1, y + 1, fillW, h - 2);
    }

    // Marca vertical en el punto de umbral
    if (thresholdVal > 0.0f && thresholdVal < maxVal) {
        uint8_t tx = x + 1 + (uint8_t)((thresholdVal / maxVal) * (w - 2));
        u8g2.drawVLine(tx, y - 1, h + 2);
    }
}

void drawHeaderBanner(U8G2 &u8g2, uint8_t y, uint8_t h, const char *text, bool inverted) {
    u8g2.setFont(u8g2_font_6x10_tf);
    if (inverted) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, y, LoRaMeshUITheme::SCREEN_WIDTH, h);
        u8g2.setDrawColor(0);
    } else {
        u8g2.setDrawColor(1);
        u8g2.drawFrame(0, y, LoRaMeshUITheme::SCREEN_WIDTH, h);
    }

    uint8_t tw = u8g2.getStrWidth(text);
    uint8_t tx = (LoRaMeshUITheme::SCREEN_WIDTH > tw) ? (LoRaMeshUITheme::SCREEN_WIDTH - tw) / 2 : 0;
    u8g2.drawStr(tx, y + h - 2, text);
    u8g2.setDrawColor(1);
}

} // namespace LoRaMeshUIWidgets
