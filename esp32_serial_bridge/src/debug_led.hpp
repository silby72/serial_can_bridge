/*====================================================================
<debug_led.hpp>
LED デバッグシステムのヘッダーファイル
シリアルモニタの代わりにLEDの点灯パターンでデバッグ情報を表示します

使用例:
  - DEBUG_INIT_LED: 初期化開始を点灯で示す
  - DEBUG_SUCCESS_LED: 成功（素早い点灯）
  - DEBUG_ERROR_LED: エラー（遅い点灯、繰り返し）
  - DEBUG_PULSE_LED: パルス出力で状態を数値表示

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <Arduino.h>

// ================= デバッグLED設定 =================

// デバッグに使用するGPIOピン（TR1-TR3は通常、ソレノイドに使用）
// 利用可能なピン: 25(TR1), 26(TR2), 27(TR3), 32, 33など
#define DEBUG_LED_PIN_1 32  // 第1デバッグLED
#define DEBUG_LED_PIN_2 33  // 第2デバッグLED
#define DEBUG_LED_PIN_3 25  // 第3デバッグLED（オプション）

// LED点灯パターンのタイミング（ミリ秒）
#define DEBUG_SHORT_FLASH 100   // 短点灯
#define DEBUG_LONG_FLASH 500    // 長点灯
#define DEBUG_PULSE_INTERVAL 100 // パルス間隔

// ================= 初期化関数 =================

/**
 * @brief デバッグLED システムの初期化
 */
inline void debug_led_init() {
    pinMode(DEBUG_LED_PIN_1, OUTPUT);
    pinMode(DEBUG_LED_PIN_2, OUTPUT);
    pinMode(DEBUG_LED_PIN_3, OUTPUT);
    
    digitalWrite(DEBUG_LED_PIN_1, LOW);
    digitalWrite(DEBUG_LED_PIN_2, LOW);
    digitalWrite(DEBUG_LED_PIN_3, LOW);
}

// ================= 基本的な点灯パターン =================

/**
 * @brief 短い点灯（成功・正常状態の確認用）
 * @param pin 使用するGPIOピン
 * @param count 点灯回数（デフォルト1回）
 */
inline void debug_flash_short(uint8_t pin, uint8_t count = 1) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delay(DEBUG_SHORT_FLASH);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(DEBUG_SHORT_FLASH);
    }
}

/**
 * @brief 長い点灯（エラー・警告状態の表示用）
 * @param pin 使用するGPIOピン
 * @param count 点灯回数（デフォルト1回）
 */
inline void debug_flash_long(uint8_t pin, uint8_t count = 1) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delay(DEBUG_LONG_FLASH);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(DEBUG_LONG_FLASH);
    }
}

/**
 * @brief パルス出力で数値を表示（LED の点灯回数で値を表現）
 * @param pin 使用するGPIOピン
 * @param value 表示する値（1-9推奨）
 */
inline void debug_pulse_value(uint8_t pin, uint8_t value) {
    for (uint8_t i = 0; i < value; i++) {
        digitalWrite(pin, HIGH);
        delay(DEBUG_SHORT_FLASH);
        digitalWrite(pin, LOW);
        delay(DEBUG_SHORT_FLASH);
    }
    delay(500); // 値と値の間隔
}

/**
 * @brief 複数ピンの組み合わせで2進数表現（状態フラグ）
 * @param value 表示する値（3ビット対応: 0-7）
 *              bit 0 → DEBUG_LED_PIN_1
 *              bit 1 → DEBUG_LED_PIN_2
 *              bit 2 → DEBUG_LED_PIN_3
 */
inline void debug_binary_display(uint8_t value) {
    digitalWrite(DEBUG_LED_PIN_1, (value & 0x01) ? HIGH : LOW);
    digitalWrite(DEBUG_LED_PIN_2, (value & 0x02) ? HIGH : LOW);
    digitalWrite(DEBUG_LED_PIN_3, (value & 0x04) ? HIGH : LOW);
}

/**
 * @brief すべてのデバッグLEDを消灯
 */
inline void debug_led_all_off() {
    digitalWrite(DEBUG_LED_PIN_1, LOW);
    digitalWrite(DEBUG_LED_PIN_2, LOW);
    digitalWrite(DEBUG_LED_PIN_3, LOW);
}

/**
 * @brief すべてのデバッグLEDを点灯（テスト用）
 */
inline void debug_led_all_on() {
    digitalWrite(DEBUG_LED_PIN_1, HIGH);
    digitalWrite(DEBUG_LED_PIN_2, HIGH);
    digitalWrite(DEBUG_LED_PIN_3, HIGH);
}

// ================= システムレベルのデバッグマクロ =================

/**
 * 使用例：
 * 
 * setup() 内：
 *   debug_led_init();  // 初期化
 *   debug_flash_short(DEBUG_LED_PIN_1);  // 初期化完了を示す短い点灯
 * 
 * 初期化エラー時：
 *   debug_flash_long(DEBUG_LED_PIN_1, 3);  // 3回の長い点灯でエラー表示
 * 
 * タスク作成時（ID番号を視覚化）：
 *   debug_pulse_value(DEBUG_LED_PIN_1, DEVICE_ID);  // デバイスIDを点灯数で表示
 * 
 * 複数状態の表示：
 *   debug_binary_display(0b101);  // LED1と3を点灯、LED2は消灯
 */

//#endif /* DEBUG_LED_HPP */
