# Guía de Integración y Referencia de API: Mini-Framework `LoRaMeshUI`

Esta guía documenta cómo importar y utilizar el **Mini-Framework de Interfaz Gráfica (`LoRaMeshUI`)** dentro del firmware del `ESP32-C3 Super Mini` para mostrar en la pantalla OLED `0.96"` ($128 \times 64\text{ px}$, `SSD1306` por I²C):

1. **Lecturas reales de los sensores sísmicos** (`MPU6050` + `Geófono`).
2. **Telemetría y topología en tiempo real del módem LoRa Mesh** (`RAK3172` vía UART).
3. **Avisos de emergencia sísmica reales**: Pre-Alerta de Onda P (`0x01`) y Alerta General Confirmada (`0x02`).
4. **Estilos y componentes visuales reutilizables (`LoRaMeshUIWidgets`)** para crear o personalizar pantallas.

---

## 1. Estructura del Mini-Framework y Separación de Ejemplos

Siguiendo el estándar de empaquetado de librerías de PlatformIO y Arduino, el código de producción del Mini-Framework vive en `lib/LoRaMeshUI/src/`, mientras que los demos y generadores de datos de prueba (*Mock Data*) se encuentran aislados en `lib/LoRaMeshUI/examples/`:

```text
lib/LoRaMeshUI/
├── library.json                               # Manifiesto de la librería y registro de ejemplos
├── src/                                       # SOLO código de producción del Mini-Framework
│   ├── SystemTypes.h
│   ├── LoRaMeshUIIcons.h
│   ├── LoRaMeshUIWidgets.h / .cpp
│   ├── LoRaMeshUIScreens.h / .cpp
│   └── LoRaMeshUI.h / .cpp
└── examples/                                  # Demos y pruebas visuales (aislados de src/)
    └── 01_FiveViewsCarouselDemo/
        ├── FiveViewsCarouselDemo.cpp          # Ejemplo que avanza entre las 5 vistas al pulsar GPIO 7 -> GND
        └── MockDataGenerator.h                # Datos inventados exclusivos para esta demostración
```

| Archivo | Propósito |
| :--- | :--- |
| [library.json](../lib/LoRaMeshUI/library.json) | Metadatos de la librería `LoRaMeshUI`, dependencia `U8g2` y declaración de ejemplos. |
| [SystemTypes.h](../lib/LoRaMeshUI/src/SystemTypes.h) | Contrato de estructuras compartidas (`SeismicTelemetry_t` y `MeshNetworkStatus_t`) y enumeraciones de estados/pantallas. |
| [LoRaMeshUIIcons.h](../lib/LoRaMeshUI/src/LoRaMeshUIIcons.h) | Constantes geométricas del tema ($128 \times 64\text{ px}$) y bitmaps XBM (`↑`, `↓`, `⇄`, alerta). |
| [LoRaMeshUIWidgets.h](../lib/LoRaMeshUI/src/LoRaMeshUIWidgets.h) | **Capa de Estilos / Widgets reutilizables**: barra superior estilo Meshtastic, barra de paginación inferior, filas de sensores, barras de umbral, insignias de nodos (`[N1:OK]`) y banners invertidos. |
| [LoRaMeshUIScreens.h](../lib/LoRaMeshUI/src/LoRaMeshUIScreens.h) | **Plantillas de las 5 pantallas** armadas con los widgets de `LoRaMeshUIWidgets`. |
| [LoRaMeshUI.h](../lib/LoRaMeshUI/src/LoRaMeshUI.h) | **API Pública principal**: clase `LoRaMeshUI` que administra el ciclo de refresco no bloqueante ($10\text{ FPS}$), arbitraje I²C, botón en `GPIO 7` y parpadeo por hardware a $4\text{ Hz}$. |
| [FiveViewsCarouselDemo.cpp](../lib/LoRaMeshUI/examples/01_FiveViewsCarouselDemo/FiveViewsCarouselDemo.cpp) | **Demo controlado por botón (`GPIO 7 -> GND`)**: avanza manualmente por las 5 vistas con cada pulsación usando [MockDataGenerator.h](../lib/LoRaMeshUI/examples/01_FiveViewsCarouselDemo/MockDataGenerator.h). |

### 1.1 Cómo Compilar y Grabar el Ejemplo vs. Producción (`platformio.ini`)

En [platformio.ini](../platformio.ini) se han definido dos entornos independientes mediante `build_src_filter` para poder compilar y flashear el ejemplo directamente al `ESP32-C3 Super Mini` **sin modificar ni ensuciar [src/main.cpp](../src/main.cpp)**:

* **Entorno de Demostración (`esp32-c3-demo-ui` - Activo por defecto):**
  Compila únicamente [FiveViewsCarouselDemo.cpp](../lib/LoRaMeshUI/examples/01_FiveViewsCarouselDemo/FiveViewsCarouselDemo.cpp) mostrando las 5 vistas controladas por cada pulsación en `GPIO 7 -> GND`:
  ```bash
  ~/.platformio/penv/bin/pio run -e esp32-c3-demo-ui -t upload
  ```
