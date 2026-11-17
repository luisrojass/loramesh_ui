#pragma once
#include <U8g2lib.h>
#include "SystemTypes.h"

/**
 * @file LoRaMeshUIScreens.h
 * @brief Plantillas de pantallas modulares del Mini-Framework (Vistas 1 a 5).
 *        Cada plantilla toma las estructuras de datos del sistema y las renderiza
 *        empleando los componentes de LoRaMeshUIWidgets.
 */

namespace LoRaMeshUIScreens {

    // Firma para pantallas personalizadas definidas por el usuario
    typedef void (*CustomScreenCallback_t)(U8G2 &u8g2,
                                           const SeismicTelemetry_t &sensor,
                                           const MeshNetworkStatus_t &net);

    /**
     * @brief Vista 1: Estado de la Malla (Home estilo Meshtastic).
     */
    void renderMeshHome(U8G2 &u8g2, const MeshNetworkStatus_t &net, uint32_t nowMs);

    /**
     * @brief Vista 2: Monitor Sísmico en Vivo (MPU6050 + Geófono + STA/LTA).
     */
    void renderSeismicMonitor(U8G2 &u8g2,
                              const SeismicTelemetry_t &sensor,
                              const MeshNetworkStatus_t &net);

    /**
     * @brief Vista 3: Mapa de Nodos Vecinos (Topología de Red y Canal LoRa).
     */
    void renderTopologyMap(U8G2 &u8g2, const MeshNetworkStatus_t &net, const char *loraChannelSummary);

    /**
     * @brief Vista 4: Pantalla de Sospecha Sísmica (Pre-Alerta Onda P - Trama 0x01).
     */
    void renderPreAlert(U8G2 &u8g2,
                        const SeismicTelemetry_t &sensor,
                        const MeshNetworkStatus_t &net);

    /**
     * @brief Vista Crítica 5: ¡Alerta Sísmica Confirmada! (Trama 0x02 del Gateway).
     */
    void renderCriticalAlarm(U8G2 &u8g2,
                             const SeismicTelemetry_t &sensor,
                             const MeshNetworkStatus_t &net);

} // namespace LoRaMeshUIScreens
