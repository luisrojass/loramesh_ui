#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "SystemTypes.h"
#include "LoRaMeshUIIcons.h"
#include "LoRaMeshUIWidgets.h"
#include "LoRaMeshUIScreens.h"

/**
 * @file LoRaMeshUI.h
 * @brief Clase principal del Mini-Framework de Interfaz Gráfica (LoRaMeshUI)
 *        para el nodo sísmico ESP32-C3 + OLED 0.96" (SSD1306 I2C).
 *
 * Provee una API desacoplada para:
 *  1. Mostrar lecturas reales del MPU6050 y Geófono.
 *  2. Mostrar telemetría de red P2P recibida por UART desde el RAK3172.
 *  3. Disparar interrupciones visuales para Pre-Alerta (0x01) y Alerta General (0x02).
 *  4. Arbitrar el bus I2C compartido con el acelerómetro MPU6050.
 */

class LoRaMeshUI {
public:
    LoRaMeshUI();

    // =========================================================================
    // 1. INICIALIZACIÓN Y CICLO DE VIDA (NO BLOQUEANTE)
    // =========================================================================

    /**
     * @brief Inicializa la pantalla OLED 0.96" SSD1306 en el bus I2C.
     * @param sdaPin Pin GPIO para SDA (Por defecto GPIO 8 en ESP32-C3 Super Mini).
     * @param sclPin Pin GPIO para SCL (Por defecto GPIO 9 en ESP32-C3 Super Mini).
     * @param i2cClockHz Velocidad del bus I2C (400000 Hz = Fast Mode recomendado).
     * @param initWire Si es true, ejecuta Wire.begin(sdaPin, sclPin) internamente.
     *                 Pasar false si el módulo del MPU6050 ya inicializó Wire antes.
     */
    bool begin(uint8_t sdaPin = 8,
               uint8_t sclPin = 9,
               uint32_t i2cClockHz = 400000UL,
               bool initWire = true);

    /**
     * @brief Configura el botón físico para cambiar de vista manualmente.
     * @param buttonPin Pin GPIO donde está conectado el pulsador (Por defecto GPIO 7).
     * @param activeLow true si el botón cierra a GND (usa INPUT_PULLUP), false si cierra a 3.3V.
     * @param debounceMs Tiempo de antirrebote por software en milisegundos.
     */
    void attachNavigationButton(uint8_t buttonPin = 7,
                                bool activeLow = true,
                                uint16_t debounceMs = 45);

    /**
     * @brief Debe llamarse en cada iteración de loop().
     *        Gestiona el botón, el temporizador de refresco a 10 FPS (100 ms),
     *        el arbitraje I2C y la inversión por hardware a 4 Hz en modo Alerta (0x02).
     */
    void update();

    // =========================================================================
    // 2. API PARA EL MÓDULO DE SENSORES SÍSMICOS (MPU6050 + GEÓFONO)
    // =========================================================================

    /**
     * @brief Actualiza toda la estructura de telemetría sísmica de una sola vez.
     */
    void setSensorTelemetry(const SeismicTelemetry_t &telemetry);

    /**
     * @brief Actualiza directamente las lecturas numéricas de los sensores en tiempo real.
     * @param pga_mg Aceleración pico vectorial del MPU6050 en mili-g.
     * @param pgv_ums Velocidad pico vertical del Geófono en um/s.
     * @param dominant_freq_hz Frecuencia dominante estimada en Hz.
     * @param sta_lta_ratio Ratio STA/LTA actual.
     */
    void updateSeismicValues(uint16_t pga_mg,
                             uint16_t pgv_ums,
                             uint8_t dominant_freq_hz,
                             float sta_lta_ratio);

    /**
     * @brief Reporta el estado operativo de los sensores locales.
     */
    void setSensorHealth(bool mpuOk, bool geophoneOk);

    /**
     * @brief ARBITRAJE DEL BUS I2C: Congela el refresco de la pantalla OLED durante
     *        `freezeMs` milisegundos para ceder el 100% del bus I2C al MPU6050 cuando
     *        se requiere muestreo de ráfaga (100-200 Hz).
     *        Nota: Un cambio crítico de estado (0x01 o 0x02) sí se dibuja una vez.
     */
    void pauseI2CForSensorBurst(uint32_t freezeMs = 500);

    // =========================================================================
    // 3. API PARA EL MÓDULO DE COMUNICACIONES LORA MESH (RAK3172)
    // =========================================================================

    /**
     * @brief Actualiza toda la estructura de estado de red Mesh de una sola vez.
     */
    void setNetworkStatus(const MeshNetworkStatus_t &status);

    /**
     * @brief Configura la identidad del nodo local y el resumen de parámetros LoRa.
     */
    void configureNode(uint8_t myNodeId,
                       uint8_t totalNodes = 5,
                       const char *loraChannelSummary = "916.2MHz | SF7 | BW125");

    /**
     * @brief Notifica a la UI que el RAK3172 recibió un paquete válido por aire.
     *        Actualiza automáticamente RSSI, SNR, último emisor, saltos, máscara de nodos
     *        vecinos, contador RX y muestra el ícono ↓ en la barra superior.
     */
    void reportPacketRx(uint8_t senderId,
                        uint8_t hops,
                        int16_t rssi,
                        int8_t snr,
                        uint8_t onlineNodesMask);

