#include "can_bridge.hpp"

// CAN(TWAI)の初期化[cite: 2]
bool CanBridge::begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        GPIO_NUM_5,
        GPIO_NUM_6,
        TWAI_MODE_NORMAL
    );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS(); // 1Mbps[cite: 2]
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // フィルタなし

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
    if (twai_start() != ESP_OK) return false;
    return true;
}



// 配列の解析
void CanBridge::processSerialBuffer(uint8_t* buf) {
    // 2. 型キャスト：メモリの解釈をバイナリから構造体へ変える[cite: 1]
    ControlPacket* packet = (ControlPacket*)buf;

    // 3. 安全の確認：チェックサムの計算（ビット演算）
    uint8_t calc = (packet->header + packet->id + (packet->value & 0xFF) + (packet->value >> 8)) & 0xFF;
    
    if (calc == packet->checksum) {
        // 4. CANへの再梱包：構造体からCANパケット（最大8byte）へ
        uint8_t can_payload[2];
        can_payload[0] = (packet->value >> 8) & 0xFF; // 上位バイト
        can_payload[1] = packet->value & 0xFF;        // 下位バイト

        transmitCan(packet->id, can_payload, 2); // 物理層へ送り出す[cite: 2]
    }
}

// 物理レイヤー（CAN）への書き込み[cite: 2]
void CanBridge::transmitCan(uint32_t id, uint8_t* data, uint8_t len) {
    twai_message_t msg;
    msg.identifier = id;
    msg.data_length_code = len;
    for (int i = 0; i < len; i++) msg.data[i] = data[i];
    
    twai_transmit(&msg, pdMS_TO_TICKS(1)); // タイムアウト1msで送信
}

void CanBridge::receiveAndDriveServos() {
    twai_message_t msg;
    if (twai_receive(&msg, 0) == ESP_OK) {


        
        // ID 0x201 をサーボ1、0x202 をサーボ2とする（識別）
        if (msg.identifier == 0x201) {
            int angle = msg.data[0]; // 1バイト目を角度として解釈
            servo1.write(angle);      // 物理層（サーボ1）へ出力
        } 
        else if (msg.identifier == 0x202) {
            int angle = msg.data[0];
            servo2.write(angle);      // 物理層（サーボ2）へ出力
        }
    }
}

void CanBridge::transmitSerialToCan(){
}