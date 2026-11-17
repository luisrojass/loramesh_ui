#include <Arduino.h>
#include <LoRaMeshUI.h>
#include "MockDataGenerator.h"

/**
 * @file FiveViewsCarouselDemo.cpp
 * @brief Ejemplo autocontenido que muestra las 5 vistas del Mini-Framework LoRaMeshUI
 *        cambiando de pantalla únicamente al presionar el pulsador en GPIO 7 -> GND.
 *
 * Para compilar y grabar este ejemplo en el ESP32-C3 Super Mini:
 *   ~/.platformio/penv/bin/pio run -e esp32-c3-demo-ui -t upload
 */

LoRaMeshUI ui;
MockDataGenerator demo(ui);

void setup() {
    Serial.begin(115200);

    // 1. Inicializar OLED 0.96" SSD1306 (0x3C) en SDA=GPIO 8, SCL=GPIO 9 a 400 kHz
    ui.begin(LORAMESH_UI_SDA_PIN, LORAMESH_UI_SCL_PIN, 400000UL, true);

    // 2. Configurar pulsador en GPIO 7 conectado a GND (INPUT_PULLUP interno, activeLow = true)
    ui.attachNavigationButton(LORAMESH_UI_BTN_PIN, /*activeLow=*/true, /*debounceMs=*/45);

    // 3. Arrancar el generador de datos de prueba en la Vista 1
    demo.begin();
}

void loop() {
    // 1. Procesa pulsaciones de GPIO 7, refresca el OLED a 10 FPS y el parpadeo HW a 4 Hz
    ui.update();

    // 2. Si el pulsador cambió la vista activa, inyecta los datos de prueba de esa vista
    demo.update();

    // 3. Ceder tiempo a la tarea IDLE de FreeRTOS para evitar busy-loop al 100% de CPU
    delay(5);
}
