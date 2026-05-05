/*====================================================================
Project: esp32_s3_serial_bridge
Target board: RRST-ESP32-S3 Rev.1

Description:
  ROS 2・マイコン間の通信を行うserial_bridgeパッケージのマイコン側プログラム。
  PCから送られてくるバイナリデータを受信、デコードしマイコンのGPIO出力に反映させる。
  config.hppで各種設定をするだけで使用可能です。このファイル(main.cpp)を直接編集しないこと。

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "pid_task.hpp"
#include "pin_ctrl_task.hpp"
#include "robomas.hpp"
#include "serial_task.hpp"
#include "can_bridge.hpp"
#include <Arduino.h>

// ================= SETUP =================

void setup() {

    // ボーレートは実機テストしながら調整する予定
    Serial.begin(20);

    delay(3000);
    delay(1000 * DEVICE_ID); // 安定待ち, IDごとに開始タイミングをずらす
    
    Serial.println("\n\n");
    Serial.println("--- System Boot Start ---");
    Serial.flush();

    pinMode(LED, OUTPUT);

    // ready
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
        delay(50);
    }

    if (!CanBridge::begin()) {
        Serial.println("CAN Init Failed!");
        Serial.flush();
        while (1);
    }


Serial.println("--- SETUP DONE ---");
Serial.flush();
delay(2000); // 2秒待ってから loop() に入れる


    // ledcSetup(1, 20000, 8);
    // ledcAttachPin(LED, 1);

#if defined(ROLE_MASTER)
    // xTaskCreate(
    //     serialTask,   // タスク関数
    //     "serialTask", // タスク名
    //     2048,         // スタックサイズ（words）
    //     NULL,
    //     10, // 優先度
    //     NULL);
    static int16_t testValue = 0;
    static uint32_t lastSend = 0;
    
    // ★ 50ms ではなく、2秒(2000ms)に1回だけ送るように変更
    if (millis() - lastSend > 2000) { 
        CanBridge::transmitTestValue(testValue); 
        testValue++;
        lastSend = millis();
        
        // 送信したことをシリアルでも出す
        Serial.println(">> 送信を試みました");
    }
    
    CanBridge::receiveAndLogMessages();

#endif

    // モードに応じた初期化
    // #if defined(MODE_OUTPUT)
    //     // 出力モード初期化
    //     xTaskCreate(
    //         Output_Task,   // タスク関数
    //         "Output_Task", // タスク名
    //         2048,          // スタックサイズ（words）
    //         NULL,
    //         11, // 優先度
    //         NULL);

    // #elif defined(MODE_INPUT)
    //     // 入力モード初期化
    //     xTaskCreate(
    //         Input_Task,   // タスク関数
    //         "Input_Task", // タスク名
    //         1024,         // スタックサイズ（words）
    //         NULL,
    //         4, // 優先度
    //         NULL);

#if defined(MODE_IO)
// 一時的にコメントアウト中
    // 入出力モード初期化
    // xTaskCreate(
    //     IO_Task,   // タスク関数
    //     "IO_Task", // タスク名
    //     2048,      // スタックサイズ（words）
    //     NULL,
    //     11, // 優先度
    //     NULL);
// ここまで

#elif defined(MODE_ROBOMAS)
    // ロボマスモード初期化

    robomas_init();

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);

    xTaskCreate(
        PID_Task,   // タスク関数
        "PID_Task", // タスク名
        2048,       // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);

#elif defined(MODE_ROBOMAS_AD)
    // ロボマスモード初期化

    robomas_init();

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);

    // xTaskCreate(
    //     PID_Task,   // タスク関数
    //     "PID_Task", // タスク名
    //     2048,       // スタックサイズ（words）
    //     NULL,
    //     11, // 優先度
    //     NULL);

    // 出力モード初期化
    xTaskCreate(
        Output_Task,   // タスク関数
        "Output_Task", // タスク名
        2048,          // スタックサイズ（words）
        NULL,
        8, // 優先度
        NULL);

#elif defined(MODE_DEBUG)
    // デバッグモード初期化

    // xTaskCreate(
    //     LED_PWM_Task,   // タスク関数
    //     "LED_PWM_Task", // タスク名
    //     1024,           // スタックサイズ（words）
    //     NULL,
    //     9, // 優先度
    //     NULL)0;

    // xTaskCreate(
    //     LED_Blink100_Task,   // タスク関数
    //     "LED_Blink100_Task", // タスク名
    //     1024,                // スタックサイズ（words）
    //     NULL,
    //     9, // 優先度
    //     NULL);

    xTaskCreate(
        PID_Task,   // タスク関数
        "PID_Task", // タスク名
        2048,       // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);

#else
#error "No mode defined. Please define one mode in config.hpp."
#endif

#if (defined(MODE_OUTPUT) + defined(MODE_INPUT) + defined(MODE_IO) + \
     defined(MODE_ROBOMAS) + defined(MODE_ROBOMAS_AD) + defined(MODE_DEBUG)) != 1
#error "Invalid mode configuration. Please define exactly *one mode* in config.hpp."
#endif

    Serial.println("setupが完了しました。");
    Serial.flush(); // ★送信完了まで待機[cite: 8]
}
// ================= LOOP =================

void loop() {

    #if defined(ROLE_MASTER)
    // マスターの時だけ実行されるコード
    static int16_t testValue = 0;
    static uint32_t lastSend = 0;
    
    if (millis() - lastSend > 2000) { 
        CanBridge::transmitTestValue(testValue); 
        testValue++;
        lastSend = millis();
        Serial.println(">> 送信を試みました");
    }
    
    CanBridge::receiveAndLogMessages();

#elif defined(ROLE_SLAVE)
    // スレーブの時だけ実行されるコード
    // スレーブ側の .cpp に定義されている関数だけを呼ぶ
    CanBridge::receiveAndDriveServos(); 
    // 必要ならここにデバッグログを追加
    Serial.println("Slave mode: Waiting for CAN..."); 
#endif

}