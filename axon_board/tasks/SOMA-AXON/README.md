# SOMA–AXON タスク一覧（ESP32-C6版）

## 概要
このディレクトリには、SOMA基板（ESP32-C6）とAXON基板（TI MSPM0）間の通信・制御に関するタスク定義が含まれています。

**注意**: 本実装はESP32-C6ベースのため、元の仕様書（TI MSPM0ベース）とハードウェア構成が異なります。
- マイコン: TI MSPM0 → ESP32-C6
- IRQ入力: 直接GPIO → IOExpander経由
- FRAM: SPI接続 → I2C接続（予定）

## タスク構成

### データ管理
- **FRAM仕様** [FRAM保存内容](./FRAM_Specification.md)
  - PORT番号・FACE番号・金額・カウンタ管理
  - API仕様（fram_update_entry, fram_load_entry等）

### IRQ監視（ESP32-C6固有）
- **0.1** [AXON IRQ監視システム](./0.1_AXON_IRQ_Monitoring.md) ✅
  - IOExpander Port1経由のIRQ入力（PORT 1-8）
  - GPIO9割り込み対応（PORT 9）※未実装
  - IRQ検出時のMUX自動切替
  - UART受信とコマンド解析

### 基本シーケンス
- **7.1** [PORT確認シーケンス](./7.1_PORT_Check_Sequence.md) ✅
  - CHKIRQ/ATIRQ通信
  - AXON情報取得・FRAM保存
  - FWバージョンチェック

- **7.2** [AXON設定Table伝搬](./7.2_AXON_Config_Table.md) 🔶
  - FRAMからの設定読み出し
  - SETAXON/ACK通信
  - 全ポート一括設定

### 設定変更
- **7.3** [金額・面番号変更の事前準備](./7.3_Change_Preparation.md)
  - ボタン状態監視
  - FRAM差分検出
  - 変更モード判定

- **7.4** [面番号変更（データ有）](./7.4_Face_Change_with_Data.md)
  - FACE重複チェック
  - 既存設定引継ぎ
  - SETAXON設定反映

- **7.5** [面番号変更（0面利用）](./7.5_Face_Change_Zero.md)
  - 0面設定（無効化）
  - データ保持・復帰処理

### 動作制御
- **7.6** [ダイヤル回転サブルーチン](./7.6_Dial_Rotation.md)
  - ROT_DET/BLK_ON監視
  - 回転検出
  - FRAMカウンタ+1

- **7.7** [通常動作（購入）](./7.7_Normal_Purchase.md)
  - 現金購入（AXON自律）
  - キャッシュレス購入（SOMA制御）
  - FRAMカウンタ増加

- **7.8** [通常動作（購入以外）](./7.8_Normal_Non_Purchase.md)
  - 現金返金処理
  - キャッシュレス中止
  - 処理未了対応

- **7.9** [運用動作](./7.9_Operation_Mode.md)
  - 0円設定
  - 売り切れ管理
  - メンテナンスモード

## 実装状況サマリ（ESP32-C6版）

### 完了済み ✅
- ✅ CHKIRQ/ATIRQ通信プロトコル
- ✅ SETAXON/ACK通信プロトコル
- ✅ AXONポートスキャン機能（7.1）
- ✅ MUX切替機能
- ✅ CRC16計算・検証
- ✅ FRAM基本実装（静的配列版）
- ✅ IOExpander IRQ入力（PORT 1-8）
- ✅ IRQ監視タスク（50ms周期ポーリング）
- ✅ IRQ検出時のUART受信処理
- ✅ コマンドID解析

### 部分実装 🔶
- 🔶 FRAM API（基本機能のみ）
- 🔶 IRQ監視（ポーリング方式、割り込み未実装）
- 🔶 エラーハンドリング（基礎のみ）
- 🔶 SETAXON全ポート設定（手動実行のみ）

### 未実装 ⏳
- ⏳ GPIO9割り込み（PORT 9監視）
- ⏳ IOExpander INT割り込み（リアルタイムIRQ）
- ⏳ fram_increment_counter() API
- ⏳ fram_check_face_duplicate() API
- ⏳ ボタン状態監視機能
- ⏳ 回転検出状態機械（7.6）
- ⏳ 購入シーケンス状態機械（7.7）
- ⏳ 運用モード管理（7.9）
- ⏳ 売り切れ管理機能
- ⏳ I2C FRAM実装（現在は静的配列）
- ⏳ TG–SOMA通信連携

## コマンド一覧

| コマンド | ID | 方向 | 用途 |
|---------|----|----|------|
| CHKIRQ | 0x49 | SOMA→AXON | 状態確認要求 |
| ATIRQ | 0x6A | AXON→SOMA | 状態応答（AXON情報） |
| SETAXON | 0x4A | SOMA→AXON | AXON設定書き込み |
| ACK | 0x00 | AXON→SOMA | コマンド受信確認 |

## 状態ビットフィールド

