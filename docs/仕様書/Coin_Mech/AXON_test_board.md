# AXON用 コインメック 代替えテスト基板
  
## 概要
本資料は、AXON基板 CN3（PHD 2×9 / 18pin）に接続する
**スイッチ入力およびソレノイド代替LED出力を確認するためのテスト用基板**
について、回路・配線・レイアウトをまとめた最終版資料です。
  
---
  
## 1. AXON I/Fのピンアサイン
  
Table 3-2, AXON I/Fのピンアサイン情報
  
| PIN No. | 項目 | 内容 |
|---|---|---|
| 1 | ESCRW_DET_NO | 返金ボタン押下検出（ノーマリーオープン：5V PULL-UP）<br>0：検出（Lowパルス）<br>1：通常状態 |
| 2 | ESCRW_DET_GND | カプセル排出検出用 マイクロスイッチ用GND |
| 3 | COIN_VCC | 硬貨検出用 フォトセンサーVcc（24V/15mA） |
| 4 | COIN_DET | 硬貨検出（ノーマリーオープン：5V PULL-UP）<br>0：検出（Lowパルス）<br>1：通常状態 |
| 5 | GND | 基板GND |
| 6 | NC | Non connect |
| 7 | BLK_VCC | 硬貨用ブロックソレノイド用 Vcc（12V/100mA）<br>0：硬貨ブロック<br>1：硬貨投入許可 |
| 8 | BLK_ON | 回転検出（ノーマリーオープン：5V PULL-UP）(硬貨用ブロックソレノイド制御出力) <br>0：検出（50msのLowパルス）<br>1：通常状態 |
| 9 | 3.3V | 電源3.3V |
| 10 | INSERT_DET | PORT利用検出<br>0：OPEN<br>1：検出 |
| 11 | ROT_DET_NO | カプセル排出検出（ノーマリーオープン：5V PULL-UP）<br>0：検出（Lowパルス）<br>1：通常状態 |
| 12 | ROT_DET_GND | カプセル排出検出用 マイクロスイッチ用GND |
| 13 | SLD_OUT_DET_VCC | 売り切れ検出（オープンコレクタ出力：3.3V PULL-UP）<br>0：売り切れ状態（Lowレベル）<br>1：通常状態<br>**※テスト基板では入力（スイッチ）として使用** |
| 14 | GND | 基板GND |
| 15 | SOL_VCC | キャッシュレス決済用 ソレノイドVcc（5V/1A供給） |
| 16 | COIN_SOL | キャッシュレス決済用 ソレノイドON<br>0：決済未完了<br>1：決済完了 |
| 17 | DOOR_DET | カプセルトイ前面パネル開閉検知<br>0：通常状態（CLOSE状態）<br>1：解放状態（OPEN状態） |
| 18 | GND | 基板GND |
  
---
  
## 2. コネクタ構成
  
### AXON側
- CN3 : JST PHDシリーズ 2.0mm ピッチ
  2×9 / 18pin
  
### テスト基板側
- J_SW6 : JST XHシリーズ 2.5mm ピッチ / 1×6（スイッチ信号）
- J_LED4 : JST XHシリーズ 2.5mm ピッチ / 1×4（LED +3.3V + GND）
  
---
<br><br><br><br><br>  <br><br><br><br><br>
<br><br><br><br><br>

## 3. CN3 ピン割り当て表
  
| CN3 Pin | 信号名         | 方向 | 部品                    |
|--------:|:---------------|:----:|:------------------------|
|       1 | ESCRW_DET      | IN   | プッシュ スイッチ       |
|       4 | COIN_DET       | IN   | プッシュ スイッチ       |
|      10 | INSERT_DET       | IN   | スライド スイッチ       |
|      11 | ROT_DET        | IN   | プッシュ スイッチ       |
|      13 | SLDOUT_DET     | IN   | スライド スイッチ       |
|      17 | DOOR_DET       | IN   | スライド スイッチ       |
|       8 | BLK_SOL        | OUT  | LED Orange              |
|      16 | COIN_SOL       | OUT  | LED Bule                |
|       9 | +3.3V          | PWR  | LED電源                 |
|       2 | GND            | GND  | 共通GND                 |
  
