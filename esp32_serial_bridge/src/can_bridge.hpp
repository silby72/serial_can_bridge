/*====================================================================
<can_bridge.hpp>
・マイコン間CAN通信ブリッジのヘッダーファイル
・ESP32無印 + MCP2561 を使用
・ESP-IDF組み込みTWAIドライバ使用（外部ライブラリ不要）
・main.cpp を改造せずに使用可能（自動初期化）

使用方法:
  config.hpp で IS_MASTER を定義するとMasterとして動作，
  未定義の場合はSlaveとして動作

  begin() は setup() 内で明示的に呼ぶか，
  未呼び出しの場合は sendDataToBus() / receiveDataFromBus() の
  初回呼び出し時に自動で初期化される．

Master: Rx_16Data[] → CAN送信 (sendDataToBus)
Slave:  CAN受信    → Rx_16Data[] に格納 (receiveDataFromBus)

【配線】（MCP2561）
  ESP32 GPIO(CAN_BRIDGE_TX_PIN) → MCP2561 TXD (pin1)
  ESP32 GPIO(CAN_BRIDGE_RX_PIN) ← MCP2561 RXD (pin4)
  MCP2561 VDD  → 5V
  MCP2561 VSS  → GND
  MCP2561 CANH → バスのCANH（両端に120Ω終端抵抗）
  MCP2561 CANL → バスのCANL
  MCP2561 RS(pin8) → GND  ← 必須！未接続だとバスオフになる

【フレームID割り当て】（1フレーム = int16_t × 4個 = 8バイト）
  0x100: Rx_16Data[ 0~ 3]
  0x101: Rx_16Data[ 4~ 7]
  0x102: Rx_16Data[ 8~11]
  0x103: Rx_16Data[12~15]
  0x104: Rx_16Data[16~19]
  0x105: Rx_16Data[20~23]

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once

#include <Arduino.h>
#include "config.hpp"
#include "frame_data.hpp"

namespace CanBridge {

    constexpr uint32_t BASE_ID        = 0x100;
    constexpr uint8_t  DATA_PER_FRAME = 4;
    constexpr uint8_t  NUM_FRAMES     = (Rx16NUM + DATA_PER_FRAME - 1) / DATA_PER_FRAME;

    bool begin();              // 明示的初期化（main.cppから呼ぶ場合）
    void sendDataToBus();      // Master用: Rx_16Data[] → CAN送信
    void receiveDataFromBus(); // Slave用:  CAN受信 → Rx_16Data[]

} // namespace CanBridge