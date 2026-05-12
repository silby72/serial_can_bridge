/*====================================================================
Project: esp32_serial_bridge
Target board: ESP32 Dev Module

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
#include <Arduino.h>
#include "can_bridge.hpp"
#include "debug_led.hpp"
// ================= SETUP =================

void setup() {

    // ボーレートは実機テストしながら調整する予定
    Serial.begin(115200);

    delay(200);
    delay(100 * DEVICE_ID); // 安定待ち, IDごとに開始タイミングをずらす

    pinMode(LED, OUTPUT);
    
    // デバッグLED システムの初期化
    debug_led_init();
    debug_flash_short(DEBUG_LED_PIN_1); // 初期化開始を示す

    if (!CanBridge::begin()) {
        // CAN初期化失敗: 長い点灯で3回エラー表示
        debug_flash_long(DEBUG_LED_PIN_1, 3);
        Serial.println("[CAN] initialization failed");
    } else {
        // CAN初期化成功: 短い点灯で2回表示
        debug_flash_short(DEBUG_LED_PIN_1, 2);
        Serial.println("[CAN] initialization success");
    }

    // ready: デバイスIDを点灯数で表示
    debug_pulse_value(DEBUG_LED_PIN_2, DEVICE_ID);

    // ledcSetup(1, 20000, 8);
    // ledcAttachPin(LED, 1);

    // xTaskCreate(
    //     serialTask,   // タスク関数
    //     "serialTask", // タスク名
    //     2048,         // スタックサイズ（words）
    //     NULL,
    //     10, // 優先度
    //     NULL);
    // Serial.println("[TASK] serialTask created (prio=10)");

// モードに応じた初期化
#if defined(MODE_OUTPUT)
    // 出力モード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 1); // モード1: OUTPUT
    Serial.println("[MODE] MODE_OUTPUT");
    
    xTaskCreate(
        serialTask,   // シリアル受信タスク（Masterには必須）
        "serialTask",
        4096,         // 余裕を持たせて4096
        NULL,
        10, 
        NULL);
    Serial.println("[TASK] serialTask created (prio=10)");

    xTaskCreate(
        Output_Task,   // 出力タスク（SlaveでCAN受信に必須）
        "Output_Task", 
        4096,          // こちらも4096へ
        NULL,
        11, 
        NULL);
    Serial.println("[TASK] Output_Task created (prio=11)");

#elif defined(MODE_INPUT)
    // 入力モード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 2); // モード2: INPUT
    Serial.println("[MODE] MODE_INPUT");
    xTaskCreate(
        Input_Task,   // タスク関数
        "Input_Task", // タスク名
        1024,         // スタックサイズ（words）
        NULL,
        4, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] Input_Task created (prio=4)");

#elif defined(MODE_IO)
    // 入出力モード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 3); // モード3: IO
    Serial.println("[MODE] MODE_IO");
    xTaskCreate(
        IO_Task,   // タスク関数
        "IO_Task", // タスク名
        2048,      // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] IO_Task created (prio=11)");

#elif defined(MODE_ROBOMAS)
    // ロボマスモード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 4); // モード4: ROBOMAS
    Serial.println("[MODE] MODE_ROBOMAS");

    robomas_init();
    debug_flash_short(DEBUG_LED_PIN_3); // ROBOMAS初期化完了
    Serial.println("[ROBOMAS] init complete");

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] M3508_Task created (prio=9)");

    xTaskCreate(
        PID_Task,   // タスク関数
        "PID_Task", // タスク名
        2048,       // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] PID_Task created (prio=11)");

#elif defined(MODE_ROBOMAS_PLUS_OUTPUT)
    // ロボマスモード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 5); // モード5: ROBOMAS_PLUS_OUTPUT
    Serial.println("[MODE] MODE_ROBOMAS_PLUS_OUTPUT");

    robomas_init();
    debug_flash_short(DEBUG_LED_PIN_3); // ROBOMAS初期化完了
    Serial.println("[ROBOMAS] init complete");

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] M3508_Task created (prio=9)");

    // 出力モード初期化
    xTaskCreate(
        Output_Task,   // タスク関数
        "Output_Task", // タスク名
        2048,          // スタックサイズ（words）
        NULL,
        8, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] Output_Task created (prio=8)");

#elif defined(MODE_ROBOMAS_PLUS_INPUT)
    debug_flash_short(DEBUG_LED_PIN_2, 6); // モード6: ROBOMAS_PLUS_INPUT
    Serial.println("[MODE] MODE_ROBOMAS_PLUS_INPUT");

    robomas_init();
    debug_flash_short(DEBUG_LED_PIN_3); // ROBOMAS初期化完了
    Serial.println("[ROBOMAS] init complete");

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] M3508_Task created (prio=9)");

    xTaskCreate(
        Input_Task,   // タスク関数
        "Input_Task", // タスク名
        1024,         // スタックサイズ（words）
        NULL,
        4, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] Input_Task created (prio=4)");

#elif defined(MODE_ROBOMAS_PLUS_IO)
    debug_flash_short(DEBUG_LED_PIN_2, 7); // モード7: ROBOMAS_PLUS_IO
    Serial.println("[MODE] MODE_ROBOMAS_PLUS_IO");

    robomas_init();
    debug_flash_short(DEBUG_LED_PIN_3); // ROBOMAS初期化完了
    Serial.println("[ROBOMAS] init complete");

    xTaskCreate(
        M3508_Task,   // タスク関数
        "M3508_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        9, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] M3508_Task created (prio=9)");

    xTaskCreate(
        ROBOMAS_IO_Task,   // タスク関数
        "ROBOMAS_IO_Task", // タスク名
        2048,              // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] ROBOMAS_IO_Task created (prio=11)");

#elif defined(MODE_DEBUG)
    // デバッグモード初期化
    debug_flash_short(DEBUG_LED_PIN_2, 8); // モード8: DEBUG
    Serial.println("[MODE] MODE_DEBUG");

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
    debug_flash_short(DEBUG_LED_PIN_3); // タスク作成完了
    Serial.println("[TASK] PID_Task created (prio=11)");

#else
#error "No mode defined. Please define one mode in config.hpp."
#endif

#if (defined(MODE_OUTPUT) + defined(MODE_INPUT) + defined(MODE_IO) + \
     defined(MODE_ROBOMAS) + defined(MODE_ROBOMAS_PLUS_OUTPUT) + defined(MODE_ROBOMAS_PLUS_INPUT) + defined(MODE_ROBOMAS_PLUS_IO) + defined(MODE_DEBUG)) != 1
#error "Invalid mode configuration. Please define exactly *one mode* in config.hpp."
#endif

    Serial.println("[BOOT] setup complete");
}

// ================= LOOP =================

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    // メインループはなにもしない、処理はすべてFreeRTOSタスクで行う
}