#include <unity.h>
#include "MockEnv.h"
#include <LoRaMeshUI.h>

void setUp(void) {
    mockEnv().reset();
}

void tearDown(void) {}

void test_begin_initializes_i2c_and_renders_home(void) {
    LoRaMeshUI ui;
    bool ok = ui.begin(/*sda=*/8, /*scl=*/9, /*clock=*/400000UL, /*initWire=*/true);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(1, mockEnv().wireBeginCalls);
    TEST_ASSERT_EQUAL_INT(8, mockEnv().wireLastSda);
    TEST_ASSERT_EQUAL_INT(9, mockEnv().wireLastScl);
    TEST_ASSERT_EQUAL_UINT32(400000UL, mockEnv().wireClockHz);
    TEST_ASSERT_EQUAL_UINT32(400000UL, mockEnv().u8g2BusClockHz);
    TEST_ASSERT_EQUAL_UINT32(1, mockEnv().sendBufferCount);
    TEST_ASSERT_EQUAL_INT(0x0a6, mockEnv().lastSendFCmd);
    TEST_ASSERT_EQUAL(SCREEN_MESH_HOME, ui.getCurrentScreen());
    TEST_ASSERT_TRUE(mockEnv().hasDrawnText("Ult. Rx:"));
}

void test_begin_respects_initWire_false(void) {
    LoRaMeshUI ui;
    bool ok = ui.begin(8, 9, 400000UL, /*initWire=*/false);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(0, mockEnv().wireBeginCalls);
    TEST_ASSERT_EQUAL_UINT32(400000UL, mockEnv().wireClockHz);
}

void test_sensor_telemetry_auto_triggers_pre_alert(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    ui.configureNode(/*myNodeId=*/2, /*totalNodes=*/5);

    SeismicTelemetry_t data;
    data.pga_mg        = 95;
    data.pgv_ums       = 48;
    data.dominant_freq = 5;
    data.sta_lta_ratio = 3.9f;
    data.local_trigger = true;

    ui.setSensorTelemetry(data);
    mockEnv().nowMs += 100;
    ui.update();

    TEST_ASSERT_EQUAL(ALARM_PRE_ALERT_0x01, ui.getAlarmLevel());
    TEST_ASSERT_EQUAL(SCREEN_PRE_ALERT, ui.getCurrentScreen());
    TEST_ASSERT_EQUAL_UINT8(0b00000010, ui.getNetworkStatus().confirm_mask);
    TEST_ASSERT_TRUE(mockEnv().hasDrawnText("ONDA P DETECTADA"));
}

void test_network_status_auto_triggers_critical_alarm(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);

    MeshNetworkStatus_t net;
    net.my_node_id          = 2;
    net.total_mesh_nodes    = 5;
    net.confirm_mask        = 0b00001111;
    net.siren_active        = true;
    net.global_alarm_active = true;

    ui.setNetworkStatus(net);
    ui.update();

    TEST_ASSERT_EQUAL(ALARM_CRITICAL_0x02, ui.getAlarmLevel());
    TEST_ASSERT_EQUAL(SCREEN_CRITICAL_ALARM, ui.getCurrentScreen());
    TEST_ASSERT_TRUE(mockEnv().hasDrawnText("ALERTA DE SISMO"));
    TEST_ASSERT_TRUE(mockEnv().hasDrawnText("SIRENA:ON"));
}

void test_critical_alarm_0x02_has_priority_over_pre_alert_0x01(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);

    ui.triggerCriticalAlarm0x02(0b00001111, 220, true);
    TEST_ASSERT_EQUAL(ALARM_CRITICAL_0x02, ui.getAlarmLevel());
    TEST_ASSERT_EQUAL(SCREEN_CRITICAL_ALARM, ui.getCurrentScreen());

    // Regla 7: 0x01 nunca debe sobreescribir ni degradar 0x02
    ui.triggerPreAlert0x01(0b00000001, 60);
    TEST_ASSERT_EQUAL(ALARM_CRITICAL_0x02, ui.getAlarmLevel());
    TEST_ASSERT_EQUAL(SCREEN_CRITICAL_ALARM, ui.getCurrentScreen());
}

void test_clear_alerts_restores_saved_screen_and_normal_polarity(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    ui.showScreen(SCREEN_SEISMIC_MONITOR);
    mockEnv().nowMs += 150;
    ui.update();

    ui.triggerCriticalAlarm0x02(0b00000111, 180, true);
    ui.update();
    mockEnv().nowMs += 130;
    ui.update(); // Invierte pantalla (0x0a7)

    ui.clearAlerts();
    ui.update();

    TEST_ASSERT_EQUAL(ALARM_NONE, ui.getAlarmLevel());
    TEST_ASSERT_EQUAL(SCREEN_SEISMIC_MONITOR, ui.getCurrentScreen());
    TEST_ASSERT_FALSE(ui.getSensorTelemetry().local_trigger);
    TEST_ASSERT_FALSE(ui.getNetworkStatus().global_alarm_active);
    TEST_ASSERT_FALSE(ui.getNetworkStatus().siren_active);
    TEST_ASSERT_EQUAL_UINT8(0x00, ui.getNetworkStatus().confirm_mask);
    TEST_ASSERT_EQUAL_INT(0x0a6, mockEnv().lastSendFCmd);
}

