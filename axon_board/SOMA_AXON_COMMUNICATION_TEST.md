# SOMA-AXON間通信テストガイド

## 概要
SOMA側からAXON側への通信プロトコルとテスト手順

## パケット構造

### uart_s2a_packet_t (3バイト)
```
+--------+----------+--------+
| Header | Command  |  Data  |
| 1 byte | 1 byte   | 1 byte |
+--------+----------+--------+
```

- **Header**: `0xFF` (固定値)
- **Command**: コマンドID (下記参照)
- **Data**: コマンドに応じたデータ

## コマンド一覧

### 1. GET_STATUS_REQ (0x00) - 状態取得要求

AXON側の現在の状態を取得します。

**SOMA → AXON**
```
Header:  0xFF
Command: 0x00
Data:    0x00 (任意、無視される)
```

**AXON → SOMA (応答: GET_STATUS_RESP 0x01)**
```
Header:  0xFF
Command: 0x01
Data:    状態データ (bit構成は下記参照)
```

### 2. SET_STATUS_REQ (0x02) - 状態設定要求

AXON側の状態を設定します。

**SOMA → AXON**
```
Header:  0xFF
Command: 0x02
Data:    設定データ (bit構成は下記参照)
```

**AXON → SOMA (応答: SET_STATUS_RESP 0x03)**
```
Header:  0xFF
Command: 0x03
Data:    設定したデータをそのまま返す (ACK)
```

## Data フィールドのビット構成

```
bit 7 6 5 4 | 3 2 | 1       | 0
------------+-----+---------+-------------
amount      | led | sol     | dial_detect
(0-15)      |(0-3)| (0/1)   | (0/1)
```

### 各フィールドの説明

- **amount (bit 7-4)**: 7セグメントLED表示用の数値 (0-15)
  - 現在は使用されていない (0固定)

- **led_state (bit 3-2)**: LED状態
  - `0b00` (0): OFF
  - `0b01` (1): PWM (SLOW)
  - `0b10` (2): PWM (FAST)
  - `0b11` (3): ON

- **sol_state (bit 1)**: ソレノイド状態
  - `0`: OFF
  - `1`: ON

- **dial_detect (bit 0)**: ハンドル回転検出
  - `0`: 未検出
  - `1`: 検出済み

## 通信テストシナリオ

### テスト1: 基本的な状態取得

**目的**: AXON側の現在の状態を取得する

**送信データ (SOMA → AXON)**
```
Hex:  FF 00 00
Dec:  255, 0, 0
Bin:  11111111 00000000 00000000
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 01 XX
      (XXは現在の状態による)
```

### テスト2: ソレノイドON

**目的**: ソレノイドを駆動する

**送信データ (SOMA → AXON)**
```
Hex:  FF 02 02
Dec:  255, 2, 2
Bin:  11111111 00000010 00000010

Data詳細 (0x02 = 0b00000010):
  amount: 0
  led_state: 0 (OFF)
  sol_state: 1 (ON)  ← ソレノイドON
  dial_state: 0
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 03 02
      (設定値をそのまま返す)
```

**確認事項**:
- AXON基板のソレノイド(BLOCK_SOL)が駆動される
- AXON側の状態が STATE_SOL_ON に遷移
- ハンドル回転検出が開始される

### テスト3: ソレノイドOFF

**目的**: ソレノイドを停止する

**送信データ (SOMA → AXON)**
```
Hex:  FF 02 00
Dec:  255, 2, 0
Bin:  11111111 00000010 00000000

Data詳細 (0x00 = 0b00000000):
  amount: 0
  led_state: 0 (OFF)
  sol_state: 0 (OFF)  ← ソレノイドOFF
  dial_state: 0
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 03 00
```

### テスト4: LED点灯制御

**目的**: LEDを点灯させる

**送信データ (SOMA → AXON)**
```
Hex:  FF 02 0C
Dec:  255, 2, 12
Bin:  11111111 00000010 00001100

Data詳細 (0x0C = 0b00001100):
  amount: 0
  led_state: 3 (ON)  ← LED ON
  sol_state: 0 (OFF)
  dial_state: 0
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 03 0C
```

### テスト5: LED + ソレノイド同時制御

**目的**: LEDを点灯しながらソレノイドを駆動

**送信データ (SOMA → AXON)**
```
Hex:  FF 02 0E
Dec:  255, 2, 14
Bin:  11111111 00000010 00001110

Data詳細 (0x0E = 0b00001110):
  amount: 0
  led_state: 3 (ON)  ← LED ON
  sol_state: 1 (ON)  ← ソレノイドON
  dial_state: 0
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 03 0E
```

### テスト6: ハンドル回転検出確認

**目的**: ハンドル回転が検出されているか確認

**手順**:
1. テスト2でソレノイドをON
2. 手動でハンドルを回転
3. 状態取得コマンドを送信

**送信データ (SOMA → AXON)**
```
Hex:  FF 00 00
```

**期待される応答 (AXON → SOMA)**
```
Hex:  FF 01 01
      (dial_detectビットが1)

Data詳細 (0x01 = 0b00000001):
  amount: 0
  led_state: 0
  sol_state: 0 (自動でOFFになる)
  dial_detect: 1 (検出済み)  ← これを確認
```

## AXON側の状態遷移

```
[STATE_NOTIFY] ← 起動時、または定期通知時
     ↓ GET_STATUS_REQ受信
[STATE_NORMAL] ← 通常状態
     ↓ SET_STATUS_REQ (sol_state=1)
[STATE_SOL_ON] ← ソレノイドON、ハンドル回転検出待ち
     ↓ ハンドル回転検出
[STATE_DIAL_DETECT] ← 回転検出完了
     ↓ 定期通知タイマー
[STATE_NOTIFY] ← IRQ信号をSOMAに通知
```

## IRQ信号について

AXON側は以下の条件でSOMAにIRQ信号を送信します:
- 起動時
- ハンドル回転検出時
- 500ms間コマンドを受信しなかった場合

IRQ信号検出後、SOMA側は `GET_STATUS_REQ` を送信して状態を取得してください。

## デバッグ用データ計算ツール

### Dataバイト生成式
```c
data = (amount << 4) | (led_state << 2) | (sol_state << 1) | dial_detect;
```

### 例
```
amount=0, led=0, sol=1, dial=0
→ data = (0 << 4) | (0 << 2) | (1 << 1) | 0
→ data = 0x02

amount=0, led=3, sol=1, dial=0
→ data = (0 << 4) | (3 << 2) | (1 << 1) | 0
→ data = 0x0E
```

## トラブルシューティング

### 応答が返ってこない
- UART接続を確認 (ボーレート、結線)
- AXON側のUART初期化が完了しているか確認
- タイムアウト設定を確認

### 不正なデータが返ってくる
- パケットヘッダーが `0xFF` であることを確認
- コマンドIDが正しいか確認
- RXバッファのフラッシュを検討

### ソレノイドが動作しない
- 電源供給を確認
- GPIO設定を確認 (BLOCK_SOL_PORT/PIN)
- ハードウェア接続を確認

## 通信仕様

- **ボーレート**: `UART_BAUD_RATE` (driver_config.hで定義)
- **データビット**: 8bit
- **パリティ**: なし
- **ストップビット**: 1bit
- **フロー制御**: なし
- **タイムアウト**: 受信処理による

## 関連ファイル

- `uart_packet.h` - パケット定義、プロトコル定義
- `uart_packet.c` - 送受信処理実装
- `axon_routine.c` - コマンド処理、状態管理
