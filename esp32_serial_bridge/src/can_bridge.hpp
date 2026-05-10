#pragma once

#include <Arduino.h>

namespace CanBridge {
    // 通信の初期化
    bool begin();

    // Master用：シリアルで受け取ったデータをCANへ送信
    void sendDataToBus();

    // Slave用：CANからデータを受信してRx_16Dataへ展開
    void receiveDataFromBus();
}