# Phase 2実機検証 - AXON側使用方法

## 📋 概要

Phase 2で実装した全機能（T2.1~T2.6）を実機で検証するためのAXON側コードです。

**検証対象機能**:
- T2.1: `fram_increment_counter()` API
- T2.2: ダイヤル回転検出（5状態機械）
- T2.3: 通常購入処理（現金/キャッシュレス）
- T2.4: 非購入処理（返金/中止/タイムアウト）
- T2.5: 面番号変更（長押し/重複防止/0面モード）
- T2.6: 運用動作（ドア開閉/売り切れ/LED制御）

---

## 🔧 実装ファイル

### 新規作成ファイル
1. **`axon_phase2_verification.h`** (330行)
   - 統計構造体: `axon_phase2_stats_t`
   - 制御構造体: `axon_phase2_config_t`
   - API関数: 27個

2. **`axon_phase2_verification.c`** (610行)
   - 統計記録関数の実装
   - レポート生成関数
   - 制御関数

3. **`tasks/SOMA-AXON/T2.7_Phase2_実機検証.md`** (320行)
   - タスク仕様書
   - 検証シーケンス
   - 合格基準

### 修正済みファイル
4. **`axon_routine.c`**
   - `#include "axon_phase2_verification.h"` 追加
   - `axon_phase2_verification_init(1)` 初期化追加
   - ドア開閉・売り切れ検知時の記録フック追加

5. **`protocol/src/s2a_packet.c`**
   - `#include "../../axon_phase2_verification.h"` 追加
   - CHKIRQ/ATIRQ/SETAXON/ACK/NACK/NOP受信時の記録フック追加
   - CRC異常時の記録フック追加

6. **`peripheral/fram_memory_map.c`**
   - `fram_increment_counter()` 内にオーバーフロー防止記録追加

---

## 🚀 使用方法

### 1. ビルド方法

```powershell
# CCS (Code Composer Studio) でプロジェクトをインポート
cd c:\DATA\DEVELOP\Git\capsule_toy_control_board_CCS\axon_board

# ビルド (CCS GUI または ticlang コマンド)
# プロジェクト → Build Project
```

**ビルド設定確認**:
- `axon_phase2_verification.c` がビルド対象に含まれているか
- `AXON_BOARD` マクロが定義されているか

### 2. 初期化（自動）

`axon_routine.c` の `main()` 関数内で自動的に初期化されます:

```c
// Phase2実機検証モード初期化（PORT 1-9想定）
axon_phase2_verification_init(1);  // PORT 1に接続想定
```

### 3. 検証モード開始

検証モードを有効にするには、以下の関数を呼び出します:

```c
// 検証開始（統計記録開始）
axon_phase2_verification_start();

// 自動応答モード有効化（CHKIRQに自動的にATIRQを返す）
axon_phase2_set_auto_response(true);

// 詳細ログ有効化（UART経由で各イベントをログ出力）
axon_phase2_set_detailed_logging(true);

// テストフェーズ設定（1-6）
axon_phase2_set_test_phase(1);  // T2.1: FRAMカウンタAPI検証
```

**推奨設定**:
```c
axon_phase2_verification_start();
axon_phase2_set_auto_response(false);  // SOMA側から明示的に制御
axon_phase2_set_detailed_logging(true);  // ログ出力有効
```

### 4. SOMA側からの検証実行

SOMA側（PORT 1-9）から以下のコマンドを送信:

#### Phase 2.1: FRAMカウンタAPI検証
```c
// SOMA側でSETAXONコマンド送信
// → AXON側でfram_increment_counter()が実行される（別の処理で）
// → axon_phase2_record_fram_increment(overflow_prevented)が自動記録
```

#### Phase 2.2: ダイヤル回転検出検証
```c
// SOMA側でCHKIRQ送信
// → AXON側でATIRQを返す（ROT_DET, BLK_ON含む）
// [ユーザー操作] ダイヤルを回転
// → axon_phase2_record_rotation_detected()が自動記録
```

#### Phase 2.3: 購入処理検証
```c
// [ユーザー操作] 現金投入シミュレート
// SOMA側でCHKIRQ送信 → AXON側がATIRQ返す（COIN_DET=1）
// [ユーザー操作] ダイヤル回転
// → axon_phase2_record_purchase(is_cashless=false)が自動記録
```

#### Phase 2.4: 非購入処理検証
```c
// [ユーザー操作] エスクロSW押下
// SOMA側でCHKIRQ送信 → AXON側がATIRQ返す（ESCRW_DET=1）
// → axon_phase2_record_non_purchase(NON_PURCHASE_REASON_REFUND)が自動記録
```

#### Phase 2.5: 面番号変更検証
```c
// [ユーザー操作] ボタン長押し（3秒以上）
// → 面番号変更モード突入
// → axon_phase2_record_face_change(is_duplicate, is_zero_mode)が自動記録
```

#### Phase 2.6: 運用動作検証
```c
// [ユーザー操作] ドア開閉
// → axon_phase2_record_door_event(is_open)が自動記録

// [ユーザー操作] 磁気センサー動作（商品なし）
// → axon_phase2_record_sold_out()が自動記録

// [ユーザー操作] 商品補充
// → axon_phase2_record_sold_out_cleared()が自動記録
```

### 5. 統計レポート出力

検証終了後、統計レポートを出力:

```c
// 統計レポート出力（UART経由）
axon_phase2_print_stats();

// または、統計構造体を直接取得
axon_phase2_stats_t* stats = axon_phase2_get_stats();
printf("FRAM Increment Count: %lu\n", stats->fram_increment_count);
printf("Rotation Detected: %lu\n", stats->rotation_detected_count);
printf("Cash Purchase: %lu\n", stats->cash_purchase_count);
```

