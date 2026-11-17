#include "LoRaMeshUIScreens.h"
#include "LoRaMeshUIWidgets.h"
#include <stdio.h>

namespace LoRaMeshUIScreens {

static uint8_t countBits(uint8_t mask) {
    uint8_t count = 0;
    while (mask) {
        count += (mask & 0x01);
        mask >>= 1;
    }
    return count;
}

void renderMeshHome(U8G2 &u8g2, const MeshNetworkStatus_t &net, uint32_t nowMs) {
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);

    u8g2.setFont(u8g2_font_5x7_tf);
    char line[32];

    // Línea 1: Último nodo que transmitió y cantidad de saltos
    if (net.last_sender_id > 0 || net.rx_packets > 0) {
        snprintf(line, sizeof(line), "Ult. Rx: Nodo %02u (%u %s)",
                 net.last_sender_id,
                 net.last_hops,
                 (net.last_hops == 1) ? "salto" : "saltos");
    } else {
        snprintf(line, sizeof(line), "Ult. Rx: Esperando malla...");
    }
    u8g2.drawStr(0, 22, line);

    // Línea 2: Métricas de RF (RSSI y SNR)
    snprintf(line, sizeof(line), "RSSI: %d dBm | SNR: %+d dB",
             (int)net.last_rssi,
             (int)net.last_snr);
    u8g2.drawStr(0, 34, line);

    // Línea 3: Contadores de tráfico P2P
    snprintf(line, sizeof(line), "Pkts TX: %-4u | RX: %-4u",
             net.tx_packets,
             net.rx_packets);
    u8g2.drawStr(0, 46, line);

    // Barra inferior: Tiempo desde el último paquete RX
    char rxAgo[16];
    if (net.rx_packets == 0) {
        snprintf(rxAgo, sizeof(rxAgo), "Rx: --");
    } else {
        uint32_t elapsedSec = (nowMs >= net.last_rx_timestamp)
                                  ? ((nowMs - net.last_rx_timestamp) / 1000UL)
                                  : 0;
        snprintf(rxAgo, sizeof(rxAgo), "hace %lus", (unsigned long)elapsedSec);
    }

    LoRaMeshUIWidgets::drawBottomPaginationBar(u8g2, 0, 4, rxAgo);
}

void renderSeismicMonitor(U8G2 &u8g2,
                          const SeismicTelemetry_t &sensor,
                          const MeshNetworkStatus_t &net) {
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);

    char valBuf[20];

    // Fila 1: Acelerómetro MPU6050 (PGA en mg)
    snprintf(valBuf, sizeof(valBuf), "%4u mg", sensor.pga_mg);
    LoRaMeshUIWidgets::drawSensorMetricRow(u8g2, 21, "PGA (MPU):", valBuf, sensor.mpu_ok);

    // Fila 2: Geófono ADC (PGV en um/s)
    snprintf(valBuf, sizeof(valBuf), "%4u um/s", sensor.pgv_ums);
    LoRaMeshUIWidgets::drawSensorMetricRow(u8g2, 32, "PGV (GEO):", valBuf, sensor.geophone_ok);

    // Fila 3: Ratio STA/LTA y Frecuencia Dominante
    u8g2.setFont(u8g2_font_5x7_tf);
    char line[32];
    snprintf(line, sizeof(line), "STA/LTA:%4.1f | Freq:%2uHz",
             (double)sensor.sta_lta_ratio,
             sensor.dominant_freq);
    u8g2.drawStr(0, 43, line);

    // Barra de nivel STA/LTA (rango 0..6.0, umbral típico en 3.0)
    LoRaMeshUIWidgets::drawThresholdBar(u8g2, 0, 46, 128, 6, sensor.sta_lta_ratio, 6.0f, 3.0f);

    const char *stateLabel = sensor.local_trigger ? "UMBRAL P!" : "EN REPOSO";
    LoRaMeshUIWidgets::drawBottomPaginationBar(u8g2, 1, 4, stateLabel);
}

