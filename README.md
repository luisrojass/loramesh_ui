# LoRaMeshUI

Mini-framework modular de interfaz gráfica estilo Meshtastic en **C++ (Arduino / PlatformIO)** para nodos de **Alerta Sísmica Temprana** basados en `ESP32-C3 Super Mini` y pantalla **OLED `0.96"` (`SSD1306` $128 \times 64\text{ px}$ por I²C)**.

Diseñado de forma desacoplada para integrarse fácilmente mediante contratos de datos compartidos ([`SystemTypes.h`](./lib/LoRaMeshUI/src/SystemTypes.h)) con el módulo de sensórica sísmica (`MPU6050` + `Geófono`) y el módem LoRa Mesh (`RAK3172` vía UART).

---

## Características Principales

* **Renderizado no bloqueante a `10 FPS`:** Basado en `U8g2` (*Full Buffer*), sin `delay()` internos dentro de la librería.
* **Arquitectura modular por capas:** Separación entre contrato de datos, tema/iconos XBM, *widgets* reutilizables (`LoRaMeshUIWidgets`), plantillas de pantallas (`LoRaMeshUIScreens`) y controlador principal (`LoRaMeshUI`).
* **Arbitraje de bus I²C compartido:** Permite congelar el refresco de pantalla (`pauseI2CForSensorBurst`) para ceder el 100% del bus I²C al acelerómetro `MPU6050` durante ráfagas de muestreo en sospecha de Onda P.
* **Estroboscópico por hardware a `4 Hz`:** En estado de alerta sísmica confirmada (`0x02`), alterna los comandos `0xA7`/`0xA6` directamente en el controlador `SSD1306` sin reenviar el *framebuffer*.
* **Tests unitarios nativos en PC:** Suite con **Unity** y *mocks* de `Arduino`/`Wire`/`U8g2` para validar la lógica sin hardware conectado.

---

## Hardware y Mapa de Pines (`ESP32-C3 Super Mini`)

| Periférico | Pin `ESP32-C3` | Configuración | Notas |
| :--- | :---: | :--- | :--- |
| **OLED `0.96"` `SSD1306` (`0x3C`)** | `SDA = GPIO 8`<br>`SCL = GPIO 9` | I²C Fast Mode (`400 kHz`) | Comparte bus I²C con el `MPU6050` (dirección `0x68`). |
| **Botón de Navegación** | `GPIO 7` | `INPUT_PULLUP` (`GPIO 7` $\rightarrow$ `GND`) | Antirrebote por software (`45 ms`), avanza por las vistas del carrusel. |

---

## Vistas del Sistema

| Vista | Identificador | Descripción |
| :---: | :--- | :--- |
| **1** | `SCREEN_MESH_HOME` | **Estado de la Malla:** último emisor y saltos, `RSSI`, `SNR`, contadores `TX/RX` y tiempo desde el último paquete. |
| **2** | `SCREEN_SEISMIC_MONITOR` | **Monitor Sísmico en Vivo:** lecturas `PGA` (`MPU6050`), `PGV` (`Geófono`), frecuencia dominante y barra de umbral `STA/LTA`. |
| **3** | `SCREEN_TOPOLOGY_MAP` | **Mapa de Topología:** estado del Gateway (`GW:OK`) y nodos vecinos (`N1..N5`), resaltando el nodo local y el resumen de canal LoRa. |
| **4** | `SCREEN_PRE_ALERT` | **Pre-Alerta (`0x01`):** interrupción por sospecha de Onda P mostrando nodos que votan en `confirm_mask` (*piggybacking*). |
| **5** | `SCREEN_CRITICAL_ALARM` | **Alerta Sísmica Confirmada (`0x02`):** toma el 100% de la pantalla con aviso de evacuación, votos, `PGA`, estado de sirena y parpadeo HW `4 Hz`. |

---

## Estructura del Proyecto

```text
loramesh_ui/
├── README.md
├── LICENSE
├── platformio.ini                   # Entornos: esp32-c3-demo-ui, esp32-c3-prod y native
├── docs/
│   ├── api.md                       # Guía de integración (Sensores, RAK3172, Alertas, Widgets)
│   └── testing.md                   # Guía de tests unitarios nativos y mocks
├── lib/
│   └── LoRaMeshUI/
│       ├── library.json
│       ├── src/                     # Código de producción del Mini-Framework
│       └── examples/
│           └── 01_FiveViewsCarouselDemo/
├── src/
│   └── main.cpp                     # Punto de entrada de producción
└── test/                            # Suite Unity + mocks para ejecución en PC
```

---

## Inicio Rápido (PlatformIO)

### 1. Subir el Demo interactivo al `ESP32-C3` *(entorno por defecto)*
Compila el ejemplo [`01_FiveViewsCarouselDemo`](./lib/LoRaMeshUI/examples/01_FiveViewsCarouselDemo/FiveViewsCarouselDemo.cpp) con datos simulados y cambio de vistas mediante el pulsador en `GPIO 7 -> GND`:

```bash
~/.platformio/penv/bin/pio run -e esp32-c3-demo-ui -t upload
```

### 2. Compilar / Subir el entorno de Producción
Compila únicamente [`src/main.cpp`](./src/main.cpp) junto con `lib/LoRaMeshUI/src/` (sin incluir ejemplos):

```bash
~/.platformio/penv/bin/pio run -e esp32-c3-prod -t upload
```

### 3. Ejecutar los Tests Unitarios en PC (sin hardware)
```bash
~/.platformio/penv/bin/pio test -e native -v
```

---

## Documentación Detallada

* **[Guía de Integración y Referencia de API (`docs/api.md`)](./docs/api.md):** Cómo inicializar `LoRaMeshUI`, inyectar lecturas reales de sensores y `RAK3172`, disparar alertas `0x01`/`0x02` y crear pantallas personalizadas con *widgets*.
* **[Guía de Tests Unitarios (`docs/testing.md`)](./docs/testing.md):** Arquitectura de *mocks* en `test/mocks/` y detalle de los 12 casos de prueba verificados con Unity.

---

## Licencia

Distribuido bajo licencia **MIT**. Consulta el archivo [`LICENSE`](./LICENSE) para más información.