void test_manual_navigation_respects_allow_all_screens_flag(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);

    // Modo producción: solo rota vistas normales (0 -> 1 -> 2 -> 0)
    ui.setAllowManualNavigationAllScreens(false);
    TEST_ASSERT_EQUAL(SCREEN_MESH_HOME, ui.getCurrentScreen());
    ui.nextScreen();
    TEST_ASSERT_EQUAL(SCREEN_SEISMIC_MONITOR, ui.getCurrentScreen());
    ui.nextScreen();
    TEST_ASSERT_EQUAL(SCREEN_TOPOLOGY_MAP, ui.getCurrentScreen());
    ui.nextScreen();
    TEST_ASSERT_EQUAL(SCREEN_MESH_HOME, ui.getCurrentScreen());

    // Modo demo: rota las 5 vistas (0 -> 1 -> 2 -> 3 -> 4 -> 0)
    ui.setAllowManualNavigationAllScreens(true);
    ui.nextScreen(); // 1
    ui.nextScreen(); // 2
    ui.nextScreen(); // 3
    TEST_ASSERT_EQUAL(SCREEN_PRE_ALERT, ui.getCurrentScreen());
    ui.nextScreen(); // 4
    TEST_ASSERT_EQUAL(SCREEN_CRITICAL_ALARM, ui.getCurrentScreen());
    ui.nextScreen(); // 0
    TEST_ASSERT_EQUAL(SCREEN_MESH_HOME, ui.getCurrentScreen());
}

void test_button_debounce_filters_glitches_and_advances_on_stable_press(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    ui.attachNavigationButton(/*pin=*/7, /*activeLow=*/true, /*debounceMs=*/45);

    // Rebote corto de 20 ms: no debe cambiar de pantalla
    mockEnv().pinLevels[7] = LOW;
    ui.update();
    mockEnv().nowMs += 20;
    ui.update();
    mockEnv().pinLevels[7] = HIGH;
    ui.update();
    mockEnv().nowMs += 50;
    ui.update();
    TEST_ASSERT_EQUAL(SCREEN_MESH_HOME, ui.getCurrentScreen());

    // Pulsación real estable >= 45 ms: debe avanzar exactamente 1 vista
    mockEnv().pinLevels[7] = LOW;
    ui.update();
    mockEnv().nowMs += 45;
    ui.update();
    TEST_ASSERT_EQUAL(SCREEN_SEISMIC_MONITOR, ui.getCurrentScreen());

    // Mantener presionado más tiempo no debe disparar múltiples avances
    mockEnv().nowMs += 200;
    ui.update();
    TEST_ASSERT_EQUAL(SCREEN_SEISMIC_MONITOR, ui.getCurrentScreen());
}

void test_i2c_burst_arbitration_blocks_normal_redraw_but_allows_critical_transition(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    uint32_t initialSends = mockEnv().sendBufferCount;

    // Congelar bus I2C 500 ms para ráfaga de MPU6050
    ui.pauseI2CForSensorBurst(500);
    ui.updateSeismicValues(15, 6, 3, 1.4f);

    // Dentro de los 500 ms, un refresco normal NO debe enviar nada por I2C
    mockEnv().nowMs += 200;
    ui.update();
    TEST_ASSERT_EQUAL_UINT32(initialSends, mockEnv().sendBufferCount);

    // Pero una transición crítica (0x02) sí debe dibujarse inmediatamente una vez
    ui.triggerCriticalAlarm0x02(0b00000011, 200, true);
    ui.update();
    TEST_ASSERT_EQUAL_UINT32(initialSends + 1, mockEnv().sendBufferCount);

    // Tras expirar los 500 ms, vuelve a refrescar normalmente
    ui.clearAlerts();
    mockEnv().nowMs += 400; // Total +600 ms desde pauseI2CForSensorBurst
    ui.update();
    TEST_ASSERT_GREATER_THAN_UINT32(initialSends + 1, mockEnv().sendBufferCount);
}

