/*====================================================================
<can_test_main.cpp>
・CAN通信テスト（NORMALモード、Master+Slave両接続）
・config.hpp で IS_MASTER 定義あり → Master（定数送信）
・IS_MASTER 未定義               → Slave（受信表示）

期待される結果:
  Master: [TX] ID=0x100~0x105  OK:6  NG:0  が繰り返される
  Slave:  [RX] ID=0x100~0x105 と値が表示される

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include <Arduino.h>
#include "driver/twai.h"

// ===== ピン設定 =====
#define CAN_TX_PIN 4
#define CAN_RX_PIN 5  // GPIO2は書き込み干渉するためGPIO5を使用

// ===== CAN設定 =====
#define BASE_ID        0x100
#define DATA_PER_FRAME 4
#define NUM_FRAMES     6

// #define IS_MASTER

static const int16_t TEST_DATA[NUM_FRAMES * DATA_PER_FRAME] = {
    1,   2,   3,   4,
    5,   6,   7,   8,
    100, 200, 300, 400,
    -1,  -2,  -3,  -4,
    0,   0,   0,   0,
    999, 0,   0,   0,
};

// ================================================================
// 初期化（Master / Slave 共通、両方NORMALモード）
// ================================================================
bool can_init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN,
        (gpio_num_t)CAN_RX_PIN,
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
// バスオフ検出 → 自動リカバリ
// ================================================================
// true を返したらその周期は送信しない
bool check_and_recover() {
    twai_status_info_t status;
    twai_get_status_info(&status);

    if (status.state != TWAI_STATE_BUS_OFF) return false;

    Serial.println("[CAN] BUS OFF → restarting...");
    twai_stop();
    twai_driver_uninstall();
    delay(200);
    can_init();
    Serial.println("[CAN] restarted");
    return true;
}

// ================================================================
// Master: 定数を6フレーム送信
// ================================================================
#ifdef IS_MASTER

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[MASTER] boot  (NORMAL mode)");
    if (!can_init()) { while(1); }
    Serial.println("[CAN] init OK");
}

void loop() {
    if (check_and_recover()) {
        delay(500);
        return;
    }

    int ok = 0, ng = 0;

    for (uint8_t f = 0; f < NUM_FRAMES; f++) {
        twai_message_t tx = {};
        tx.identifier       = BASE_ID + f;
        tx.extd             = 0;
        tx.rtr              = 0;
        tx.data_length_code = 8;

        for (uint8_t i = 0; i < DATA_PER_FRAME; i++) {
            int16_t val = TEST_DATA[f * DATA_PER_FRAME + i];
            tx.data[i * 2]     = (uint8_t)(val >> 8);
            tx.data[i * 2 + 1] = (uint8_t)(val & 0xFF);
        }

        delay(5); // フレーム間に少し待つ（バッファ溢れ防止）

        esp_err_t err = twai_transmit(&tx, pdMS_TO_TICKS(50));
        if (err == ESP_OK) {
            Serial.printf("[TX] ID=0x%03X  OK   data: %6d %6d %6d %6d\n",
                tx.identifier,
                TEST_DATA[f * DATA_PER_FRAME + 0],
                TEST_DATA[f * DATA_PER_FRAME + 1],
                TEST_DATA[f * DATA_PER_FRAME + 2],
                TEST_DATA[f * DATA_PER_FRAME + 3]);
            ok++;
        } else {
            Serial.printf("[TX] ID=0x%03X  FAILED (err=%d)\n", tx.identifier, err);
            ng++;
        }
    }

    Serial.printf("--- OK:%d  NG:%d\n\n", ok, ng);
    delay(1000);
}

// ================================================================
// Slave: 受信してシリアルに表示
// ================================================================
#else

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[SLAVE] boot  (NORMAL mode)");
    if (!can_init()) { while(1); }
    Serial.println("[CAN] init OK");
}

void loop() {
    check_and_recover();

    twai_message_t rx = {};
    while (twai_receive(&rx, 0) == ESP_OK) {
        if (rx.data_length_code < 8) continue;
        if (rx.identifier < BASE_ID || rx.identifier >= BASE_ID + NUM_FRAMES) continue;

        int16_t vals[DATA_PER_FRAME];
        for (int i = 0; i < DATA_PER_FRAME; i++) {
            vals[i] = (int16_t)(
                ((uint16_t)rx.data[i * 2] << 8) |
                 (uint16_t)rx.data[i * 2 + 1]
            );
        }
        Serial.printf("[RX] ID=0x%03X  data: %6d %6d %6d %6d\n",
            rx.identifier, vals[0], vals[1], vals[2], vals[3]);
    }
    delay(10);
}

#endif