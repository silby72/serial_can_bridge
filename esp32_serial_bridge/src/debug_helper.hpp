/*====================================================================
<debug_helper.hpp>
FreeRTOS タスク内での LED デバッグ用ヘルパー関数
delay() を使用しない方式でタスク内デバッグを実装します

使用例:
  - debug_binary_display(): タスク内での状態表示（ノンブロッキング）
  - Serial.println() と LED を組み合わせたハイブリッドデバッグ

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include "debug_led.hpp"
#include <Arduino.h>

// ================= タスク内非同期デバッグ関数 =================

/**
 * @brief タスク内でノンブロッキングにLED状態を設定
 * @param led_state 0-7 の値（3ビット対応）
 *        bit0: DEBUG_LED_PIN_1 (GPIO32)
 *        bit1: DEBUG_LED_PIN_2 (GPIO33)
 *        bit2: DEBUG_LED_PIN_3 (GPIO25)
 */
inline void set_task_debug_state(uint8_t led_state) {
    debug_binary_display(led_state);
}

/**
 * @brief 特定のLED ピンをトグル（点灯/消灯を反転）
 * @param pin GPIO ピン番号
 */
inline void toggle_debug_led(uint8_t pin) {
    digitalWrite(pin, !digitalRead(pin));
}

/**
 * @brief 特定のLED を点灯状態に設定
 * @param pin GPIO ピン番号
 */
inline void set_debug_led_on(uint8_t pin) {
    digitalWrite(pin, HIGH);
}

/**
 * @brief 特定のLED を消灯状態に設定
 * @param pin GPIO ピン番号
 */
inline void set_debug_led_off(uint8_t pin) {
    digitalWrite(pin, LOW);
}

// ================= デバッグ状態定義 =================

/**
 * 推奨される状態コード (binary_display で使用):
 * 
 * タスク状態を2進数で表現:
 *   0b000 (0): アイドル状態
 *   0b001 (1): タスク実行中
 *   0b010 (2): データ受信待ち
 *   0b011 (3): データ処理中
 *   0b100 (4): エラー検出
 *   0b101 (5): 警告状態
 *   0b110 (6): 完了/準備完了
 *   0b111 (7): 特殊状態/デバッグ
 */

#define DEBUG_STATE_IDLE 0b000       // アイドル
#define DEBUG_STATE_RUNNING 0b001     // 実行中
#define DEBUG_STATE_WAITING 0b010     // 待機中
#define DEBUG_STATE_PROCESSING 0b011  // 処理中
#define DEBUG_STATE_ERROR 0b100       // エラー
#define DEBUG_STATE_WARNING 0b101     // 警告
#define DEBUG_STATE_READY 0b110       // 準備完了
#define DEBUG_STATE_DEBUG 0b111       // デバッグ状態

// ================= ハイブリッドデバッグマクロ =================

/**
 * デバッグ出力マクロ：Serial + LED の組み合わせ
 * 
 * 使用例:
 *   DEBUG_OUTPUT_HYBRID(DEBUG_STATE_RUNNING, "[INPUT] Reading sensor");
 *   DEBUG_OUTPUT_HYBRID(DEBUG_STATE_ERROR, "[ERROR] CAN timeout");
 */

#define DEBUG_OUTPUT_HYBRID(state, message) \
    do { \
        set_task_debug_state(state); \
        Serial.println(message); \
    } while(0)

#define DEBUG_OUTPUT_NO_BLOCK(state, message) \
    do { \
        set_task_debug_state(state); \
        Serial.print(message); \
    } while(0)

// ================= パルス点灯パターン（タスク用） =================

/**
 * @brief タスク内で周期的に呼び出してLED をパルス表示
 * @param pin GPIO ピン番号
 * @param cycle_ms 点灯周期(ms)
 * @param uptime_ms システム起動時間(ms)
 * 
 * 使用例:
 *   TickType_t start_time = xTaskGetTickCount();
 *   while(1) {
 *       uint32_t uptime = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
 *       pulse_led_in_task(DEBUG_LED_PIN_1, 500, uptime);
 *       vTaskDelay(pdMS_TO_TICKS(100));
 *   }
 */
inline void pulse_led_in_task(uint8_t pin, uint16_t cycle_ms, uint32_t uptime_ms) {
    // cycle_ms の半周期で On/Off を切り替え
    uint32_t phase = (uptime_ms % cycle_ms) < (cycle_ms / 2);
    digitalWrite(pin, phase ? HIGH : LOW);
}

/**
 * @brief 複数周期でLED をスキャン表示（状態モニタリング用）
 * @param led1, led2, led3 各GPIO ピン
 * @param speed_ms 各LED の点灯時間(ms)
 * @param uptime_ms システム起動時間(ms)
 * 
 * 3つのLED が順番に点灯するナイトライダー風パターン
 */
inline void scan_led_pattern(uint8_t led1, uint8_t led2, uint8_t led3, 
                             uint16_t speed_ms, uint32_t uptime_ms) {
    uint32_t pattern = (uptime_ms / speed_ms) % 6;
    
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    
    if (pattern < 3) {
        digitalWrite(led1, pattern == 0 ? HIGH : LOW);
        digitalWrite(led2, pattern == 1 ? HIGH : LOW);
        digitalWrite(led3, pattern == 2 ? HIGH : LOW);
    } else {
        digitalWrite(led1, pattern == 5 ? HIGH : LOW);
        digitalWrite(led2, pattern == 4 ? HIGH : LOW);
        digitalWrite(led3, pattern == 3 ? HIGH : LOW);
    }
}

// ================= 使用例 =================

/**
 * 例1: INPUT_Task での使用
 * 
 * void Input_Task(void *param) {
 *     TickType_t xLastWakeTime = xTaskGetTickCount();
 *     
 *     while(1) {
 *         if(receive_data()) {
 *             DEBUG_OUTPUT_HYBRID(DEBUG_STATE_PROCESSING, "[INPUT] Data received");
 *             process_data();
 *             DEBUG_OUTPUT_HYBRID(DEBUG_STATE_READY, "[INPUT] Data ready");
 *         } else {
 *             set_task_debug_state(DEBUG_STATE_WAITING);
 *         }
 *         vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
 *     }
 * }
 * 
 * 
 * 例2: M3508_Task でのハートビート表示
 * 
 * void M3508_Task(void *param) {
 *     TickType_t start_time = xTaskGetTickCount();
 *     
 *     while(1) {
 *         uint32_t uptime = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
 *         
 *         // 500msサイクルで点灯/消灯
 *         pulse_led_in_task(DEBUG_LED_PIN_3, 500, uptime);
 *         
 *         process_motor();
 *         vTaskDelay(pdMS_TO_TICKS(50));
 *     }
 * }
 * 
 * 
 * 例3: エラーハンドリング
 * 
 * if(error_detected) {
 *     DEBUG_OUTPUT_HYBRID(DEBUG_STATE_ERROR, "[ERROR] CAN transmission failed");
 *     set_debug_led_on(DEBUG_LED_PIN_1);  // LED1を常時点灯でエラー状態を示す
 * }
 */

//#endif /* DEBUG_HELPER_HPP */
