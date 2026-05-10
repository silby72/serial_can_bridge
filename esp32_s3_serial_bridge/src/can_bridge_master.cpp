#include "config.hpp"
#include "defs.hpp"
#include "can_bridge.hpp"

CanBridge canBridge;

bool CanBridge::begin() {
    // 1. 基本設定
    // TWAI_MODE_NO_LOOPBACK: ループバックモードを無効にする
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX,
        (gpio_num_t)CAN_RX,
        TWAI_MODE_NORMAL
    );

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // 2. ドライバのインストール
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("CAN Driver Install Failed! Error: 0x%X\n", err);
        return false;
    }

    // 3. ドライバの開始
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("CAN Driver Start Failed! Error: 0x%X\n", err);
        return false;
    }

    Serial.println("--- CAN DRIVER STARTED SUCCESSFULLY (NO_ACK MODE) ---");
    return true;
}

void CanBridge::transmitSerialToCan() {
// 疎通確認用のダミーデータ（定数）
    int16_t dummy_servo1 = 1500;
    int16_t dummy_servo2 = -500;

    // 定数をそのままCANへ送信
    // ID: 0x201 に dummy_servo1 を、0x202 に dummy_servo2 を送信
    transmitCan(0x201, (uint8_t*)&dummy_servo1, 2);
    transmitCan(0x202, (uint8_t*)&dummy_servo2, 2);

    // 送信していることがわかるようにデバッグ表示
    // 頻繁にログが出過ぎる場合は、メインのloop側で頻度を調整してください
    Serial.printf("Test Send - ID 0x201: %d, ID 0x202: %d\n", dummy_servo1, dummy_servo2);

}

void CanBridge::transmitTestValue(int16_t value) {
    uint8_t d[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    transmitCan(0x201, d, 2);
}

void CanBridge::transmitCan(uint32_t id, uint8_t* data, uint8_t len) {
    twai_message_t msg;
    msg.identifier = id;
    msg.extd = 0;
    msg.rtr = 0;
    msg.data_length_code = len;

    for (int i = 0; i < len; i++) msg.data[i] = data[i];

    esp_err_t res = twai_transmit(&msg, pdMS_TO_TICKS(10));
    if (res != ESP_OK) {
        Serial.printf("CAN Send Error: 0x%X\n", res);
    }
}

void CanBridge::receiveAndLogMessages() {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
        int16_t value = (int16_t)((msg.data[0] << 8) | msg.data[1]);
        Serial.printf("CAN RX ID: 0x%03lX, Value: %d\n", msg.identifier, value);
    }
}