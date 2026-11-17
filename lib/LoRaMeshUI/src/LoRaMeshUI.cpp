#include "LoRaMeshUI.h"

LoRaMeshUI::LoRaMeshUI()
    : _u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE),
      _currentScreen(SCREEN_MESH_HOME),
      _savedNormalScreen(SCREEN_MESH_HOME),
      _alarmLevel(ALARM_NONE),
      _loraChannelSummary("916.2MHz | SF7 | BW125"),
      _lastRenderMs(0),
      _i2cFreezeUntilMs(0),
      _radioIconExpireMs(0),
      _lastBlinkToggleMs(0),
      _displayInverted(false),
      _needsRedraw(true),
      _criticalTransitionPending(false),
      _allowManualAllScreens(true),
      _btnPin(-1),
      _btnActiveLow(true),
      _btnDebounceMs(45),
      _btnLastRawState(false),
      _btnStableState(false),
      _btnLastChangeMs(0) {
    for (uint8_t i = 0; i < SCREEN_COUNT; i++) {
        _customRenderers[i] = nullptr;
    }
}

bool LoRaMeshUI::begin(uint8_t sdaPin, uint8_t sclPin, uint32_t i2cClockHz, bool initWire) {
    if (initWire) {
        Wire.begin(sdaPin, sclPin);
    }
    Wire.setClock(i2cClockHz);
    _u8g2.setBusClock(i2cClockHz);

    if (!_u8g2.begin()) {
        return false;
    }

    _u8g2.clearBuffer();
    _setHardwareInvert(false);
    _needsRedraw = true;
    _renderCurrentScreen(millis());
    return true;
}

