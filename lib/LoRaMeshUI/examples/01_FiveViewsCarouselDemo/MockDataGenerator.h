#pragma once
#include <Arduino.h>
#include <LoRaMeshUI.h>

/**
 * @file MockDataGenerator.h
 * @brief Generador de telemetría simulada exclusivo para el ejemplo de demostración.
 *
 * Detecta cuando el usuario cambia de vista con el pulsador en GPIO 7 (conectado a GND)
 * e inyecta los datos inventados correspondientes a cada una de las 5 vistas.
 */

class MockDataGenerator {
public:
    explicit MockDataGenerator(LoRaMeshUI &ui)
        : _ui(ui),
          _lastSubTickMs(0),
          _currentStage(0),
          _cycleCount(0),
          _started(false) {}

    /**
     * @brief Carga el estado inicial de prueba y muestra la Vista 1.
     */
    void begin() {
        _ui.setAllowManualNavigationAllScreens(true);
        _ui.configureNode(/*myNodeId=*/2, /*totalNodes=*/5, "916.2MHz | SF7 | BW125");
        _ui.setGatewayOnline(true);
        _ui.setBatteryLevel(86);
        _ui.setSensorHealth(true, true);

        MeshNetworkStatus_t initNet = _ui.getNetworkStatus();
        initNet.my_node_id          = 2;
        initNet.total_mesh_nodes    = 5;
        initNet.active_nodes_count  = 4;
        initNet.online_nodes_mask   = 0b00001111; // N1, N2, N3, N4 activos; N5 inactivo
        initNet.last_sender_id      = 1;
        initNet.last_hops           = 1;
        initNet.last_rssi           = -68;
        initNet.last_snr            = 9;
        initNet.tx_packets          = 142;
        initNet.rx_packets          = 389;
        initNet.battery_pct         = 86;
        initNet.gateway_online      = true;
        initNet.last_rx_timestamp   = millis();
        _ui.setNetworkStatus(initNet);

        _currentStage  = 0;
        _lastSubTickMs = millis();
        _started       = true;

        _applyStage(_currentStage);
    }

    /**
     * @brief Llamar dentro de loop().
     *        Detecta cambios de pantalla accionados por el botón en GPIO 7
     *        y anima sutilmente las lecturas de la vista activa cada 400 ms.
     */
    void update() {
        if (!_started) {
            begin();
        }

        // Si el botón en GPIO 7 cambió la vista activa en LoRaMeshUI, cargamos los datos de esa vista
        uint8_t activeScreen = (uint8_t)_ui.getCurrentScreen();
        if (activeScreen != _currentStage) {
            _currentStage = activeScreen;
            if (_currentStage == 0) {
                _cycleCount++;
            }
            _applyStage(_currentStage);
        }

        // Variación intra-vista cada 400 ms para simular telemetría en vivo sin cambiar de pantalla
        uint32_t nowMs = millis();
        if (nowMs - _lastSubTickMs >= 400UL) {
            _lastSubTickMs = nowMs;
            _animateCurrentStage();
        }
    }

private:
    LoRaMeshUI  &_ui;
    uint32_t _lastSubTickMs;
    uint8_t  _currentStage;
    uint16_t _cycleCount;
    bool     _started;

    void _applyStage(uint8_t stage) {
        switch (stage) {
            case 0: {
                // VISTA 1: ESTADO DE LA MALLA (Home Meshtastic)
                _ui.clearAlerts();
                uint8_t sender = (_cycleCount % 2 == 0) ? 1 : 3;
                uint8_t hops   = (_cycleCount % 2 == 0) ? 1 : 2;
                int16_t rssi   = -66 - (int16_t)((_cycleCount * 3) % 10);
                int8_t  snr    = 8 + (int8_t)(_cycleCount % 3);
                _ui.reportPacketRx(sender, hops, rssi, snr, 0b00001111);
                _ui.updateSeismicValues(12, 4, 2, 1.1f);
                _ui.showScreen(SCREEN_MESH_HOME);
                break;
            }

            case 1: {
                // VISTA 2: MONITOR SÍSMICO EN VIVO (Sensores MPU6050 + Geófono)
                _ui.updateSeismicValues(
                    /*pga_mg=*/14 + (_cycleCount % 5),
                    /*pgv_ums=*/5 + (_cycleCount % 3),
                    /*dominant_freq_hz=*/3,
                    /*sta_lta_ratio=*/1.2f
                );
                _ui.setSensorHealth(true, true);
                _ui.showScreen(SCREEN_SEISMIC_MONITOR);
                break;
            }

            case 2: {
                // VISTA 3: MAPA DE NODOS VECINOS (Topología)
                _ui.reportPacketTx(/*isMeshRelay=*/true);
                _ui.setGatewayOnline(true);
                _ui.showScreen(SCREEN_TOPOLOGY_MAP);
                break;
            }

            case 3: {
                // VISTA 4: PANTALLA DE SOSPECHA (Pre-Alerta Onda P - 0x01)
                _ui.updateSeismicValues(
                    /*pga_mg=*/85 + (_cycleCount % 15),
                    /*pgv_ums=*/42,
                    /*dominant_freq_hz=*/6,
                    /*sta_lta_ratio=*/3.8f
                );
                _ui.triggerPreAlert0x01(/*confirmMask=*/0b00000011, /*currentPgaMg=*/88);
                break;
            }

            case 4: {
                // VISTA 5: ¡ALERTA SÍSMICA CONFIRMADA! (0x02 + Parpadeo HW 4 Hz)
                _ui.updateSeismicValues(
                    /*pga_mg=*/195 + (_cycleCount % 25),
                    /*pgv_ums=*/130,
                    /*dominant_freq_hz=*/4,
                    /*sta_lta_ratio=*/5.6f
                );
                _ui.triggerCriticalAlarm0x02(
                    /*confirmMask=*/0b00001111,
                    /*maxPgaMg=*/195 + (_cycleCount % 25),
                    /*sirenActive=*/true
                );
                break;
            }
        }
    }

    void _animateCurrentStage() {
        uint32_t jitter = (millis() / 400UL) % 5UL;

        if (_currentStage == 1) {
            uint16_t pga = 11 + jitter * 2;
            uint16_t pgv = 4 + jitter;
            float staLta = 1.0f + (jitter * 0.25f);
            _ui.updateSeismicValues(pga, pgv, 2 + (jitter % 3), staLta);
        } else if (_currentStage == 3) {
            uint8_t mask = (jitter >= 2) ? 0b00000111 : 0b00000011;
            _ui.triggerPreAlert0x01(mask, 92 + jitter * 4);
        }
    }
};
