/*====================================================================
<config.hpp>
書き込み前にここでIDと動作モードを設定してください．
MDやサーボの設定もここで行います．

【書き込み手順】
  Master: IS_MASTER を定義、DEVICE_ID を 0x01 に設定
  Slave:  IS_MASTER をコメントアウト、DEVICE_ID を 0x02 以降に設定

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <Arduino.h>

// ================= Master / Slave 切り替え =================

// Master に書き込む場合 → IS_MASTER を有効にする
// Slave  に書き込む場合 → IS_MASTER をコメントアウトする
// #define IS_MASTER

// ================= 基本設定 =================

#define CAN_BRIDGE_TX_PIN 4
#define CAN_BRIDGE_RX_PIN 5  // GPIO2は書き込み時に干渉するためGPIO5を使用

#ifdef IS_MASTER
    #define DEVICE_ID 0x01
    #define MODE_MASTER  // シリアル受信 → CAN転送のみ（serial_task.cppの#ifdef MODE_MASTERブロックでsendDataToBus()が呼ばれる）
#else
    #define DEVICE_ID 0x02
    #define MODE_OUTPUT  // SlaveはCANから受け取った値でサーボ・MDを制御
#endif

// 上記以外のモードを使う場合は IS_MASTER ブロックを無視して直接定義する
// #define MODE_INPUT
// #define MODE_IO
// #define MODE_ROBOMAS
// #define MODE_ROBOMAS_PLUS_OUTPUT
// #define MODE_ROBOMAS_PLUS_INPUT
// #define MODE_ROBOMAS_PLUS_IO
// #define MODE_DEBUG

// ================= MD関連 =================

#define MD_PWM_FREQ 20000
#define MD_PWM_RESOLUTION 8

// ================= サーボ関連 =================

#define SERVO_PWM_FREQ 50
#define SERVO_PWM_RESOLUTION 14

#define SERVO1_MIN_US 500
#define SERVO1_MAX_US 2500
#define SERVO1_MIN_DEG 0
#define SERVO1_MAX_DEG 270
#define SERVO1_INIT_DEG 0

#define SERVO2_MIN_US 500
#define SERVO2_MAX_US 2500
#define SERVO2_MIN_DEG 0
#define SERVO2_MAX_DEG 270
#define SERVO2_INIT_DEG 0

#define SERVO3_MIN_US 500
#define SERVO3_MAX_US 2500
#define SERVO3_MIN_DEG 0
#define SERVO3_MAX_DEG 270
#define SERVO3_INIT_DEG 0

#define SERVO4_MIN_US 500
#define SERVO4_MAX_US 2500
#define SERVO4_MIN_DEG 0
#define SERVO4_MAX_DEG 270
#define SERVO4_INIT_DEG 0

// ================= 高度な設定（通常は変更不要） =================

#define ENABLE_LED 1
#define ENABLE_EXTRA_TR_PIN 0