void LoRaMeshUI::attachNavigationButton(uint8_t buttonPin, bool activeLow, uint16_t debounceMs) {
    _btnPin = (int8_t)buttonPin;
    _btnActiveLow = activeLow;
    _btnDebounceMs = debounceMs;

    pinMode(_btnPin, activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
    bool rawPressed = (digitalRead(_btnPin) == (activeLow ? LOW : HIGH));
    _btnLastRawState = rawPressed;
    _btnStableState  = rawPressed;
    _btnLastChangeMs = millis();
}

void LoRaMeshUI::update() {
    uint32_t nowMs = millis();

    // 1. Lectura no bloqueante del botón de navegación
    _pollButton(nowMs);

    // 2. Expiración automática del ícono de actividad de radio (TX/RX/Relay -> IDLE tras 800 ms)
    if (_netStatus.radio_state != RADIO_IDLE &&
        (int32_t)(nowMs - _radioIconExpireMs) >= 0) {
        _netStatus.radio_state = RADIO_IDLE;
        _needsRedraw = true;
    }

    // 3. Parpadeo estroboscópico por hardware a 4 Hz (cada 125 ms alterna 0xA7/0xA6)
    //    cuando estamos en la Vista Crítica 5 (Alerta Sísmica Confirmada 0x02).
    if (_currentScreen == SCREEN_CRITICAL_ALARM) {
        if (nowMs - _lastBlinkToggleMs >= 125UL) {
            _lastBlinkToggleMs = nowMs;
            _setHardwareInvert(!_displayInverted);
        }
    } else if (_displayInverted) {
        _setHardwareInvert(false);
    }

    // 4. Arbitraje del Bus I2C: Si el módulo MPU6050 pidió congelar el bus por sospecha
    //    de onda P, cedemos el 100% del canal salvo cuando ocurre una transición de estado.
    bool i2cFrozen = ((int32_t)(_i2cFreezeUntilMs - nowMs) > 0);
    if (i2cFrozen && !_criticalTransitionPending) {
        return;
    }

    // 5. Renderizado a 10 FPS (cada 100 ms) o inmediato ante transición crítica
    if (_criticalTransitionPending || (_needsRedraw && (nowMs - _lastRenderMs >= 100UL)) ||
        (nowMs - _lastRenderMs >= 1000UL)) {
        _lastRenderMs = nowMs;
        _needsRedraw = false;
        _criticalTransitionPending = false;
        _renderCurrentScreen(nowMs);
    }
}

void LoRaMeshUI::setSensorTelemetry(const SeismicTelemetry_t &telemetry) {
    bool prevTrigger = _sensorData.local_trigger;
    _sensorData = telemetry;
    _needsRedraw = true;

    // Si el módulo de sensores levantó local_trigger y no estamos aún en alarma 0x02,
    // disparamos la pantalla de Pre-Alerta (0x01) automáticamente.
    if (_sensorData.local_trigger && !prevTrigger && _alarmLevel == ALARM_NONE) {
        uint8_t myBit = (1U << ((_netStatus.my_node_id > 0 ? _netStatus.my_node_id : 1) - 1));
        triggerPreAlert0x01(_netStatus.confirm_mask | myBit, _sensorData.pga_mg);
    }
}

void LoRaMeshUI::updateSeismicValues(uint16_t pga_mg,
                                 uint16_t pgv_ums,
                                 uint8_t dominant_freq_hz,
                                 float sta_lta_ratio) {
    _sensorData.pga_mg        = pga_mg;
    _sensorData.pgv_ums       = pgv_ums;
    _sensorData.dominant_freq = dominant_freq_hz;
    _sensorData.sta_lta_ratio = sta_lta_ratio;
    _needsRedraw = true;
}

void LoRaMeshUI::setSensorHealth(bool mpuOk, bool geophoneOk) {
    _sensorData.mpu_ok      = mpuOk;
    _sensorData.geophone_ok = geophoneOk;
    _needsRedraw = true;
}

void LoRaMeshUI::pauseI2CForSensorBurst(uint32_t freezeMs) {
    _i2cFreezeUntilMs = millis() + freezeMs;
}

void LoRaMeshUI::setNetworkStatus(const MeshNetworkStatus_t &status) {
    bool prevAlarm = _netStatus.global_alarm_active;
    _netStatus = status;
    _needsRedraw = true;

    if (_netStatus.global_alarm_active && !prevAlarm) {
        triggerCriticalAlarm0x02(_netStatus.confirm_mask, _sensorData.pga_mg, _netStatus.siren_active);
    }
}

void LoRaMeshUI::configureNode(uint8_t myNodeId, uint8_t totalNodes, const char *loraChannelSummary) {
    _netStatus.my_node_id       = myNodeId;
    _netStatus.total_mesh_nodes = totalNodes;
    if (loraChannelSummary != nullptr) {
        _loraChannelSummary = loraChannelSummary;
    }
    _needsRedraw = true;
}

void LoRaMeshUI::reportPacketRx(uint8_t senderId,
                            uint8_t hops,
                            int16_t rssi,
                            int8_t snr,
                            uint8_t onlineNodesMask) {
    _netStatus.last_sender_id    = senderId;
    _netStatus.last_hops         = hops;
    _netStatus.last_rssi         = rssi;
    _netStatus.last_snr          = snr;
    _netStatus.online_nodes_mask = onlineNodesMask;
    _netStatus.active_nodes_count = _countSetBits(onlineNodesMask);
    _netStatus.rx_packets++;
    _netStatus.last_rx_timestamp = millis();
    _netStatus.radio_state       = RADIO_RX;
    _radioIconExpireMs           = _netStatus.last_rx_timestamp + 800UL;
    _needsRedraw = true;
}

void LoRaMeshUI::reportPacketTx(bool isMeshRelay) {
    _netStatus.tx_packets++;
    _netStatus.radio_state = isMeshRelay ? RADIO_MESH_REPEAT : RADIO_TX;
    _radioIconExpireMs     = millis() + 800UL;
    _needsRedraw = true;
}

void LoRaMeshUI::setGatewayOnline(bool online) {
    _netStatus.gateway_online = online;
    _needsRedraw = true;
}

void LoRaMeshUI::setBatteryLevel(uint8_t percent) {
    _netStatus.battery_pct = (percent > 100) ? 100 : percent;
    _needsRedraw = true;
}

void LoRaMeshUI::triggerPreAlert0x01(uint8_t confirmMask, uint16_t currentPgaMg) {
    // La alerta 0x02 tiene prioridad absoluta sobre 0x01 (Regla 7 de sección 3.2)
    if (_alarmLevel == ALARM_CRITICAL_0x02) {
        return;
    }

    if (_alarmLevel == ALARM_NONE && _currentScreen <= SCREEN_TOPOLOGY_MAP) {
        _savedNormalScreen = _currentScreen;
    }

    _alarmLevel               = ALARM_PRE_ALERT_0x01;
    _sensorData.local_trigger = true;
    _sensorData.pga_mg        = currentPgaMg;
    _netStatus.confirm_mask   = confirmMask;
    _netStatus.radio_state    = RADIO_TX;
    _radioIconExpireMs        = millis() + 1200UL;

    _currentScreen             = SCREEN_PRE_ALERT;
    _criticalTransitionPending = true;
    _needsRedraw               = true;
}

void LoRaMeshUI::triggerCriticalAlarm0x02(uint8_t confirmMask, uint16_t maxPgaMg, bool sirenActive) {
    if (_alarmLevel == ALARM_NONE && _currentScreen <= SCREEN_TOPOLOGY_MAP) {
        _savedNormalScreen = _currentScreen;
    }

    _alarmLevel                     = ALARM_CRITICAL_0x02;
    _netStatus.global_alarm_active  = true;
    _netStatus.siren_active         = sirenActive;
    _netStatus.confirm_mask         = confirmMask;
    if (maxPgaMg > _sensorData.pga_mg) {
        _sensorData.pga_mg = maxPgaMg;
    }

    _currentScreen             = SCREEN_CRITICAL_ALARM;
    _lastBlinkToggleMs         = millis();
    _criticalTransitionPending = true;
    _needsRedraw               = true;
}

void LoRaMeshUI::clearAlerts() {
    _alarmLevel                    = ALARM_NONE;
    _sensorData.local_trigger      = false;
    _netStatus.global_alarm_active = false;
    _netStatus.siren_active        = false;
    _netStatus.confirm_mask        = 0x00;

    _setHardwareInvert(false);
    _currentScreen             = _savedNormalScreen;
    _criticalTransitionPending = true;
    _needsRedraw               = true;
}

void LoRaMeshUI::nextScreen() {
    uint8_t limit = _allowManualAllScreens ? (uint8_t)SCREEN_COUNT : 3U;
    uint8_t next  = ((uint8_t)_currentScreen + 1U) % limit;
    showScreen((ScreenId_t)next);
}

void LoRaMeshUI::showScreen(ScreenId_t screen) {
    if (screen >= SCREEN_COUNT) {
        screen = SCREEN_MESH_HOME;
    }
    _currentScreen = screen;
    if (screen <= SCREEN_TOPOLOGY_MAP) {
        _savedNormalScreen = screen;
    }
    if (_currentScreen != SCREEN_CRITICAL_ALARM && _displayInverted) {
        _setHardwareInvert(false);
    }
    _needsRedraw = true;
}

void LoRaMeshUI::setCustomScreenRenderer(ScreenId_t slot, LoRaMeshUIScreens::CustomScreenCallback_t callback) {
    if (slot < SCREEN_COUNT) {
        _customRenderers[slot] = callback;
        _needsRedraw = true;
    }
}

void LoRaMeshUI::_pollButton(uint32_t nowMs) {
    if (_btnPin < 0) return;

    bool rawPressed = (digitalRead(_btnPin) == (_btnActiveLow ? LOW : HIGH));
    if (rawPressed != _btnLastRawState) {
        _btnLastRawState = rawPressed;
        _btnLastChangeMs = nowMs;
    }

    if ((nowMs - _btnLastChangeMs) >= _btnDebounceMs) {
        if (rawPressed != _btnStableState) {
            _btnStableState = rawPressed;
            if (_btnStableState) {
                // Flanco de pulsación confirmado: avanza a la siguiente pantalla
                nextScreen();
            }
        }
    }
}

void LoRaMeshUI::_renderCurrentScreen(uint32_t nowMs) {
    _u8g2.clearBuffer();

    if (_customRenderers[_currentScreen] != nullptr) {
        _customRenderers[_currentScreen](_u8g2, _sensorData, _netStatus);
    } else {
        switch (_currentScreen) {
            case SCREEN_MESH_HOME:
                LoRaMeshUIScreens::renderMeshHome(_u8g2, _netStatus, nowMs);
                break;
            case SCREEN_SEISMIC_MONITOR:
                LoRaMeshUIScreens::renderSeismicMonitor(_u8g2, _sensorData, _netStatus);
                break;
            case SCREEN_TOPOLOGY_MAP:
                LoRaMeshUIScreens::renderTopologyMap(_u8g2, _netStatus, _loraChannelSummary);
                break;
            case SCREEN_PRE_ALERT:
                LoRaMeshUIScreens::renderPreAlert(_u8g2, _sensorData, _netStatus);
                break;
            case SCREEN_CRITICAL_ALARM:
                LoRaMeshUIScreens::renderCriticalAlarm(_u8g2, _sensorData, _netStatus);
                break;
            default:
                LoRaMeshUIScreens::renderMeshHome(_u8g2, _netStatus, nowMs);
                break;
        }
    }

    _u8g2.sendBuffer();
}

void LoRaMeshUI::_setHardwareInvert(bool invert) {
    _displayInverted = invert;
    // Comando directo al controlador SSD1306: 0xA7 = Inverso, 0xA6 = Normal
    // No reenvía los 1024 bytes del buffer, logra parpadeo instantáneo con 0% CPU.
    _u8g2.sendF("c", invert ? 0x0a7 : 0x0a6);
}

uint8_t LoRaMeshUI::_countSetBits(uint8_t mask) {
    uint8_t count = 0;
    while (mask) {
        count += (mask & 0x01);
        mask >>= 1;
    }
    return count;
}
