#include "can_bridge.hpp"
#include "driver/twai.h"
#include "frame_data.hpp"
#include "config.hpp"

namespace CanBridge {

bool begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_BRIDGE_TX_PIN, 
        (gpio_num_t)CAN_BRIDGE_RX_PIN, 
        TWAI_MODE_NORMAL
    );
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) return false;
    if (twai_start() != ESP_OK) return false;
    return true;
}

// Master用：シリアルデータをCANへ送信
void sendDataToBus() {
    twai_message_t msg;
    msg.identifier = 0x100; // システム共通のデータID（またはSlaveのID）
    msg.extd = 0;
    msg.data_length_code = 4; // 16bit * 2 = 4 bytes

    // Rx_16Data[9], [10] を格納
    msg.data[0] = (uint8_t)(Rx_16Data[9] >> 8);
    msg.data[1] = (uint8_t)(Rx_16Data[9] & 0xFF);
    msg.data[2] = (uint8_t)(Rx_16Data[10] >> 8);
    msg.data[3] = (uint8_t)(Rx_16Data[10] & 0xFF);

    twai_transmit(&msg, pdMS_TO_TICKS(10));
}

// Slave用：CANからデータを受信してローカル変数に展開
void receiveDataFromBus() {
    twai_message_t msg;
    // タイムアウト0でノンブロッキング受信
    if (twai_receive(&msg, 0) == ESP_OK) {
        if (msg.identifier == 0x100 && msg.data_length_code == 4) {
            Rx_16Data[9]  = (int16_t)((msg.data[0] << 8) | msg.data[1]);
            Rx_16Data[10] = (int16_t)((msg.data[2] << 8) | msg.data[3]);
        }
    }
}

} // namespace CanBridge