```c
// ATIRQ status ビットフィールド
#define ROT_DET_BIT     (1 << 0)  // 回転検出
#define BLK_ON_BIT      (1 << 1)  // ブロック中
#define COIN_DET_BIT    (1 << 2)  // 硬貨検出
#define ESCRW_DET_BIT   (1 << 3)  // 返金検出
#define DOOR_OPEN_BIT   (1 << 4)  // ドア開放
#define SOLD_OUT_BIT    (1 << 5)  // 売り切れ
#define ERROR_STATE_BIT (1 << 6)  // エラー状態
#define MAINT_MODE_BIT  (1 << 7)  // メンテナンスモード
```

## フレーム構造

```
[Header(1)][Length(1)][Data(32)][CRC16(2)] = 36 bytes

Header: 0x14 (SOMA→AXON), 0x10 (AXON→SOMA)
Length: 0x20 (32 bytes)
Data:   コマンド依存
CRC16:  ISO/IEC 13239 (polynomial 0x8408)
```

## 依存関係ツリー

```
7.1 PORT確認
  ↓
7.2 設定伝搬
  ↓
┌─────────┬─────────┐
↓         ↓         ↓
7.3 準備  7.6 回転  7.9 運用
  ↓         ↓
7.4/7.5    7.7 購入
  変更      ↓
           7.8 非購入
```

## 関連ドキュメント
- `/workspace/docs/SPEC_Basic.md` - 基本仕様書
- `/workspace/docs/SPEC_Sequence.md` - シーケンス仕様書（作成予定）
- `/workspace/README.md` - プロジェクト全体README
- `/workspace/tasks/TG-SOMA/` - TG–SOMA通信タスク

## テスト状況

### 最新テスト結果（2025-11-21）
```
Port 1: ✅ FACE #2, 100円, FW v0.5 (構造体修正前)
Port 2: ✅ FACE #3, 200円, FW v1.0 (構造体修正後)
Port 3-9: 未接続
```

**構造体アライメント問題:**
- Port 1（旧AXON）: `status`が2バイト → afw_ver誤読（v0.5）
- Port 2（新AXON）: `status`が1バイト → afw_ver正常（v1.0）✅

**結論:** SOMA側実装は正しく、AXON側構造体修正で解決確認済み

## 進捗管理
各タスクファイル内のチェックボックス（`- [ ]`）で進捗を管理してください。
完了した項目は `- [x]` に変更します。

## ESP32-C6版 実装ロードマップ

### Phase 1: 基本通信（完了） ✅
- ✅ CHKIRQ/ATIRQ通信
- ✅ SETAXON/ACK通信
- ✅ ポートスキャン（7.1）
- ✅ FRAM基本実装（静的配列）
- ✅ IRQ監視タスク（ポーリング）
- ✅ UART受信処理

### Phase 2: IRQ割り込み化（次のステップ）
1. **GPIO9割り込み実装** - PORT 9対応
2. **IOExpander INT割り込み** - リアルタイムIRQ検出
3. ISR + タスク分離設計
4. 応答遅延の最小化

### Phase 3: FRAM API拡張
1. **fram_increment_counter()** - 売上カウント
2. **fram_check_face_duplicate()** - 重複チェック
3. I2C FRAM移行（MB85RC256V等）
4. エラーリトライ処理

### Phase 4: 状態機械実装
1. **回転検出ロジック**（7.6） - ROT_DET/BLK_ON判定
2. **購入シーケンス**（7.7） - 現金/キャッシュレス
3. **ボタン監視**（7.3） - 短押し/長押し判定
4. 状態遷移エンジン

### Phase 5: 運用機能
1. 売り切れ管理（7.9）
2. 0円設定モード（7.9）
3. メンテナンスモード
4. TG通信連携

## ESP32-C6実装の特記事項

### ハードウェア差異
- **マイコン**: TI MSPM0 → ESP32-C6（RISC-V）
- **IRQ入力**: 直接GPIO（PA0-PA30） → IOExpander Port1（P1_0-P1_7） + GPIO9
- **FRAM接続**: SPI（PA8,PA12-PA14） → I2C予定（現在は静的配列）
- **UART**: MSPM0 UART → ESP32-C6 UART1

### 現在の制約
- IRQ監視は50msポーリング（割り込み未実装）
- PORT 9監視未対応（GPIO9割り込み未実装）
- LED制御未実装
- TG通信未実装

### 設計方針
- 通信プロトコルは元仕様準拠（CHKIRQ/ATIRQ/SETAXON/ACK）
- IOExpander経由でハードウェア差異を吸収
- FreeRTOSタスクベースの非同期処理
- CRC16による通信エラー検出
- 将来的な割り込み駆動への移行を考慮

## 備考
- SOMA–AXON通信プロトコルは実装完了済み
- TG–SOMA通信は別ディレクトリ（`/workspace/tasks/TG-SOMA/`）
- FRAMは現在静的配列、将来I2C FRAM化
- 全コマンドはplaintextモード（暗号化なし）
- 元仕様書（v0.1.3）はTI MSPM0ベース
