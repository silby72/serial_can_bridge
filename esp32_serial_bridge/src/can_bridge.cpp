#include "can_bridge.hpp"
#include "frame_data.hpp"

namespace {
uint8_t toU8Saturated(int16_t value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return static_cast<uint8_t>(value);
}
}

// CANドライバの初期化
bool CanBridge::begin() {
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CanConfig::PIN_TX, 
        CanConfig::PIN_RX, 
        TWAI_MODE_NORMAL
    );
    
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // ドライバのインストール
    esp_err_t install_result = twai_driver_install(&general_config, &timing_config, &filter_config);
    if (install_result != ESP_OK) {
        return false;
    }

    // ドライバの開始
    esp_err_t start_result = twai_start();
    if (start_result != ESP_OK) {
        return false;
    }

    return true;
}

// 司令塔：受信データを各デバイス用関数へ振り分ける。振り分け方は.hppの方を参照。
void CanBridge::transmitSerialToCan() {
    // モーター、サーボ、トランジスタの順に処理を実行
    sendMotorGroup();
    sendServoGroup();
    sendTransistorGroup();
}

// モーター指令の送信
void CanBridge::sendMotorGroup() {
    uint8_t payload[CanConfig::DEVICE_COUNT] = {0};
    for (int i = 0; i < CanConfig::DEVICE_COUNT; i = i + 1) {
        payload[i] = toU8Saturated(Rx_16Data[CanConfig::OFFSET_MOTOR + i]);
    }

    executeCanTransmit(
        CanConfig::ID_MOTOR_DRIVERS, 
        payload,
        CanConfig::DEVICE_COUNT
    );
}

// サーボ指令の送信
void CanBridge::sendServoGroup() {
    uint8_t payload[CanConfig::DEVICE_COUNT] = {0};
    for (int i = 0; i < CanConfig::DEVICE_COUNT; i = i + 1) {
        payload[i] = toU8Saturated(Rx_16Data[CanConfig::OFFSET_SERVO + i]);
    }

    executeCanTransmit(
        CanConfig::ID_SERVOS, 
        payload,
        CanConfig::DEVICE_COUNT
    );
}

// トランジスタ指令の送信（ビットパッキング処理）
void CanBridge::sendTransistorGroup() {
    uint8_t packed_bits = 0x00;

    for (int i = 0; i < CanConfig::TRANSISTOR_COUNT; i = i + 1) {
        // 各TRの0/1判定
        int16_t current_tr_value = Rx_16Data[CanConfig::OFFSET_TRANSISTOR + i];
        
        if (current_tr_value != 0) {
            // ビットを立てる
            packed_bits = packed_bits | (1 << i);
        }
    }

    uint8_t payload[1];
    payload[0] = packed_bits;

    executeCanTransmit(CanConfig::ID_TRANSISTORS, payload, 1);
}

// 物理レイヤーへの送信実行
void CanBridge::executeCanTransmit(uint32_t id, const uint8_t* data, uint8_t length) {
    if (data == nullptr || length == 0 || length > 8) {
        return;
    }

    twai_message_t message;
    
    message.identifier = id;
    message.extd = 0;               // 標準IDを使用
    message.rtr = 0;                // データフレーム
    message.data_length_code = length;
    
    // データのコピー
    for (int i = 0; i < length; i = i + 1) {
        message.data[i] = data[i];
    }

    // 送信実行（タイムアウト1ms）
    twai_transmit(&message, pdMS_TO_TICKS(1));
}