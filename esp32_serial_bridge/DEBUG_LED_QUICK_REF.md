# デバッグLED クイックリファレンス

## ハードウェア配置

```
ESP32:
  GPIO 32 (DEBUG_LED_PIN_1) ──→ 抵抗330Ω ──→ LED1(赤) ──→ GND
  GPIO 33 (DEBUG_LED_PIN_2) ──→ 抵抗330Ω ──→ LED2(黄) ──→ GND
  GPIO 25 (DEBUG_LED_PIN_3) ──→ 抵抗330Ω ──→ LED3(緑) ──→ GND
```

## 起動シーケンス（正常時）

```
① LED1: 短点灯1回    (初期化開始)
       ↓
② LED1: 短点灯2回    (CAN初期化成功)
       ↓
③ LED2: パルス出力   (デバイスID表示、例：DEVICE_ID=1なら1回)
       ↓
④ LED2: 短点灯n回    (モード選択)
       ├─ 1回: MODE_OUTPUT
       ├─ 2回: MODE_INPUT
       ├─ 3回: MODE_IO
       ├─ 4回: MODE_ROBOMAS
       ├─ 5回: MODE_ROBOMAS_PLUS_OUTPUT
       ├─ 6回: MODE_ROBOMAS_PLUS_INPUT
       ├─ 7回: MODE_ROBOMAS_PLUS_IO
       └─ 8回: MODE_DEBUG
       ↓
⑤ LED3: 短点灯1回    (各タスク作成完了)
         [繰り返し]   (複数タスク存在する場合)
       ↓
⑥ LED1,2,3: 短点灯1回  (セットアップ完了！)
```

## トラブルシューティング

### ケース1: LED1が長点灯3回で止まる
```
症状: LED1が500msの長点灯を3回繰り返す
原因: CAN初期化失敗
対策:
  - CAN_BUS に何か接続されているか確認
  - CANバスの配線(TX/RX)を確認
  - CAN_BRIDGE_TX_PIN(GPIO4), RX_PIN(GPIO5) を確認
```

### ケース2: LED が全く反応しない
```
症状: LED がまったく点灯しない
対策:
  1. LED と配線の確認
     - LED のアノード/カソード向きは正しいか？
     - 抵抗値は330Ω か？
  2. ESP32 電源の確認
     - ESP32 に電源は供給されているか？
     - USB-UART の接続は正しいか？
  3. GPIO ピンの確認
     - debug_led.hpp の PIN定義を確認
     - 対応ピン番号はESP32で使用可能か？
```

### ケース3: 途中で LED が止まる
```
症状: 起動途中でLED点灯がパターンで止まる
原因: 通常、その地点でロックしている
対策:
  - シリアルモニタでエラーメッセージを確認
  - 該当タスク/初期化の settings を確認
  - config.hpp のモード設定を確認
```

## LED 点灯コード例

### コード内で LED を制御する場合

```cpp
#include "debug_led.hpp"

void setup() {
    debug_led_init();  // 初期化必須
    
    // 成功時
    debug_flash_short(DEBUG_LED_PIN_1);
    
    // エラー時
    debug_flash_long(DEBUG_LED_PIN_1, 3);
    
    // 数値表示（1～9推奨）
    debug_pulse_value(DEBUG_LED_PIN_2, error_code);
}

// ループ内での使用例
void some_task() {
    if (is_ok) {
        debug_flash_short(DEBUG_LED_PIN_3);
    } else {
        debug_flash_long(DEBUG_LED_PIN_3, 2);
    }
}
```

## 利用可能なGPIO ピン

ESP32 では以下のピンが通常利用可能です：
- 利用推奨: 25, 26, 27, 32, 33 
- 利用不可: 6, 7, 8, 11 (内部フラッシュに使用)
- PWM用に予約されたピン: 19, 21, 22, 23, 25, 26, 27
  (config.hpp でサーボに使用中)

## 関連ファイル

- [`src/debug_led.hpp`](src/debug_led.hpp) - デバッグLED ライブラリ
- [`src/main.cpp`](src/main.cpp) - LED 統合メインプログラム
- [`DEBUG_LED_GUIDE.md`](DEBUG_LED_GUIDE.md) - 詳細ガイド
- [`src/config.hpp`](src/config.hpp) - 設定ファイル

## パフォーマンス考慮事項

- LED フラッシュ関数は `delay()` を内部で使用するため、タスク実行中は使用しない
- FreeRTOS タスク内では `vTaskDelay()` を使用してください
- `debug_binary_display()` は delay を使用しないので、タイマー割り込み内で使用可能

---
**作成日**: 2026年5月11日  
**プロジェクト**: esp32_serial_bridge
