/*====================================================================
<can_bridge.cpp>
・マイコン間CAN通信ブリッジの実装ファイル
・ESP32無印 + MCP2561 を使用
・ESP-IDF組み込みTWAIドライバ使用（外部ライブラリ不要）

注意:
  robomas.cpp でもTWAIドライバを使う MODE_ROBOMAS 等では
  TWAIドライバが競合するため can_bridge は使用不可。
  can_bridge は MODE_OUTPUT / MODE_INPUT / MODE_IO 等の
  Slave側マイコンで使用することを想定している。

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "can_bridge.hpp"
#include "driver/twai.h"

namespace CanBridge {

// ================================================================
// 初期化（Master / Slave 共通）
// ================================================================
bool begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_BRIDGE_TX_PIN,
        (gpio_num_t)CAN_BRIDGE_RX_PIN,
        TWAI_MODE_NORMAL
    );
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        return false;
    }
    if (twai_start() != ESP_OK) {
        return false;
    }
    return true;
}

// ================================================================
// Master用: Rx_16Data[] → CAN送信
// ================================================================
//
// Rx_16Data[] を DATA_PER_FRAME(4) 個ずつ分割し，
// フレームID BASE_ID + フレーム番号 で送信する。
//
// 例) Rx16NUM=24, DATA_PER_FRAME=4 → 6フレーム送信
//   ID=0x100: Rx_16Data[ 0~ 3]
//   ID=0x101: Rx_16Data[ 4~ 7]
//   ...
//   ID=0x105: Rx_16Data[20~23]
//
// ================================================================
void sendDataToBus() {
#ifdef IS_MASTER
    for (uint8_t f = 0; f < NUM_FRAMES; f++) {
        twai_message_t tx;
        tx.identifier      = BASE_ID + f;
        tx.extd            = 0; // 標準フレーム
        tx.rtr             = 0; // データフレーム
        tx.data_length_code = 8; // 常に8バイト（未使用部は0埋め）

        for (uint8_t i = 0; i < DATA_PER_FRAME; i++) {
            uint8_t idx = f * DATA_PER_FRAME + i;
            int16_t val = (idx < Rx16NUM) ? Rx_16Data[idx] : 0;

            // big-endian で格納
            tx.data[i * 2]     = (uint8_t)(val >> 8);
            tx.data[i * 2 + 1] = (uint8_t)(val & 0xFF);
        }

        // タイムアウト0で即時送信（ブロッキングしない）
        // 送信失敗は許容（次の周期で再送される）
        twai_transmit(&tx, 0);
    }
#endif // IS_MASTER
}

// ================================================================
// Slave用: CAN受信 → Rx_16Data[] 格納
// ================================================================
//
// 受信バッファを空になるまでポーリングし，
// 対象IDのフレームを Rx_16Data[] に展開する。
// タスクのループ先頭で毎回呼ぶ（ノンブロッキング）。
//
// ================================================================
void receiveDataFromBus() {
#ifndef IS_MASTER
    twai_message_t rx;

    // 受信バッファを全部処理する（タイムアウト0 = ノンブロッキング）
    while (twai_receive(&rx, 0) == ESP_OK) {
        // 対象IDの範囲チェック
        if (rx.identifier < BASE_ID ||
            rx.identifier >= (uint32_t)(BASE_ID + NUM_FRAMES)) {
            continue;
        }
        if (rx.data_length_code < 8) {
            continue;
        }

        uint8_t f = (uint8_t)(rx.identifier - BASE_ID);

        for (uint8_t i = 0; i < DATA_PER_FRAME; i++) {
            uint8_t idx = f * DATA_PER_FRAME + i;
            if (idx >= Rx16NUM) break;

            // big-endian でデコード
            Rx_16Data[idx] = (int16_t)(
                ((uint16_t)rx.data[i * 2]     << 8) |
                 (uint16_t)rx.data[i * 2 + 1]
            );
        }
    }
#endif // IS_MASTER
}

} // namespace CanBridge