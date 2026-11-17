#include <Arduino.h>
#include <LoRaMeshUI.h>

/**
 * @file main.cpp
 * @brief Firmware principal de producción del nodo sísmico en ESP32-C3 Super Mini.
 *
 * Inicializa el Mini-Framework LoRaMeshUI sobre los pines I2C (SDA=GPIO 8, SCL=GPIO 9)
 * y el botón de navegación (GPIO 7), listo para recibir lecturas reales del módulo
 * de sensores (MPU6050 + Geófono) y del módem LoRa Mesh (RAK3172).
 *
 * Nota: Para probar el carrusel de demostración con datos simulados sin modificar
 * este archivo, compila y sube el entorno `esp32-c3-demo-ui` (ver docs/api.md).
 */

LoRaMeshUI ui;

void setup() {
    Serial.begin(115200);

    // 1. Inicializar pantalla OLED 0.96" (SSD1306 0x3C) en I2C Fast Mode (400 kHz)
    ui.begin(LORAMESH_UI_SDA_PIN, LORAMESH_UI_SCL_PIN, 400000UL, true);

    // 2. Asociar el botón físico en GPIO 7 para navegar entre las vistas normales
    ui.attachNavigationButton(LORAMESH_UI_BTN_PIN, true, 45);
    ui.setAllowManualNavigationAllScreens(false);

    // 3. Configurar la identidad inicial del nodo en la malla
    ui.configureNode(/*myNodeId=*/2, /*totalNodes=*/5, "916.2MHz | SF7 | BW125");
}

void loop() {
    // --- PUNTO DE INTEGRACIÓN CON EQUIPO A (Sensores: MPU6050 + Geófono) ---
    // if (seismicSensor.available()) {
    //     ui.setSensorTelemetry(seismicSensor.read());
    // }

    // --- PUNTO DE INTEGRACIÓN CON EQUIPO B (Módem LoRa Mesh: RAK3172) ---
    // if (rakLink.pollEvent(netStatus)) {
    //     ui.setNetworkStatus(netStatus);
    // }

    // Actualización no bloqueante del Mini-Framework de UI (10 FPS + parpadeo HW 4 Hz)
    ui.update();
}
