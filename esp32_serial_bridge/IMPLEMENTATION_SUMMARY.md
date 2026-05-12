# LED デバッグシステム実装概要

## 概要

シリアルモニタが文字化けする問題を解決するため、**GPIO ピンを使ったLED デバッグシステム**を実装しました。

デジタルライト（LED）の**点灯パターン**で、シリアル出力の代わりにシステムの状態を視覚的に確認できます。

## 実装内容

### 1. 追加ファイル

#### [`src/debug_led.hpp`](src/debug_led.hpp)
デバッグLED ライブラリのコアファイル
- **機能**:
  - `debug_led_init()`: 初期化
  - `debug_flash_short(pin, count)`: 短い点灯（成功系）
  - `debug_flash_long(pin, count)`: 長い点灯（エラー系）
  - `debug_pulse_value(pin, value)`: パルス出力で数値表示
  - `debug_binary_display(value)`: 複数LED で2進数表現
  - `debug_led_all_on/off()`: 全LED 制御
- **使用ピン**: GPIO 32, 33, 25

#### [`src/debug_helper.hpp`](src/debug_helper.hpp)
FreeRTOS タスク内での非ブロッキング使用用ヘルパー関数
- **機能**:
  - `set_task_debug_state()`: ノンブロッキングでLED 状態設定
  - `toggle_debug_led()`: LED トグル
  - `pulse_led_in_task()`: タスク内でのパルス表示
  - `scan_led_pattern()`: ナイトライダーパターン
  - 状態定義マクロ（IDLE, RUNNING, WAITING, ERROR など）
  - ハイブリッドデバッグマクロ（Serial + LED）

#### [`DEBUG_LED_GUIDE.md`](DEBUG_LED_GUIDE.md)
詳細なユーザーガイド
- ハードウェア接続図
- LED 点灯パターン説明
- 起動シーケンス図
- トラブルシューティング
- カスタマイズ方法

#### [`DEBUG_LED_QUICK_REF.md`](DEBUG_LED_QUICK_REF.md)
クイックリファレンス・チートシート
- ハードウェア配置図
- 起動シーケンスフロー
- トラブルシューティングテーブル
- 使用例コード

### 2. 修正ファイル

#### [`src/main.cpp`](src/main.cpp)
main.cpp にLED デバッグシステムを統合
- **追加**: `#include "debug_led.hpp"`
- **setup() 内の変更**:
  - `debug_led_init()`: LED システム初期化
  - CAN 初期化: 成功時に短点灯2回、失敗時に長点灯3回
  - デバイスID 表示: パルス出力
  - モード選択: 短点灯回数でモードを表示
  - タスク作成: 各タスク作成完了時にLED 点灯
  - セットアップ完了: すべてのLED で確認

## ハードウェア接続

### GPIO ピン配置

```
ESP32 Module
┌─────────────────────┐
│  GPIO32 ──→ [330Ω] ──→ LED1(赤) ──→ GND
│  GPIO33 ──→ [330Ω] ──→ LED2(黄) ──→ GND
│  GPIO25 ──→ [330Ω] ──→ LED3(緑) ──→ GND
└─────────────────────┘
```

### 必要な部品

- LED × 3個（色は任意、赤/黄/緑推奨）
- 抵抗 330Ω × 3個（LED の制限抵抗）
- ジャンパーケーブル

## LED 点灯パターン

### 基本パターン

| パターン | 時間 | 意味 |
|---------|------|------|
| 短点灯(100ms) | 1回 | 進行中 |
| 短点灯 | n回 | 数値情報（n = 値） |
| 長点灯(500ms) | 1～n回 | エラー |

### 起動シーケンス（正常時）

```
① LED1: 短1回  → 初期化開始
② LED1: 短2回  → CAN 初期化成功
③ LED2: パルス → デバイスID 表示
④ LED2: 短n回  → モード選択表示
⑤ LED3: 短1回  → タスク作成完了(繰り返し)
⑥ LED1,2,3: 短1回 → セットアップ完了！
```

## 使用方法

### セットアップ内での使用

```cpp
#include "debug_led.hpp"

void setup() {
    debug_led_init();  // 初期化必須
    
    // 成功確認
    debug_flash_short(DEBUG_LED_PIN_1);
    
    // エラー表示
    if(error) {
        debug_flash_long(DEBUG_LED_PIN_1, 3);
    }
}
```

### タスク内での使用

```cpp
#include "debug_helper.hpp"

void task_example(void *param) {
    while(1) {
        // ノンブロッキング状態表示
        DEBUG_OUTPUT_HYBRID(DEBUG_STATE_RUNNING, "[TASK] Processing");
        
        // パルス表示
        uint32_t uptime = get_uptime_ms();
        pulse_led_in_task(DEBUG_LED_PIN_3, 500, uptime);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## デバッグ方法

### 1. 簡単なデバッグ（セットアップ時のみ）

`debug_led.hpp` の関数を`setup()` で直接呼び出し

### 2. 高度なデバッグ（タスク内での使用）

`debug_helper.hpp` のノンブロッキング関数を使用
- `delay()` 不要
- FreeRTOS タスク内で安全に使用可能
- Serial.println() と組み合わせて使用

### 3. リアルタイムモニタリング

`scan_led_pattern()` でシステムの周期的な活動を確認
- 3つのLED が順番に点灯
- システムが生きているかの確認

## トラブルシューティング

### LED が点灯しない場合

1. ピン番号の確認（`debug_led.hpp` の定義）
2. LED とGND の配線確認
3. 抵抗値が330Ω か確認
4. ESP32 電源供給の確認

### 特定のLED だけ反応しない

1. ピン番号の対応確認
2. LED が破損していないか（別のLED で試す）
3. GPIO ピンが他の機能と競合していないか確認

### LED が頻繁にちらつく

- メインループ内での頻繁なLED 制御を見直す
- タスク内ではノンブロッキング関数を使用

## 関連ドキュメント

- [`DEBUG_LED_GUIDE.md`](DEBUG_LED_GUIDE.md) - 詳細なユーザーガイド
- [`DEBUG_LED_QUICK_REF.md`](DEBUG_LED_QUICK_REF.md) - クイックリファレンス
- [`src/debug_led.hpp`](src/debug_led.hpp) - ライブラリコード
- [`src/debug_helper.hpp`](src/debug_helper.hpp) - ヘルパー関数コード
- [`src/main.cpp`](src/main.cpp) - 統合されたメインプログラム

## パフォーマンス

- **メモリ使用**: 最小限（inline 関数のみ）
- **CPU 使用**: delay() によるブロッキング時間のみ
- **応答性**: ノンブロッキング関数使用で最小化

## 今後の拡張可能性

1. **ネットワーク経由のモニタリング**
   - Bluetooth LE で LED 状態をスマートフォンに送信

2. **複数マイコン間の状態共有**
   - CAN バスで複数ESP32 の状態をモニタリング

3. **ログシステムとの連携**
   - SPIFFS にLED イベントログを記録

4. **Webダッシュボード**
   - リアルタイムでLED 状態を可視化

## 注意事項

- ✅ Serial.println() は引き続き使用可能
- ✅ LED デバッグと通常のシリアル出力は共存
- ❌ Setup() 内で `debug_flash_*()` を使用時のみ delay() が発生
- ❌ FreeRTOS タスク内では必ず `debug_helper.hpp` のノンブロッキング関数を使用

---

**実装日**: 2026年5月11日  
**プロジェクト**: esp32_serial_bridge  
**対応ボード**: ESP32 Dev Module