void renderTopologyMap(U8G2 &u8g2, const MeshNetworkStatus_t &net, const char *loraChannelSummary) {
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);

    // Fila 1 de insignias: [GW:OK] [N1:OK] [N2:YO] [N3:OK]
    char badgeText[10];
    snprintf(badgeText, sizeof(badgeText), "GW:%s", net.gateway_online ? "OK" : "--");
    LoRaMeshUIWidgets::drawNodeBadge(u8g2, 0, 14, 30, badgeText, net.gateway_online, false);

    uint8_t maxNodes = (net.total_mesh_nodes > 5) ? 5 : net.total_mesh_nodes;
    for (uint8_t n = 1; n <= maxNodes; n++) {
        bool isMe = (n == net.my_node_id);
        bool isOnline = isMe || ((net.online_nodes_mask & (1U << (n - 1))) != 0);

        if (isMe) {
            snprintf(badgeText, sizeof(badgeText), "N%u:YO", n);
        } else {
            snprintf(badgeText, sizeof(badgeText), "N%u:%s", n, isOnline ? "OK" : "--");
        }

        uint8_t col = (n <= 3) ? n : (n - 4);
        uint8_t rowY = (n <= 3) ? 14 : 27;
        uint8_t x = col * 32;
        LoRaMeshUIWidgets::drawNodeBadge(u8g2, x, rowY, 30, badgeText, isOnline, isMe);
    }

    // Resumen de parámetros físicos LoRa
    u8g2.setFont(u8g2_font_5x7_tf);
    const char *summary = (loraChannelSummary != nullptr) ? loraChannelSummary : "916.2MHz | SF7 | BW125";
    u8g2.drawStr(0, 48, summary);

    LoRaMeshUIWidgets::drawBottomPaginationBar(u8g2, 2, 4, "TOPOLOGIA");
}

void renderPreAlert(U8G2 &u8g2,
                    const SeismicTelemetry_t &sensor,
                    const MeshNetworkStatus_t &net) {
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);

    // Encabezado invertido de advertencia Onda P
    LoRaMeshUIWidgets::drawHeaderBanner(u8g2, 13, 11, "! ONDA P DETECTADA !", true);

    u8g2.setFont(u8g2_font_5x7_tf);
    char line[32];
    snprintf(line, sizeof(line), "Enviando a GW (PGA:%umg)", sensor.pga_mg);
    u8g2.drawStr(0, 34, line);

    // Mostrar qué nodos están votando en el Confirm_Mask (Piggybacking)
    u8g2.drawStr(0, 45, "Votos:");
    uint8_t x = 34;
    for (uint8_t n = 1; n <= 5; n++) {
        bool voting = ((net.confirm_mask & (1U << (n - 1))) != 0);
        char lbl[6];
        if (voting) {
            snprintf(lbl, sizeof(lbl), "N%u", n);
        } else {
            snprintf(lbl, sizeof(lbl), "--");
        }
        LoRaMeshUIWidgets::drawNodeBadge(u8g2, x, 37, 17, lbl, voting, voting);
        x += 19;
    }

    LoRaMeshUIWidgets::drawBottomPaginationBar(u8g2, 3, 4, "Esp. 0x02...");
}

void renderCriticalAlarm(U8G2 &u8g2,
                         const SeismicTelemetry_t &sensor,
                         const MeshNetworkStatus_t &net) {
    // Marco de emergencia que ocupa los 128x64 px completos
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 0, LoRaMeshUITheme::SCREEN_WIDTH, LoRaMeshUITheme::SCREEN_HEIGHT);
    u8g2.drawFrame(2, 2, LoRaMeshUITheme::SCREEN_WIDTH - 4, LoRaMeshUITheme::SCREEN_HEIGHT - 4);

    // Banda superior destacada
    u8g2.drawBox(4, 4, LoRaMeshUITheme::SCREEN_WIDTH - 8, 13);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_6x10_tf);
    const char *title = "! ALERTA DE SISMO !";
    uint8_t tw = u8g2.getStrWidth(title);
    u8g2.drawStr((LoRaMeshUITheme::SCREEN_WIDTH - tw) / 2, 14, title);

    // Contenido crítico
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x7_tf);
    const char *sub = "ONDAS S EN CAMINO";
    uint8_t sw = u8g2.getStrWidth(sub);
    u8g2.drawStr((LoRaMeshUITheme::SCREEN_WIDTH - sw) / 2, 28, sub);

    char line[32];
    uint8_t votes = countBits(net.confirm_mask);
    if (votes == 0 && net.global_alarm_active) {
        votes = net.active_nodes_count;
    }
    snprintf(line, sizeof(line), "Confirmado (Votos: %u/%u)", votes, net.total_mesh_nodes);
    uint8_t lw = u8g2.getStrWidth(line);
    u8g2.drawStr((LoRaMeshUITheme::SCREEN_WIDTH - lw) / 2, 41, line);

    snprintf(line, sizeof(line), "PGA:%4umg | SIRENA:%s",
             sensor.pga_mg,
             net.siren_active ? "ON" : "OFF");
    lw = u8g2.getStrWidth(line);
    u8g2.drawStr((LoRaMeshUITheme::SCREEN_WIDTH - lw) / 2, 54, line);
}

} // namespace LoRaMeshUIScreens
