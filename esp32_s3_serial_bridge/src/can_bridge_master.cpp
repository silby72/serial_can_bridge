#include "config.hpp"
#include "defs.hpp"
#include "can_bridge.hpp"


// PC(ROS2)側から送られてくる配列
extern volatile int16_t Rx_16Data[24];

bool CanBridge::begin() {
    
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX,
        (gpio_num_t)CAN_RX,
        TWAI_MODE_NORMAL);

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
    if (twai_start() != ESP_OK) return false;
    
    Serial.println("CAN Master Initialized (1Mbps)"); //デバッグ用
    return true;
}

void CanBridge::transmitSerialToCan() {

    // 16bitデータを2バイトに分割して送信（安全のため）
    uint8_t d1[2] = { (uint8_t)(Rx_16Data[9] >> 8), (uint8_t)(Rx_16Data[9] & 0xFF) };
    uint8_t d2[2] = { (uint8_t)(Rx_16Data[10] >> 8), (uint8_t)(Rx_16Data[10] & 0xFF) };

    // CANバスへ送信
    transmitCan(0x201, d1, 2); 
    transmitCan(0x202, d2, 2);
}

void CanBridge::transmitCan(uint32_t id, uint8_t* data, uint8_t len) {
    twai_message_t msg;
    msg.identifier = id;
    msg.extd = 0; // 標準ID
    msg.rtr = 0;  // データフレーム
    msg.data_length_code = len;
    for (int i = 0; i < len; i++) msg.data[i] = data[i];

    // 送信結果をシリアルモニタで確認できるようにする
    esp_err_t res = twai_transmit(&msg, pdMS_TO_TICKS(10));
    if (res != ESP_OK) {
        // 送信失敗（バスが混んでいる、または相手がいない等）
        Serial.printf("CAN Send Error: 0x%X\n", res);
    }
}