# デバッグLED システム使用ガイド

シリアルモニタの代わりにLED の点灯パターンでシステムの状態をデバッグします。

## ハードウェア設定

### 使用するGPIOピン

| ピン番号 | 用途 | 対応LED |
|---------|------|---------|
| GPIO 32 | DEBUG_LED_PIN_1 | LED1（CAN、初期化状態） |
| GPIO 33 | DEBUG_LED_PIN_2 | LED2（動作モード） |
| GPIO 25 | DEBUG_LED_PIN_3 | LED3（タスク作成、進行状況） |

### LED接続方法

各GPIO ピンに対して以下のように接続してください：

```
ESP32ピン → 330Ω抵抗 → LED のアノード
LED のカソード → GND
```

## LED点灯パターン

### パターン1: 短い点灯（100ms）
**意味**: 正常系・成功・進行状況通知
- 回数でさらに詳細な情報を表現

**例**：
- 1回の短点灯 → 初期化開始/進行状況確認
- 2回の短点灯 → CAN初期化成功
- 3回の短点灯 → タスク作成成功

### パターン2: 長い点灯（500ms）
**意味**: エラー状態・警告
- 長い点灯が繰り返される = エラー

**例**：
- 3回の長点灯 → CAN初期化失敗

### パターン3: パルス出力（デバイスID表示）
**意味**: 数値情報をLED点灯回数で表現
- 点灯回数 = 値

**例**：
- デバイスID = 1 → 1回の短点灯
- デバイスID = 2 → 2回の短点灯
- デバイスID = 3 → 3回の短点灯

### パターン4: 複数LED の同時点灯（2進数表現）
**意味**: 複数のステータスを同時に表現
```
LED1 (GPIO32) = bit 0
LED2 (GPIO33) = bit 1
LED3 (GPIO25) = bit 2
```

**例**：
- `0b101` → LED1とLED3が点灯、LED2は消灯

## 起動シーケンス

### 正常起動時の LED点灯パターン

1. **初期化開始**
   - LED1: 短点灯1回（初期化開始）

2. **CAN初期化**
   - 成功時: LED1: 短点灯2回
   - 失敗時: LED1: 長点灯3回（この場合、以降の初期化は中止）

3. **デバイスID表示**
   - LED2: パルス出力（DEVICE_ID の値分、短点灯）

4. **モード選択**
   - LED2: 短点灯複数回でモードを表示
     - 1回 → MODE_OUTPUT
     - 2回 → MODE_INPUT
     - 3回 → MODE_IO
     - 4回 → MODE_ROBOMAS
     - 5回 → MODE_ROBOMAS_PLUS_OUTPUT
     - 6回 → MODE_ROBOMAS_PLUS_INPUT
     - 7回 → MODE_ROBOMAS_PLUS_IO
     - 8回 → MODE_DEBUG

5. **タスク作成**
   - LED3: 各タスク作成完了時に短点灯1回

6. **セットアップ完了**
   - LED1, LED2, LED3: すべて短点灯1回ずつ（起動完了）

## 使用例

### デバッグLED 関数一覧

```cpp
// 初期化
debug_led_init();

// 短い点灯（成功系）
debug_flash_short(DEBUG_LED_PIN_1);          // 1回
debug_flash_short(DEBUG_LED_PIN_2, 2);       // 2回

// 長い点灯（エラー系）
debug_flash_long(DEBUG_LED_PIN_1);           // 1回
debug_flash_long(DEBUG_LED_PIN_1, 3);        // 3回

// 数値をパルス出力で表示
debug_pulse_value(DEBUG_LED_PIN_1, 5);       // 5回の短点灯

// 複数LEDで2進数表現
debug_binary_display(0b101);                 // LED1と3を点灯

// すべてのLEDを制御
debug_led_all_on();                          // すべて点灯
debug_led_all_off();                         // すべて消灯
```

## トラブルシューティング

### LED が反応しない場合

1. **GPIOピンの確認**
   - `debug_led.hpp` の`DEBUG_LED_PIN_x` の定義が正しいか確認
   - ESP32 の対応ピンリストを確認（gpio 6-8、11 は使用不可）

2. **配線の確認**
   - LED とGPIO ピンが正しく接続されているか確認
   - 抵抗値が適切か（330Ω推奨）
   - カソード・アノード向きは正しいか

3. **ビルド・書き込みの確認**
   - 最新の`main.cpp` がビルドされているか確認
   - `debug_led.hpp` がプロジェクトに含まれているか確認

4. **電力の確認**
   - ESP32 に十分な電力が供給されているか確認
   - 複数LED を同時に点灯させる場合、電力不足がないか確認

## LED点灯パターンリファレンス

| 状況 | LED1 | LED2 | LED3 | 意味 |
|------|------|------|------|------|
| 起動開始 | 短1回 | - | - | 初期化開始 |
| CAN成功 | 短2回 | - | - | CAN初期化成功 |
| CAN失敗 | 長3回 | - | - | CAN初期化エラー（致命的） |
| ID表示 | - | パルス | - | デバイスIDを点灯数で表示 |
| モード表示 | - | 短(n回) | - | 動作モードを短点灯数で表示 |
| タスク作成 | - | - | 短1回 | タスク作成完了 |
| 起動完了 | 短1回 | 短1回 | 短1回 | セットアップ完了 |

## カスタマイズ

### タイミングの調整

[debug_led.hpp](debug_led.hpp) の以下の定義を変更して点灯時間を調整できます：

```cpp
#define DEBUG_SHORT_FLASH 100   // 短点灯：100ms
#define DEBUG_LONG_FLASH 500    // 長点灯：500ms
#define DEBUG_PULSE_INTERVAL 100 // パルス間隔：100ms
```

### ピン番号の変更

config 環境に応じて以下を変更してください：

```cpp
#define DEBUG_LED_PIN_1 32  // ← ピン番号変更
#define DEBUG_LED_PIN_2 33  // ← ピン番号変更
#define DEBUG_LED_PIN_3 25  // ← ピン番号変更
```

## 注意事項

- **シリアル出力は継続**：LED デバッグシステムはシリアル出力に加えて機能します。Serial.println() も引き続き使用可能です。
- **複数イベント時の確認**：同時に複数のイベントが起きた場合、最後のパターンが残ります。
- **周期的な点灯**：メインループ内で頻繁にLED を制御するとちらつきが生じる可能性があります。

## 参考資料

- [ESP32 GPIO ピンリスト](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [Arduino digitalWrite() リファレンス](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/)