### スイッチ動作定義
  
#### プッシュスイッチ（ESCRW_DET、COIN_DET、ROT_DET）
- 押下時：信号をGNDへ短絡（Low＝検出）
- 開放時：開放（High＝通常）
- ※AXON側はプルアップ前提、押下時に一時的なLowパルスが発生
  
#### スライドスイッチ（INSERT_DET、SLDOUT_DET、DOOR_DET）
- ON：信号をGNDへ短絡（Low＝検出）
- OFF：開放（High＝通常）
- ※AXON側はプルアップ前提、テスト基板側はGNDへ落とすのみ
- ※スライドSWは状態を保持する
  
#### LED出力（BLK_SOL、COIN_SOL）
- 信号がLowのとき点灯（3.3V出力時）
- 電流制限抵抗を介してGNDへ落とす構成
---

<br><br><br><br><br> <br><br>

## 4. 配線図（PHD18 → XH6 / XH4）
  
### J_SW6（スイッチ）
  
| CN3 Pin | 信号           | XH6 Pin |
|--------:|:---------------|--------:|
|       1 | ESCRW_DET      |       1 |
|       4 | COIN_DET       |       2 |
|      11 | ROT_DET        |       3 |
|      10 | INSERT_DET       |       4 |
|      13 | SLDOUT_DET     |       5 |
|      17 | DOOR_DET       |       6 |
  
**※注記：**
- スイッチのもう片側は **基板内GND共通**
- INSERT_DET / SLDOUT_DET / DOOR_DET はスライドSWで保持入力とする
- スライドSWは OFF=開放、ON=GND短絡
  
### J_LED4（LED + 電源）
  
| CN3 Pin | 信号         | XH4 Pin |
|--------:|:-------------|--------:|
|       8 | BLK_SOL      |       1 |
|      16 | COIN_SOL     |       2 |
|       9 | +3.3V        |       3 |
|       2 | GND          |       4 |
  
**※注記：**
- LEDは信号がLowで点灯（3.3V出力時）
- BLK_SOL（Pin8）：LED Orange
- COIN_SOL（Pin16）：LED Blue
  
---
  

<br><br><br><br><br>
<br><br><br><br><br>

## 5. レイアウト図（イメージ）
  
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                  AXON用 テスト基板                  ┃
┃                                                    ┃
┃          AXON基板へ接続                             ┃
┃  ┌──────────────────────────────┐                  ┃
┃  │ CN3  (JST PHD 2.0mm pitch)   │                  ┃
┃  │ PHD 2x9   18pin              │                  ┃
┃  └──────────────────────────────┘                  ┃
┃                                                    ┃
┃  [PUSH SWITCH]                                     ┃
┃     SW1          SW2          SW4                  ┃
┃     [○]          [○]          [○]                  ┃
┃  ESCRW_DET    COIN_DET     ROT_DET                 ┃
┃                                                    ┃
┃  [SLIDE SWITCH]                                    ┃
┃     SW3          SW5          SW6                  ┃
┃    [---]        [---]        [---]                 ┃
┃  INSERT_DET   SLDOUT_DET   DOOR_DET                ┃
┃                                                    ┃
┃  [LED OUTPUT CHECK]                                ┃
┃    LED1 (Orange)      LED2 (Blue)                  ┃
┃        ●                 ●                         ┃
┃     BLK_SOL          COIN_SOL                      ┃
┃  (Pin8:Low点灯)  (Pin16:Low点灯)                    ┃
┃                                                    ┃
┃  ┌─────────┐         ┌─────────┐                   ┃
┃  │  J_SW6  │         │ J_LED4  │                   ┃
┃  │  XH 1x6 │         │ XH 1x4  │                   ┃
┃  └─────────┘         └─────────┘                   ┃
┃  SWITCH SIGNAL        LED+POWER                    ┃
┃                                                    ┃
┃  ※ GND共通 / 3.3V電源使用                          ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

  
---
  
## 6. 注意事項
  
- 24V / 5V は使用しない
- GNDは1点共通で配線する
---
  
以上。
  