* **Entorno de Producción (`esp32-c3-prod`):**
  Compila únicamente [src/main.cpp](../src/main.cpp) y `lib/LoRaMeshUI/src/` (ignorando por completo la carpeta `examples/`):
  ```bash
  ~/.platformio/penv/bin/pio run -e esp32-c3-prod -t upload
  ```

---

## 2. Importación e Inicialización (Compartiendo el Bus I²C con el `MPU6050`)

En el archivo principal del proyecto (`main.cpp` o el integrador del nodo), importa únicamente `<LoRaMeshUI.h>`:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <LoRaMeshUI.h>

LoRaMeshUI ui;

void setup() {
    // Si el módulo del MPU6050 inicializa Wire antes, puedes pasar initWire = false.
    // Si deseas que LoRaMeshUI inicialice el bus I2C en GPIO 8 (SDA) y GPIO 9 (SCL) a 400 kHz:
    ui.begin(/*sdaPin=*/8, /*sclPin=*/9, /*i2cClockHz=*/400000UL, /*initWire=*/true);

    // Conectar el botón pulsador en GPIO 7 (activo en LOW con INPUT_PULLUP interno)
    ui.attachNavigationButton(/*buttonPin=*/7, /*activeLow=*/true, /*debounceMs=*/45);

    // Configurar el ID de este nodo (ej. Nodo 2 de una red de 5 nodos)
    ui.configureNode(/*myNodeId=*/2, /*totalNodes=*/5, "916.2MHz | SF7 | BW125");

    // En producción, puedes limitar el botón para que solo rote entre las 3 vistas normales
    // y deje las vistas 4 y 5 exclusivas para eventos reales de alarma (o pasar true para pruebas):
    ui.setAllowManualNavigationAllScreens(false);
}

void loop() {
    // Llamar siempre en loop(); NO usa delay() y solo dibuja cuando corresponde (10 FPS)
    ui.update();
}
```

---

## 3. API para Mostrar Datos Reales de los Sensores (`MPU6050` + `Geófono`)

El Equipo de Sensores dispone de **dos formas** para enviar sus lecturas reales a la pantalla:

### Opción A: Usando la estructura contrato `SeismicTelemetry_t` (Recomendada)
Si el módulo `SeismicSensor.h` llena el `struct` compartido en `SystemTypes.h`:

```cpp
SeismicTelemetry_t datosSensor;
datosSensor.pga_mg        = mpu.getPGAMilliG();       // ej. 18 mg
datosSensor.pgv_ums       = geophone.getPGVMicroMS(); // ej. 5 um/s
datosSensor.dominant_freq = fft.getDominantFreqHz();  // ej. 4 Hz
datosSensor.sta_lta_ratio = filter.getStaLtaRatio();  // ej. 1.2f
datosSensor.local_trigger = (datosSensor.sta_lta_ratio >= 3.0f);
datosSensor.mpu_ok        = mpu.isConnected();
datosSensor.geophone_ok   = geophone.isOk();

// Envía todos los datos a la UI.
// Si local_trigger pasa a true, la UI salta sola a la Vista 4 (Pre-Alerta 0x01):
ui.setSensorTelemetry(datosSensor);
```

### Opción B: Usando funciones individuales (Setters directos)
Si prefieres actualizar los valores directamente desde tus variables locales sin armar el `struct`:

```cpp
// 1. Actualizar valores numéricos de aceleración, velocidad, frecuencia y STA/LTA:
ui.updateSeismicValues(pga_mg, pgv_ums, freq_hz, sta_lta_ratio);

