/*====================================================================
<can_bridge.cpp>
・マイコン間CAN通信ブリッジの実装ファイル
・ESP32無印 + MCP2561 を使用
・ESP-IDF組み込みTWAIドライバ使用（外部ライブラリ不要）

・sendDataToBus() / receiveDataFromBus() の初回呼び出し時に
  自動で初期化するため main.cpp の改造不要

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "can_bridge.hpp"
#include "driver/twai.h"

namespace CanBridge {

// ================================================================
// 内部状態
// ================================================================
static bool s_initialized = false;

// ================================================================
// 内部: TWAIドライバ初期化
// ================================================================
static bool twai_init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_BRIDGE_TX_PIN,
        (gpio_num_t)CAN_BRIDGE_RX_PIN,
        TWAI_MODE_NORMAL
    );
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("[CAN] driver install failed");
        return false;
    }
    if (twai_start() != ESP_OK) {
        Serial.println("[CAN] start failed");
        return false;
    }
    return true;
}

// ================================================================
// 内部: バスオフ検出 → 自動リカバリ
// ================================================================
static void recover_if_busoff() {
    twai_status_info_t status;
    if (twai_get_status_info(&status) != ESP_OK) return;
    if (status.state != TWAI_STATE_BUS_OFF) return;

    // ドライバごと再インストール（twai_initiate_recovery後にtwai_startするとassert crashするため）
    twai_stop();
    twai_driver_uninstall();
    delay(100);
    twai_init();
}

// ================================================================
// 公開: 明示的初期化（呼ばなくても自動初期化される）
// ================================================================
bool begin() {
    if (s_initialized) return true;
    bool ok = twai_init();
    if (ok) s_initialized = true;
    return ok;
}

// ================================================================
// Master用: Rx_16Data[] → CAN送信
// ================================================================
//
// serial_task.cpp の受信完了時（#ifdef MODE_MASTER ブロック）から呼ばれる
// Rx_16Data[] を DATA_PER_FRAME(4) 個ずつ分割して送信
//
// ================================================================
void sendDataToBus() {
#ifdef IS_MASTER
    // 未初期化なら自動初期化
    if (!s_initialized) {
        if (!twai_init()) return;
        s_initialized = true;
    }

    recover_if_busoff();

    for (uint8_t f = 0; f < NUM_FRAMES; f++) {
        twai_message_t tx = {};
        tx.identifier       = BASE_ID + f;
        tx.extd             = 0;
        tx.rtr              = 0;
        tx.data_length_code = 8;

        for (uint8_t i = 0; i < DATA_PER_FRAME; i++) {
            uint8_t idx = f * DATA_PER_FRAME + i;
            int16_t val = (idx < Rx16NUM) ? Rx_16Data[idx] : 0;
            tx.data[i * 2]     = (uint8_t)(val >> 8);
            tx.data[i * 2 + 1] = (uint8_t)(val & 0xFF);
        }

        // タイムアウト10ms（Slaveが繋がっていればACKが返る）
        twai_transmit(&tx, pdMS_TO_TICKS(10));
    }
#endif // IS_MASTER
}

// ================================================================
// Slave用: CAN受信 → Rx_16Data[] 格納
// ================================================================
//
// pin_ctrl_task.cpp の Output_Task ループ先頭から呼ばれる
// 受信バッファを空になるまでポーリング（ノンブロッキング）
//
// ================================================================
void receiveDataFromBus() {
#ifndef IS_MASTER
    // 未初期化なら自動初期化
    if (!s_initialized) {
        if (!twai_init()) return;
        s_initialized = true;
    }

    recover_if_busoff();

    twai_message_t rx = {};
    while (twai_receive(&rx, 0) == ESP_OK) {
        if (rx.identifier < BASE_ID ||
            rx.identifier >= (uint32_t)(BASE_ID + NUM_FRAMES)) continue;
        if (rx.data_length_code < 8) continue;

        uint8_t f = (uint8_t)(rx.identifier - BASE_ID);
        for (uint8_t i = 0; i < DATA_PER_FRAME; i++) {
            uint8_t idx = f * DATA_PER_FRAME + i;
            if (idx >= Rx16NUM) break;
            Rx_16Data[idx] = (int16_t)(
                ((uint16_t)rx.data[i * 2]     << 8) |
                 (uint16_t)rx.data[i * 2 + 1]
            );
        }
    }
#endif // IS_MASTER
}

} // namespace CanBridge