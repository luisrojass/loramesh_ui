#pragma once
#include <U8g2lib.h>
#include "SystemTypes.h"
#include "LoRaMeshUIIcons.h"

/**
 * @file LoRaMeshUIWidgets.h
 * @brief Componentes visuales reutilizables (Widgets) del Mini-Framework de UI.
 *        Permite construir pantallas estilo Meshtastic manteniendo consistencia
 *        tipográfica y geométrica en los 128x64 píxeles.
 */

namespace LoRaMeshUIWidgets {

    /**
     * @brief Dibuja la Barra Superior de Estado fija (128x12 px).
     *        Muestra: ID del nodo (N0X), actividad de radio (↑/↓/⇄),
     *        contador de malla (M:X/Y), estado del Gateway (GW:OK/--),
     *        barras de señal RSSI e ícono gráfico de batería.
     */
    void drawTopStatusBar(U8G2 &u8g2, const MeshNetworkStatus_t &net);

    /**
     * @brief Dibuja la Barra Inferior de Paginación (128x8 px).
     *        Muestra: Indicador de página [ ● ○ ○ ○ ] y etiqueta derecha (ej. "Rx: 2s" o "REPOSO").
     */
    void drawBottomPaginationBar(U8G2 &u8g2, uint8_t activePage, uint8_t totalPages, const char *rightStatusText);

    /**
     * @brief Dibuja las 4 barras escalonadas de calidad de señal según el RSSI (dBm).
     */
    void drawSignalBars(U8G2 &u8g2, uint8_t x, uint8_t y, int16_t rssi);

    /**
     * @brief Dibuja un ícono de batería horizontal con relleno proporcional (0-100%).
     */
    void drawBatteryIcon(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t batteryPct);

    /**
     * @brief Dibuja una fila estandarizada de telemetría con etiqueta, valor y badge [OK]/[ERR].
     */
    void drawSensorMetricRow(U8G2 &u8g2, uint8_t y, const char *label, const char *valueText, bool isOk);

    /**
     * @brief Dibuja una insignia (badge) para representar un nodo o Gateway en el mapa de topología.
     * @param isHighlighted Si es true, invierte el fondo (útil para el nodo propio o nodo votando sismo).
     */
    void drawNodeBadge(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, const char *label, bool isOnline, bool isHighlighted);

    /**
     * @brief Dibuja una barra de progreso horizontal con línea vertical de umbral (ej. para STA/LTA).
     */
    void drawThresholdBar(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t h, float currentVal, float maxVal, float thresholdVal);

    /**
     * @brief Dibuja un encabezado centrado con opción de banda blanca y texto invertido.
     */
    void drawHeaderBanner(U8G2 &u8g2, uint8_t y, uint8_t h, const char *text, bool inverted);

} // namespace LoRaMeshUIWidgets