// 2. Actualizar el estado [OK] / [ER] de cada sensor:
ui.setSensorHealth(mpu_conectado, geofono_conectado);
```

### Arbitraje del Bus I²C durante Ráfagas del `MPU6050`
Cuando el algoritmo sísmico detecta el inicio de una vibración y necesita leer el `MPU6050` a $100\text{–}200\text{ Hz}$ sin que la pantalla ocupe el bus I²C durante $500\text{ ms}$, invoca:

```cpp
// Congela las transferencias periódicas a la pantalla OLED por 500 ms
// para dejar el 100% del ancho de banda I2C libre al MPU6050:
ui.pauseI2CForSensorBurst(500);
```

---

## 4. API para Mostrar Datos Reales Provenientes del `RAK3172` (LoRa Mesh)

Cada vez que el `ESP32-C3` recibe una trama binaria por `UART1` (`GPIO 20/21`) desde el módem `RAK3172` o transmite un paquete:

### 4.1 Al recibir un paquete desde la malla (`OnRx`)
```cpp
// Actualiza automáticamente:
// - Último nodo emisor y número de saltos (Hops)
// - RSSI (dBm) y barras de señal en la barra superior
// - SNR (dB)
// - Máscara de nodos activos (para la Vista 3 de Topología y el contador M:X/Y)
// - Contador de paquetes RX, reloj "hace Xs" e ícono ↓ en la barra superior
ui.reportPacketRx(sender_id, hops, rssi_dbm, snr_db, nodes_online_mask);
```

### 4.2 Al transmitir o repetir un paquete (`OnTx`)
```cpp
// Si es transmisión propia muestra el ícono ↑; si es retransmisión Mesh muestra ⇄
ui.reportPacketTx(/*isMeshRelay=*/false);
```

### 4.3 Estado del Gateway Central (`Redmi 9`) y Batería
```cpp
ui.setGatewayOnline(true);   // Muestra "GW:OK" en la barra superior y en el mapa de topología
ui.setBatteryLevel(85);      // Actualiza el relleno del ícono de batería (0-100%)
```

*(También puedes actualizar todo el estado de red en un solo paso pasando un `MeshNetworkStatus_t` a `ui.setNetworkStatus(status)`).*

---

## 5. API para Disparar el Aviso de Alarma Real (`0x01` y `0x02`)

El framework implementa dos niveles de interrupción visual con prioridad automática (`0x02` siempre domina sobre `0x01`):

### 5.1 Disparo de Pre-Alerta por Sospecha de Onda P (`Trama 0x01`)
Llámalo cuando el sensor local supere el umbral `STA/LTA` o cuando el `RAK3172` informe que vecinos están votando en `Confirm_Mask`:

```cpp
// Cambia inmediatamente a la VISTA 4 (Pre-Alerta 0x01), enciende el ícono ↑
// y resalta en pantalla las cajas de los nodos cuyos bits estén en 1 dentro de confirm_mask:
ui.triggerPreAlert0x01(confirm_mask, pga_actual_mg);
```

### 5.2 Disparo de ¡ALERTA SÍSMICA CONFIRMADA! (`Trama 0x02` del Gateway)
Llámalo en cuanto el `RAK3172` reciba del aire el paquete maestro `Msg_Type == 0x02`:

```cpp
// 1. Activa tu pin físico de Sirena/MOSFET en el ESP32-C3
digitalWrite(PIN_SIRENA, HIGH);

// 2. Dispara la VISTA CRÍTICA 5 en el OLED:
//    - Toma el 100% de la pantalla con el aviso de evacuación
//    - Muestra cuántos nodos votaron y el PGA máximo
//    - Activa la inversión estroboscópica por hardware (comandos SSD1306 0xA7/0xA6 a 4 Hz)
//      que parpadea toda la pantalla en blanco/negro con CERO carga de CPU y sin tráfico I2C extra.
ui.triggerCriticalAlarm0x02(confirm_mask, pga_max_mg, /*sirenActive=*/true);
```

### 5.3 Restablecer la Pantalla tras finalizar la Alerta
Cuando la emergencia concluye y se apaga la sirena:

```cpp
digitalWrite(PIN_SIRENA, LOW);

// Desactiva el parpadeo inverso (0xA6), limpia las banderas de alarma
// y regresa automáticamente a la pantalla que el usuario tenía abierta antes del sismo:
ui.clearAlerts();
```

---

## 6. Cómo Crear o Personalizar Vistas con los Widgets del Mini-Framework

Si deseas personalizar el diseño de alguna pantalla o crear una vista propia manteniendo el estilo visual de Meshtastic, puedes usar los componentes de [`LoRaMeshUIWidgets`](../lib/LoRaMeshUI/src/LoRaMeshUIWidgets.h) y registrarla con `ui.setCustomScreenRenderer(...)`:

```cpp
#include <LoRaMeshUI.h>

void miVistaPersonalizada(U8G2 &u8g2,
                          const SeismicTelemetry_t &sensor,
                          const MeshNetworkStatus_t &net) {
    // 1. Dibujar la barra superior estándar (ID, flecha LoRa, M:X/Y, GW, RSSI, Batería)
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);

    // 2. Usar widgets de estilo en el área central (Y = 13..54)
    LoRaMeshUIWidgets::drawHeaderBanner(u8g2, 13, 10, "DIAGNOSTICO SENSOR", false);

    char buf[20];
    snprintf(buf, sizeof(buf), "%u mg", sensor.pga_mg);
    LoRaMeshUIWidgets::drawSensorMetricRow(u8g2, 34, "Aceleracion:", buf, sensor.mpu_ok);

    // Barra gráfica con marca de umbral
    LoRaMeshUIWidgets::drawThresholdBar(u8g2, 0, 40, 128, 6, sensor.sta_lta_ratio, 6.0f, 3.0f);

    // 3. Dibujar la barra inferior de paginación
    LoRaMeshUIWidgets::drawBottomPaginationBar(u8g2, 1, 4, "CUSTOM");
}

// En setup():
// Reemplaza la Vista 2 por tu vista personalizada:
ui.setCustomScreenRenderer(SCREEN_SEISMIC_MONITOR, miVistaPersonalizada);
```
