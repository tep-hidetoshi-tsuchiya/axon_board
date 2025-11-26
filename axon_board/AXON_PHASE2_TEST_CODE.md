# AXON Phase2 テストコード実装ガイド

## 概要

AXON側Phase2検証テスト用のコード実装です。SOMA側Phase2検証テストと連携し、各種イベント発生時にIRQ信号をSOMA側に通知します。

## 主要ファイル

### 1. `axon_phase2_autotest.c/h`
テスト機能の実装ファイル

**主要機能:**
- 6つのテストケース実行（TC-01～TC-06）
- イベント発生時のIRQ信号出力（Active-LOW）
- テスト結果の統計・表示

**IRQ信号制御API:**
```c
// IRQ信号アサート（内部関数）
static void assert_irq_signal(void);

// IRQ信号クリア（CHKIRQ受信後に呼び出す）
void axon_phase2_clear_irq_signal(void);

// IRQ信号状態取得
bool axon_phase2_is_irq_asserted(void);
```

### 2. `peripheral/msp_peripheral_config.c`
GPIO初期化処理

**IRQ出力ピン設定:**
- GPIO: PA24 (UART_IRQ_OUT_PIN)
- 初期状態: HIGH（非アクティブ）
- Active-LOW（LOWで割り込み通知）

## テストシーケンス

### TC-01: FRAMカウンタAPI
- FRAM統計記録のシミュレーション
- IRQ信号: なし

### TC-02: ダイヤル回転検出
1. 回転状態遷移 5回（200ms間隔）
2. 回転検出イベント発生
3. **IRQ信号アサート**（500ms保持）

### TC-03: 購入フロー
1. コイン投入シミュレーション → **IRQ信号アサート**
2. 現金購入完了 → **IRQ信号アサート**
3. キャッシュレス購入完了 → **IRQ信号アサート**

### TC-04: 非購入フロー
1. エスクローSW押下（返金） → **IRQ信号アサート**
2. キャッシュレス中止
3. タイムアウト

### TC-05: 面番号変更
- 通常変更、重複防止、0面モード設定、データ継承
- IRQ信号: なし（統計のみ）

### TC-06: 運用動作
1. ドア開 → **IRQ信号アサート**
2. ドア閉 → **IRQ信号アサート**
3. 売り切れ検知 → **IRQ信号アサート**
4. 売り切れ解除
5. LED制御

## IRQ信号仕様

### 信号線
- **GPIO**: PA24 (GPIOA, DL_GPIO_PIN_24)
- **極性**: Active-LOW
  - HIGH（非アクティブ）: 通常状態
  - LOW（アクティブ）: イベント発生を通知

### タイミング
```
AXON Event → assert_irq_signal() → GPIO PA24 = LOW
                                        ↓
SOMA側が50msポーリングでLOWを検出
                                        ↓
SOMA → AXON: CHKIRQパケット送信
                                        ↓
AXON → SOMA: ATIRQパケット応答
                                        ↓
AXON: axon_phase2_clear_irq_signal() → GPIO PA24 = HIGH
```

### SOMA側との連携

**SOMA側実装（参考）:**
```c
// 50msポーリング、30秒タイムアウト
bool wait_for_axon_irq(uint8_t port, uint32_t timeout_ms) {
    for (uint32_t elapsed = 0; elapsed < timeout_ms; elapsed += 50) {
        if (ioexp_read_pin(port) == 0) {  // Active-LOW
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return false;  // Timeout
}
```

**通信フロー:**
1. AXON: イベント発生 → IRQ信号アサート（GPIO LOW）
2. SOMA: 50msポーリングでIRQ検出
3. SOMA → AXON: CHKIRQパケット送信（0x49）
4. AXON → SOMA: ATIRQパケット応答（0x6A + イベント情報）
5. AXON: IRQクリア（GPIO HIGH）

## テスト実行方法

### 1. ボタン操作（SOMA連携時）
**SOMA側のテストフローに合わせた実行手順:**

```
[SOMA]              [AXON]
Phase2テスト開始
  ↓
案内表示
  ↓
5秒待機  -------> BUTTON1+BUTTON2を5秒押す
  ↓                  ↓
T2.1実行           自動テスト開始(7秒)
  ↓                  ↓
T2.2開始           TC-02実行(IRQ送信) ← 1.5秒後
IRQ検出(10秒以内)   ↓
  ↓               TC-03実行(IRQ送信) ← +1.5秒
T2.3開始            ↓
IRQ検出(10秒以内)  TC-04実行(IRQ送信) ← +1.1秒
  ↓                ↓
T2.4開始           TC-06実行(IRQ送信) ← +2.1秒
IRQ検出(10秒以内)  ↓
                  完了
```

- **タイミング**: SOMA側の5秒待機中にボタンを押す
- **AXON実行時間**: 約7秒
- **IRQ送信タイミング**: TC-02(1.5秒後), TC-03(3秒後), TC-04(4.1秒後), TC-06(6.2秒後)
- **SOMA検出時間**: 各テスト開始から10秒以内