    /**
     * @brief Notifica a la UI que el RAK3172 transmitió un paquete propio (↑)
     *        o retransmitió un paquete de otro nodo en la malla (⇄).
     */
    void reportPacketTx(bool isMeshRelay = false);

    /**
     * @brief Actualiza el indicador de conexión con el Gateway Central (Redmi 9).
     */
    void setGatewayOnline(bool online);

    /**
     * @brief Actualiza el porcentaje de batería mostrado en la barra de estado (0-100%).
     */
    void setBatteryLevel(uint8_t percent);

    // =========================================================================
    // 4. API DE ALERTAS Y EMERGENCIA SÍSMICA (0x01 y 0x02)
    // =========================================================================

    /**
     * @brief Dispara la VISTA 4: Pre-Alerta por Sospecha de Onda P (Trama 0x01).
     *        Cambia automáticamente la pantalla activa a SCREEN_PRE_ALERT.
     * @param confirmMask Máscara de bits de nodos que confirman sismo (Bit 0 = N1...).
     * @param currentPgaMg Valor de PGA actual para mostrar en el aviso de envío.
     */
    void triggerPreAlert0x01(uint8_t confirmMask, uint16_t currentPgaMg);

    /**
     * @brief Dispara la VISTA CRÍTICA 5: ¡Alerta Sísmica Confirmada! (Trama 0x02).
     *        Toma control inmediato de la pantalla y activa el parpadeo estroboscópico
     *        por hardware (0xA7/0xA6) a 4 Hz sin sobrecargar la CPU ni el bus I2C.
     * @param confirmMask Máscara de nodos que votaron a favor del consenso.
     * @param maxPgaMg Aceleración pico máxima registrada (mg).
     * @param sirenActive true si el MOSFET/Sirena local fue activado.
     */
    void triggerCriticalAlarm0x02(uint8_t confirmMask, uint16_t maxPgaMg, bool sirenActive = true);

    /**
     * @brief Finaliza el estado de Pre-Alerta (0x01) o Alerta Crítica (0x02),
     *        restaura los colores normales de la pantalla (0xA6) y vuelve a la vista previa.
     */
    void clearAlerts();

    /**
     * @brief Devuelve el nivel de alarma activo actualmente.
     */
    AlarmLevel_t getAlarmLevel() const { return _alarmLevel; }

    // =========================================================================
    // 5. API DE NAVEGACIÓN Y EXTENSIBILIDAD DE PANTALLAS
    // =========================================================================

    /**
     * @brief Avanza manualmente a la siguiente pantalla del carrusel.
     */
    void nextScreen();

    /**
     * @brief Fija una pantalla específica en el visor.
     */
    void showScreen(ScreenId_t screen);

    /**
     * @brief Obtiene la pantalla actualmente visible.
     */
    ScreenId_t getCurrentScreen() const { return _currentScreen; }

    /**
     * @brief Define si el botón físico puede recorrer las 5 vistas (true, útil para
     *        demostración/pruebas) o únicamente las 3 vistas normales (false, dejando
     *        las vistas 4 y 5 exclusivas para disparos automáticos 0x01/0x02).
     */
    void setAllowManualNavigationAllScreens(bool allowAllFive) {
        _allowManualAllScreens = allowAllFive;
    }

    /**
     * @brief Permite registrar una función de renderizado personalizada para cualquier
     *        slot de pantalla utilizando los componentes de LoRaMeshUIWidgets.
     */
    void setCustomScreenRenderer(ScreenId_t slot, LoRaMeshUIScreens::CustomScreenCallback_t callback);

    /**
     * @brief Solicita forzar un redibujado en el próximo ciclo de update().
     */
    void requestRedraw() { _needsRedraw = true; }

    /**
     * @brief Acceso directo a la instancia U8G2 (por si se requiere dibujo avanzado).
     */
    U8G2 &getDisplay() { return _u8g2; }

    // Lectura directa del estado interno actual
    const SeismicTelemetry_t &getSensorTelemetry() const { return _sensorData; }
    const MeshNetworkStatus_t &getNetworkStatus() const { return _netStatus; }

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2;

    SeismicTelemetry_t  _sensorData;
    MeshNetworkStatus_t _netStatus;

    ScreenId_t   _currentScreen;
    ScreenId_t   _savedNormalScreen;
    AlarmLevel_t _alarmLevel;

    const char *_loraChannelSummary;
    LoRaMeshUIScreens::CustomScreenCallback_t _customRenderers[SCREEN_COUNT];

    // Control de tiempos no bloqueantes (0 delay)
    uint32_t _lastRenderMs;
    uint32_t _i2cFreezeUntilMs;
    uint32_t _radioIconExpireMs;
    uint32_t _lastBlinkToggleMs;
    bool     _displayInverted;
    bool     _needsRedraw;
    bool     _criticalTransitionPending;
    bool     _allowManualAllScreens;

    // Estado del botón de navegación con debounce no bloqueante
    int8_t   _btnPin;
    bool     _btnActiveLow;
    uint16_t _btnDebounceMs;
    bool     _btnLastRawState;
    bool     _btnStableState;
    uint32_t _btnLastChangeMs;

    void _pollButton(uint32_t nowMs);
    void _renderCurrentScreen(uint32_t nowMs);
    void _setHardwareInvert(bool invert);
    static uint8_t _countSetBits(uint8_t mask);
};
