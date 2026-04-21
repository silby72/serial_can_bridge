#ifndef CAN_BRIDGE_HPP
#define CAN_BRIDGE_HPP

#include <Arduino.h>
#include "driver/twai.h"
#include "config.hpp"

// 通信に関する定数定義
namespace CanConfig {
    // CAN識別子(ID)
    const uint32_t ID_MOTOR_DRIVERS  = 0x101;
    const uint32_t ID_SERVOS          = 0x102;
    const uint32_t ID_TRANSISTORS     = 0x103;

    // シリアルデータ配列内での各データの開始位置(インデックス)
    const int OFFSET_DEBUG       = 0;
    const int OFFSET_MOTOR       = 1;
    const int OFFSET_SERVO       = 9;
    const int OFFSET_TRANSISTOR  = 17;

    // 各デバイスの搭載個数
    const int DEVICE_COUNT       = 8;
    const int TRANSISTOR_COUNT   = 7;
    
    // 通信設定(現在intのみ対応)
const auto PIN_TX = gpio_num_t(CAN_BRIDGE_TX_PIN);
const auto PIN_RX = gpio_num_t(CAN_BRIDGE_RX_PIN);
}

class CanBridge {
public:
    // 通信の初期化
    static bool begin();

    // 受信済みフレームデータを解析してCANへ転送
    static void transmitSerialToCan();

private:
    // 各デバイスカテゴリごとの送信処理
    static void sendMotorGroup();
    static void sendServoGroup();
    static void sendTransistorGroup();

    // CAN送信の基本処理
    static void executeCanTransmit(uint32_t id, const uint8_t* data, uint8_t length);
};

#endif