void test_hardware_strobe_4hz_toggles_0xA7_0xA6_and_resets_on_exit(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    ui.triggerCriticalAlarm0x02(0b00000101, 175, true);
    ui.update();

    mockEnv().sendFHistory.clear();

    // Primer tick >= 125 ms -> invierte (0xA7)
    mockEnv().nowMs += 125;
    ui.update();
    TEST_ASSERT_EQUAL_INT(0x0a7, mockEnv().lastSendFCmd);

    // Segundo tick >= 125 ms -> vuelve a normal (0xA6)
    mockEnv().nowMs += 125;
    ui.update();
    TEST_ASSERT_EQUAL_INT(0x0a6, mockEnv().lastSendFCmd);

    // Tercer tick >= 125 ms -> invierte otra vez (0xA7), luego salir de Vista 5 debe forzar 0xA6
    mockEnv().nowMs += 125;
    ui.update();
    TEST_ASSERT_EQUAL_INT(0x0a7, mockEnv().lastSendFCmd);

    ui.showScreen(SCREEN_MESH_HOME);
    TEST_ASSERT_EQUAL_INT(0x0a6, mockEnv().lastSendFCmd);
}

void test_packet_rx_tx_updates_counts_active_nodes_and_expires_radio_icon(void) {
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);

    ui.reportPacketRx(/*senderId=*/3, /*hops=*/2, /*rssi=*/-72, /*snr=*/8, /*mask=*/0b00010111);
    mockEnv().nowMs += 100;
    ui.update();

    const MeshNetworkStatus_t &net = ui.getNetworkStatus();
    TEST_ASSERT_EQUAL_UINT8(3, net.last_sender_id);
    TEST_ASSERT_EQUAL_UINT8(2, net.last_hops);
    TEST_ASSERT_EQUAL_INT16(-72, net.last_rssi);
    TEST_ASSERT_EQUAL_UINT8(4, net.active_nodes_count);
    TEST_ASSERT_EQUAL_UINT16(1, net.rx_packets);
    TEST_ASSERT_EQUAL(RADIO_RX, net.radio_state);
    TEST_ASSERT_GREATER_THAN_UINT32(0, mockEnv().drawnXBMs.size());

    // Tras 800 ms el ícono de actividad debe expirar a RADIO_IDLE
    mockEnv().nowMs += 800;
    ui.update();
    TEST_ASSERT_EQUAL(RADIO_IDLE, ui.getNetworkStatus().radio_state);

    // Retransmisión Mesh cambia el estado a RADIO_MESH_REPEAT e incrementa tx_packets
    ui.reportPacketTx(/*isMeshRelay=*/true);
    TEST_ASSERT_EQUAL(RADIO_MESH_REPEAT, ui.getNetworkStatus().radio_state);
    TEST_ASSERT_EQUAL_UINT16(1, ui.getNetworkStatus().tx_packets);
}

static bool g_customRendererCalled = false;
static void customTestRenderer(U8G2 &u8g2,
                               const SeismicTelemetry_t &,
                               const MeshNetworkStatus_t &net) {
    g_customRendererCalled = true;
    LoRaMeshUIWidgets::drawTopStatusBar(u8g2, net);
    u8g2.drawStr(0, 30, "VISTA_CUSTOM_OK");
}

void test_custom_screen_renderer_hook(void) {
    g_customRendererCalled = false;
    LoRaMeshUI ui;
    ui.begin(8, 9, 400000UL, true);
    ui.setCustomScreenRenderer(SCREEN_TOPOLOGY_MAP, customTestRenderer);

    ui.showScreen(SCREEN_TOPOLOGY_MAP);
    mockEnv().nowMs += 150;
    ui.update();

    TEST_ASSERT_TRUE(g_customRendererCalled);
    TEST_ASSERT_TRUE(mockEnv().hasDrawnText("VISTA_CUSTOM_OK"));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_initializes_i2c_and_renders_home);
    RUN_TEST(test_begin_respects_initWire_false);
    RUN_TEST(test_sensor_telemetry_auto_triggers_pre_alert);
    RUN_TEST(test_network_status_auto_triggers_critical_alarm);
    RUN_TEST(test_critical_alarm_0x02_has_priority_over_pre_alert_0x01);
    RUN_TEST(test_clear_alerts_restores_saved_screen_and_normal_polarity);
    RUN_TEST(test_manual_navigation_respects_allow_all_screens_flag);
    RUN_TEST(test_button_debounce_filters_glitches_and_advances_on_stable_press);
    RUN_TEST(test_i2c_burst_arbitration_blocks_normal_redraw_but_allows_critical_transition);
    RUN_TEST(test_hardware_strobe_4hz_toggles_0xA7_0xA6_and_resets_on_exit);
    RUN_TEST(test_packet_rx_tx_updates_counts_active_nodes_and_expires_radio_icon);
    RUN_TEST(test_custom_screen_renderer_hook);
    return UNITY_END();
}
