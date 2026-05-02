#include "config.hpp"
#include "defs.hpp"
#include "can_bridge.hpp"

// スレイブとして定義
#define ROLE_SLAVE 

Servo CanBridge::servo1;
Servo CanBridge::servo2;

bool CanBridge::begin() {
#if defined(ROLE_SLAVE)
    // 物理ピンへのアタッチ（ピン番号は config.hpp 等で定義されている想定）
    servo1.attach(SERVO1);
    servo2.attach(SERVO2);
#endif

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX,
        (gpio_num_t)CAN_RX,
        TWAI_MODE_NORMAL);

    // マスターと一致させるため 500kbps に設定
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // 全IDを受信

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
    if (twai_start() != ESP_OK) return false;

    Serial.println("CAN Slave Initialized (500kbps)");
    return true;
}

void CanBridge::receiveAndDriveServos() {
    twai_message_t msg;
    
    // 20ms タイムアウトで受信を試みる[cite: 4]
    if (twai_receive(&msg, pdMS_TO_TICKS(20)) == ESP_OK) {
        
        // マスターが 2バイト (int16) で送っている場合の復元処理
        // msg.data[0] が上位バイト、msg.data[1] が下位バイト[cite: 2]
        int16_t value = (msg.data[0] << 8) | msg.data[1];

        // 受信ログを出力（デバッグの要）[cite: 4]
        Serial.printf("Received ID: 0x%03X, Value: %d\n", msg.identifier, value);

        if (msg.identifier == 0x201) {
            // サーボ1 (0-180度) に制限して出力[cite: 2]
            servo1.write(constrain(value, 0, 180));
        } 
        else if (msg.identifier == 0x202) {
            // サーボ2 に出力[cite: 2]
            servo2.write(constrain(value, 0, 180));
        }
    }
}