# Guía de Tests Unitarios Nativos (`LoRaMeshUI`)

El proyecto incluye una suite de pruebas unitarias con **Unity** que valida la lógica de `LoRaMeshUI` directamente en la PC (`platform = native`, compilado con `g++`) sin necesidad de conectar un `ESP32-C3` ni una pantalla física `SSD1306`.

---

## 1. Arquitectura de Pruebas y *Mocks*

Para aislar el código de producción (`lib/LoRaMeshUI/src/`) de los periféricos reales durante la ejecución en PC, el entorno `[env:native]` inyecta cabeceras *stub/spy* mediante `-I test/mocks`:

| Archivo | Propósito |
| :--- | :--- |
| [platformio.ini](../platformio.ini) | Define el entorno `[env:native]` (`platform = native`, `test_framework = unity`, `lib_compat_mode = off`, `lib_ignore = U8g2`, `-I test/mocks`). |
| [MockEnv.h](../test/mocks/MockEnv.h) | Estado global simulado (`mockEnv()`): reloj `millis()` controlable, niveles digitales de GPIO y espías de `Wire` / `U8g2` (`sendBufferCount`, historial de `sendF`, textos e iconos XBM dibujados). |
| [Arduino.h](../test/mocks/Arduino.h) | Implementa `millis()`, `delay()`, `pinMode()` y `digitalRead()` sobre `MockEnv`. |
| [Wire.h](../test/mocks/Wire.h) | Simula `Wire.begin(sda, scl)` y `Wire.setClock(hz)` registrando pines y frecuencia. |
| [U8g2lib.h](../test/mocks/U8g2lib.h) | Simula `U8G2` y `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`, capturando `sendBuffer()`, `sendF("c", cmd)`, `drawStr()` y `drawXBM()`. |
| [test_loramesh_ui.cpp](../test/test_loramesh_ui/test_loramesh_ui.cpp) | Suite principal de Unity con los **12 casos de prueba**. |

> **Nota:** Los archivos dentro de `test/` nunca se compilan en los entornos del microcontrolador (`esp32-c3-demo-ui` y `esp32-c3-prod`).

---

## 2. Casos de Prueba Verificados (`12` tests)

1. `test_begin_initializes_i2c_and_renders_home` — Verifica la inicialización de `Wire` en `GPIO 8/9` a `400 kHz`, el comando `0x0a6`, el primer envío de buffer y el renderizado inicial de `SCREEN_MESH_HOME`.
2. `test_begin_respects_initWire_false` — Verifica que pasando `initWire = false` se omita `Wire.begin()` cuando otro módulo (ej. `MPU6050`) ya inicializó el bus.
3. `test_sensor_telemetry_auto_triggers_pre_alert` — Valida el salto automático a `SCREEN_PRE_ALERT` (`ALARM_PRE_ALERT_0x01`) al recibir `local_trigger = true` en `setSensorTelemetry()`.
4. `test_network_status_auto_triggers_critical_alarm` — Valida el salto automático a `SCREEN_CRITICAL_ALARM` (`ALARM_CRITICAL_0x02`) al recibir `global_alarm_active = true` en `setNetworkStatus()`.
5. `test_critical_alarm_0x02_has_priority_over_pre_alert_0x01` — Garantiza la prioridad absoluta de `0x02`: una alerta `0x01` posterior no degrada ni interrumpe la Vista 5.
6. `test_clear_alerts_restores_saved_screen_and_normal_polarity` — Comprueba que `clearAlerts()` limpia todas las banderas de alarma, restaura la polaridad normal (`0x0a6`) y vuelve a la vista previa.
7. `test_manual_navigation_respects_allow_all_screens_flag` — Valida que `nextScreen()` cicla solo por las 3 vistas normales en modo producción (`false`) y por las 5 vistas en modo demo (`true`).
8. `test_button_debounce_filters_glitches_and_advances_on_stable_press` — Verifica que el antirrebote en `GPIO 7` (`45 ms`) descarte pulsos cortos (`20 ms`) y avance exactamente una vista en pulsaciones estables.
9. `test_i2c_burst_arbitration_blocks_normal_redraw_but_allows_critical_transition` — Valida que `pauseI2CForSensorBurst(500)` bloquee redibujos normales durante la ráfaga del `MPU6050`, permitiendo únicamente transiciones críticas.
10. `test_hardware_strobe_4hz_toggles_0xA7_0xA6_and_resets_on_exit` — Comprueba la alternancia `0x0a7` / `0x0a6` cada `125 ms` en `SCREEN_CRITICAL_ALARM` y el restablecimiento a `0x0a6` al salir.
11. `test_packet_rx_tx_updates_counts_active_nodes_and_expires_radio_icon` — Verifica el conteo de bits activos en `online_nodes_mask`, contadores `RX`/`TX`, íconos de actividad y expiración a `RADIO_IDLE` tras `800 ms`.
12. `test_custom_screen_renderer_hook` — Valida el registro y renderizado de vistas personalizadas mediante `setCustomScreenRenderer()`.

---

## 3. Cómo Ejecutar la Suite

Requiere tener `g++` instalado en el sistema (Linux) y ejecutar:

```bash
~/.platformio/penv/bin/pio test -e native -v
```