**出力例**:
```
========================================
  Phase 2 Verification Statistics
========================================
Target PORT: 1
Test Phase: 1
Total Time: 120345 ms

--- T2.1: FRAM Counter API ---
  Increment Count:       150
  Overflow Prevented:    0

--- T2.2: Dial Rotation Detection ---
  Rotation Detected:     45
  State Transitions:     225
  Debounce Count:        12
  Timeout Count:         0

--- T2.3: Purchase Processing ---
  Cash Purchase:         30
  Cashless Purchase:     15
  State Transitions:     540

--- T2.4: Non-Purchase Processing ---
  Cash Refund:           5
  Cashless Cancel:       2
  Timeout:               1

--- T2.5: Face Number Change ---
  Face Change:           3
  Duplicate Prevented:   1
  Zero Mode:             0
  Data Inherited:        3

--- T2.6: Operation ---
  Door Open:             8
  Door Close:            8
  Sold Out Detected:     2
  Sold Out Cleared:      2
  LED Control:           100

--- Communication Statistics ---
  CHKIRQ Received:       200
  ATIRQ Sent:            200
  SETAXON Received:      50
  ACK Sent:              50
  NOP Received:          10

--- Error Statistics ---
  CRC Error:             0
  NACK Sent:             0
  Communication Timeout: 0
  Invalid State Trans:   0

--- Success Rate ---
  Total Commands:        250
  Total Errors:          0
  Success Rate:          100.00%

========================================
```

### 6. 検証モード停止

```c
// 検証停止（最終統計レポート自動出力）
axon_phase2_verification_stop();

// 統計カウンタリセット（再検証時）
axon_phase2_verification_reset_stats();
```

---

## 📊 検証合格基準

### 機能検証
- ✅ T2.1: FRAMインクリメント成功率 **100%**、オーバーフロー防止動作確認
- ✅ T2.2: ダイヤル回転検出精度 **100%**、状態遷移正常
- ✅ T2.3: 購入処理完了率 **100%**（現金/キャッシュレス両方）
- ✅ T2.4: 非購入処理正常動作（返金/中止/タイムアウト）
- ✅ T2.5: 面番号変更成功率 **100%**、重複防止動作確認
- ✅ T2.6: ドア開閉/売り切れ検知精度 **100%**

### 通信検証
- ✅ CHKIRQ/ATIRQ応答率 **100%**
- ✅ SETAXON/ACK応答率 **100%**
- ✅ CRC異常率 **0%**
- ✅ 応答時間 **100ms以内**

### 統計記録
- ✅ 全イベント記録漏れ **0件**
- ✅ タイムスタンプ精度 **±10ms以内**

---

## 🔍 デバッグ方法

### UART出力確認

詳細ログを有効にすると、各イベントがUART経由で出力されます:

```
[PHASE2_VERIFY] Verification started at 12345 ms
[PHASE2_VERIFY] Target PORT: 1
[1234 ms] CHKIRQ_RECEIVED
[1236 ms] ATIRQ_SENT
[5678 ms] ROTATION_DETECTED
[PHASE2_VERIFY] Rotation detected (total=1)
[9012 ms] FRAM_INCREMENT
[9500 ms] CASH_PURCHASE
[PHASE2_VERIFY] Purchase completed (Cash, total=1/0)
```

### 統計構造体直接アクセス

```c
#include "axon_phase2_verification.h"

// 統計取得
axon_phase2_stats_t* stats = axon_phase2_get_stats();

// デバッガでブレークポイント設定して確認
if (stats->crc_error_count > 0) {
    // CRCエラー発生
    printf("CRC Error detected: %lu\n", stats->crc_error_count);
}

if (stats->rotation_detected_count < expected_count) {
    // 回転検出失敗
    printf("Rotation detection failed: %lu/%lu\n", 
           stats->rotation_detected_count, expected_count);
}
```

### LED確認

AXON基板のRGB LEDで動作確認:
- **赤LED**: エラー発生時
- **緑LED**: 正常処理時
- **青LED**: 待機状態
- **白LED（全点灯）**: ATIRQ送信成功

---

## 📝 注意事項

1. **検証モード有効化忘れ**
   - `axon_phase2_verification_start()` を呼び出さないと統計記録されません

2. **詳細ログのオーバーヘッド**
   - `detailed_logging_enabled=true` はUART送信が多発するため、タイミングに影響する可能性があります
   - 高頻度イベント検証時は `false` 推奨

3. **統計カウンタのオーバーフロー**
   - `uint32_t` 型なので、4,294,967,295回まで記録可能
   - 長期間検証時は定期的に `axon_phase2_print_stats()` でログ出力

4. **Phase 3統合時の再検証**
   - TG通信機能実装後、本検証を再実施してPhase 2+3の統合検証とすること

---

## 🔗 関連ドキュメント

- **タスク仕様書**: `tasks/SOMA-AXON/T2.7_Phase2_実機検証.md`
- **Phase 2実装タスク**:
  - T2.1: FRAM API
  - T2.2: ダイヤル回転検出
  - T2.3: 通常購入処理
  - T2.4: 非購入処理
  - T2.5: 面番号変更
  - T2.6: 運用動作
- **通信仕様**: `tasks/SOMA-AXON/8.0_Communication_Test_Task.md`

---

**作成日**: 2025年11月25日  
**更新日**: 2025年11月25日  
**作成者**: GitHub Copilot (Claude Sonnet 4.5)
