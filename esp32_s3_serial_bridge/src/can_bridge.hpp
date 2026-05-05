#ifndef CAN_BRIDGE_HPP
#define CAN_BRIDGE_HPP

#include <Arduino.h>
#include <ESP32Servo.h>
#include "driver/twai.h" // ESP-IDFのCAN(TWAI)ドライバ

// 1. 受信データの構造定義（ボトムアップなメモリ管理の核）
#pragma pack(1) // メモリの隙間（パディング）を詰め、バイナリと完全に一致させる
struct ControlPacket {
    uint8_t  header;   // 識別：0xAA
    uint8_t  id;       // 識別：デバイスID
    int16_t  value;    // 本題：制御値（-32768〜32767）
    uint8_t  checksum; // 安全：データの正しさ確認
    
};
#pragma pack()

class CanBridge {
public: // ★ここから下は外（main.cpp）から見える
    static bool begin();
    static void transmitSerialToCan();
    static void receiveAndDriveServos();
    static void processSerialBuffer(uint8_t* buf);
    static void transmitCan(uint32_t id, uint8_t* data, uint8_t len);
    static void WAI_GENERAL_CONFIG_DEFAULT();
    static void transmitTestValue(int16_t value);
    static void receiveAndLogMessages();

    static Servo servo1; 
    static Servo servo2;

};

#endif