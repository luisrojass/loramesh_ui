#pragma once
#include <stdint.h>

/**
 * @file LoRaMeshUIIcons.h
 * @brief Constantes de geometría (128x64 px) y bitmaps XBM estilo Meshtastic
 *        para la pantalla OLED 0.96" SSD1306.
 */

namespace LoRaMeshUITheme {
    constexpr uint8_t SCREEN_WIDTH      = 128;
    constexpr uint8_t SCREEN_HEIGHT     = 64;

    // Distribución vertical de las 3 zonas definidas en el documento de arquitectura
    constexpr uint8_t TOP_BAR_HEIGHT    = 12; // 128 x 12 px (Fija superior)
    constexpr uint8_t CONTENT_Y_START   = 13; // Inicio de área central
    constexpr uint8_t CONTENT_HEIGHT    = 43; // 128 x 43 px (Área central dinámica)
    constexpr uint8_t BOTTOM_BAR_Y      = 56; // Inicio barra inferior (56..63)
    constexpr uint8_t BOTTOM_BAR_HEIGHT = 8;  // 128 x 8 px (Paginación y tiempos)
}

namespace LoRaMeshUIIcons {
    // Flecha TX (↑) 7x7 px (Formato XBM LSB-first)
    static const uint8_t ICON_TX_7x7[] = {
        0x08, // ...#...
        0x1C, // ..###..
        0x3E, // .#####.
        0x08, // ...#...
        0x08, // ...#...
        0x08, // ...#...
        0x08  // ...#...
    };

    // Flecha RX (↓) 7x7 px
    static const uint8_t ICON_RX_7x7[] = {
        0x08, // ...#...
        0x08, // ...#...
        0x08, // ...#...
        0x08, // ...#...
        0x3E, // .#####.
        0x1C, // ..###..
        0x08  // ...#...
    };

    // Flechas Mesh Relay (⇄) 7x7 px
    static const uint8_t ICON_RELAY_7x7[] = {
        0x10, // ....#..
        0x3F, // .######
        0x10, // ....#..
        0x00, // .......
        0x04, // ..#....
        0x7E, // ######.
        0x04  // ..#....
    };

    // Ícono de Alerta / Triángulo (!) 8x8 px
    static const uint8_t ICON_WARNING_8x8[] = {
        0x18, // ...##...
        0x3C, // ..####..
        0x24, // ..#..#..
        0x66, // .##..##.
        0x66, // .##..##.
        0x7E, // .######.
        0xE7, // ###..###
        0xFF  // ########
    };
}