### 2. UARTコマンド（単独テスト時）
```
autotest
```

### 3. プログラムから呼び出し
```c
#include "axon_phase2_autotest.h"

void test_main(void) {
    // テスト実行
    axon_autotest_result_t result = axon_phase2_run_autotest();
    
    // 結果表示
    axon_phase2_print_autotest_result(&result);
}
```

## 実装済み機能

### ✅ IRQ信号出力
- GPIO PA24初期化完了（HIGH = 非アクティブ）
- assert_irq_signal()でLOW出力
- axon_phase2_clear_irq_signal()でHIGH復帰

### ✅ テストケース
- TC-01: FRAM統計（シミュレーション）
- TC-02: ダイヤル回転検出 + IRQ
- TC-03: 購入フロー（現金/キャッシュレス） + IRQ
- TC-04: 非購入フロー（返金/中止/タイムアウト） + IRQ
- TC-05: 面番号変更（統計のみ）
- TC-06: 運用動作（ドア/売り切れ/LED） + IRQ

### ✅ デバッグ出力
各テストケースで詳細なprintf出力
- イベント発生タイミング
- IRQ信号アサート/クリア
- 統計カウンタ値

## 未実装機能（今後の拡張）

### ⚠️ CHKIRQ自動応答
現在、CHKIRQパケット受信後の`axon_phase2_clear_irq_signal()`呼び出しは手動実装が必要です。

**実装案:**
```c
// uart_packet.cなどで
void handle_chkirq_packet(void) {
    // ATIRQ応答を送信
    send_atirq_response();
    
    // IRQクリア
    if (axon_phase2_is_irq_asserted()) {
        axon_phase2_clear_irq_signal();
    }
}
```

### ⚠️ 実イベント連携
現在はシミュレーションのみ。実際のセンサー/スイッチイベントでIRQを出力する場合:

```c
// isr.cなどで
void DIAL_SW_IRQHandler(void) {
    if (DL_GPIO_getEnabledInterruptStatus(DIAL_SW_PORT, DIAL_SW_PIN)) {
        // ダイヤル回転検出
        axon_phase2_record_rotation_detected();
        
        // テストモード時のみIRQ出力
        if (is_test_mode_enabled()) {
            assert_irq_signal();
        }
        
        DL_GPIO_clearInterruptStatus(DIAL_SW_PORT, DIAL_SW_PIN);
    }
}
```

## ビルドと動作確認

### ビルド
```bash
cd axon_board
# CCSでプロジェクトをビルド
```

### 動作確認
1. AXON基板にプログラム書き込み
2. UARTターミナル接続（115200bps）
3. コマンド実行: `autotest`
4. 各テストケースのログ確認
5. GPIO PA24をオシロスコープで観測（IRQ信号確認）

### 期待されるログ出力例
```
========================================
  AXON Phase2 Auto Test Started
========================================
Testing all Phase 2 functions...

[TC-01] FRAM Counter API Test...
  Testing FRAM increment (Simulation)...
  ✓ FRAM API simulation working (increments=1)
  Result: PASS

[TC-02] Dial Rotation Detection Test...
  Simulating rotation detection...
    Rotation state transition 1/5
    ...
  Dial rotation detected!
  [IRQ] Signal asserted (GPIO PA24 = LOW)
  ✓ Rotation detection working (detected=1, transitions=5)
  Result: PASS

...

========================================
  AXON Phase2 Auto Test Summary
========================================
Total Tests:     6
Passed:          6 (100%)
Failed:          0

🎉 ALL TESTS PASSED! 🎉
```

## トラブルシューティング

### IRQ信号が出ない
1. GPIO PA24の初期化確認
   - `msp_peripheral_config.c`の`UART_IRQ_OUT`設定
2. `assert_irq_signal()`呼び出し確認
   - 各テストケースでprintf出力を確認

### SOMA側がIRQを検出しない
1. 配線確認（PA24 → SOMA側IOExpander）
2. 信号極性確認（Active-LOW）
3. SOMA側ポーリング間隔（50ms）とIRQ保持時間（500ms）

### テストがFAILする
- 統計カウンタがリセットされていない可能性
- `axon_phase2_reset_stats()`を先に呼び出す

## 参考ドキュメント

- `tasks/SOMA-AXON/PHASE2_TEST_ROLES.md` - SOMA/AXON役割分担
- `tasks/SOMA-AXON/PHASE2_TEST_AXON_GUIDE.md` - AXON実装ガイド
- `tasks/SOMA-AXON/PHASE2_IRQ_IMPLEMENTATION.md` - IRQ実装完了報告
- `axon_phase2_verification.c/h` - Phase2統計記録機能

## 更新履歴

- 2025-11-25: 初版作成（テストコード実装完了）
  - IRQ信号出力機能追加
  - 6つのテストケース実装
  - デバッグ出力強化
