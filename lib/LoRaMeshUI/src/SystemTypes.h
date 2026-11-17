#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file SystemTypes.h
 * @brief Contrato de datos compartido entre el Módulo de Sensores Sísmicos,
 *        el Enlace LoRa Mesh (RAK3172) y el Mini-Framework de UI (LoRaMeshUI).
 */

// Actividad actual del radio LoRa para el ícono de la barra de estado
enum RadioActivity_t : uint8_t {
    RADIO_IDLE = 0,        // En escucha continua RX sin tráfico reciente
    RADIO_TX,              // ↑ Transmitiendo trama propia
    RADIO_RX,              // ↓ Recibiendo trama desde la malla
    RADIO_MESH_REPEAT      // ⇄ Repitiendo/enrutando paquete de otro nodo
};

// Nivel de estado/alarma del sistema
enum AlarmLevel_t : uint8_t {
    ALARM_NONE = 0,           // Operación normal en reposo
    ALARM_PRE_ALERT_0x01 = 1, // Sospecha local o vecina de Onda P (Trama 0x01)
    ALARM_CRITICAL_0x02  = 2  // Alerta Sísmica Confirmada por Gateway (Trama 0x02)
};

// Identificadores de las pantallas del Mini-Framework
enum ScreenId_t : uint8_t {
    SCREEN_MESH_HOME       = 0, // Vista 1: Estado de la Malla (Home estilo Meshtastic)
    SCREEN_SEISMIC_MONITOR = 1, // Vista 2: Monitor Sísmico en Vivo (MPU6050 + Geófono)
    SCREEN_TOPOLOGY_MAP    = 2, // Vista 3: Mapa de Nodos Vecinos (Topología de Red)
    SCREEN_PRE_ALERT       = 3, // Vista 4: Pantalla de Sospecha (Pre-Alerta 0x01)
    SCREEN_CRITICAL_ALARM  = 4, // Vista 5: ¡Alerta Sísmica Confirmada! (0x02)
    SCREEN_COUNT           = 5
};

// 1. Estructura que el Equipo de Sensores llena y entrega a la UI:
struct SeismicTelemetry_t {
    uint16_t pga_mg        = 0;     // Aceleración pico vectorial (MPU6050) en mili-g (mg)
    uint16_t pgv_ums       = 0;     // Velocidad pico vertical (Geófono) en um/s
    uint8_t  dominant_freq = 0;     // Frecuencia dominante estimada en Hz (0-50 Hz)
    float    sta_lta_ratio = 1.0f;  // Ratio de disparo STA/LTA
    bool     local_trigger = false; // true si superó el umbral local de onda P
    bool     mpu_ok        = true;  // Estado de salud del acelerómetro I2C
    bool     geophone_ok   = true;  // Estado de salud del canal ADC del geófono
};

// 2. Estructura que el Módulo RAK3172 llena y entrega a la UI:
struct MeshNetworkStatus_t {
    uint8_t  my_node_id          = 1;     // ID de este nodo (1..8)
    uint8_t  total_mesh_nodes    = 5;     // Total de nodos esperados en la malla
    uint8_t  active_nodes_count  = 0;     // Cuántos nodos vecinos vemos activos
    uint8_t  online_nodes_mask   = 0x00;  // Bitmask de nodos online (Bit 0 = N1 ... Bit 7 = N8)
    uint8_t  confirm_mask        = 0x00;  // Qué nodos están votando sismo ahora (Piggybacking)
    int16_t  last_rssi           = -120;  // RSSI del último paquete recibido (dBm)
    int8_t   last_snr            = 0;     // SNR del último paquete recibido (dB)
    uint8_t  last_sender_id      = 0;     // Quién nos habló por última vez
    uint8_t  last_hops           = 0;     // A cuántos saltos llegó el mensaje
    uint16_t tx_packets          = 0;     // Contador de paquetes transmitidos
    uint16_t rx_packets          = 0;     // Contador de paquetes recibidos
    uint8_t  battery_pct         = 100;   // Nivel de batería (0-100%)
    uint32_t last_rx_timestamp   = 0;     // millis() del último paquete recibido
    RadioActivity_t radio_state  = RADIO_IDLE;
    bool     gateway_online      = false; // true si se escuchó el heartbeat del Gateway Redmi 9
    bool     siren_active        = false; // Estado físico del actuador local (Sirena/MOSFET)
    bool     global_alarm_active = false; // ¡TRUE CUANDO LLEGA EL PAQUETE 0x02!
};
