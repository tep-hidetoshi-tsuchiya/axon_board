# Phase2テストシーケンス（修正版）

## 全体フロー

```
┌─────────────────────────────────────────────────────────────┐
│                    Phase2 検証テスト（修正版）                 │
└─────────────────────────────────────────────────────────────┘

【1】SOMA準備フェーズ (1秒)
     └─ 初期化完了待ち

【2】ローカルテスト (T2.1)
     └─ SOMA: FRAMカウンタAPI検証

【3】AXON自動テスト起動 + リアルタイムIRQ検出
     ┌─ SOMA: PHASE2TEST(0x4F)コマンド送信 → AXON
     │
     ├─ AXON: ACK応答 → SOMA
     │   (Header=0x10, ID=0x00)
     │
     ├─ SOMA: IRQポーリング開始 (50ms間隔、バックグラウンド)
     │
     └─ AXON: 自動テスト実行 (約15秒)
        ├─ TC-01: FRAM書き込み (IRQなし)
        ├─ TC-02: ダイヤル回転 → IRQ (2s LOW)
        │   └─ SOMA検出 → CHKIRQ → ATIRQ(0x6A)
        │
        ├─ TC-03: 通常購入×3 → IRQ×3 (各2s LOW)
        │   └─ SOMA検出×3 → CHKIRQ×3 → ATIRQ×3
        │
        ├─ TC-04: 非購入 → IRQ (2s LOW)
        │   └─ SOMA検出 → CHKIRQ → ATIRQ
        │
        ├─ TC-05: 面変更 (IRQなし)
        │
        └─ TC-06: 運用動作×3 → IRQ×3 (各2s LOW)
            └─ SOMA検出×3 → CHKIRQ×3 → ATIRQ×3

【4】結果サマリ表示
     └─ IRQ検出数: 7/7
     └─ CHKIRQ/ATIRQ成功数確認
```

## UART通信シーケンス（修正版）

```
SOMA                          AXON
 │                             │
 │──PHASE2TEST(0x4F)──────────▶│
 │   Header: 0x14              │
 │   ID: 0x4F                  │
 │   test_phase: 1             │
 │                             │
 │◀───ACK──────────────────────│
 │   Header: 0x10              │
 │   ID: 0x00                  │
 │                             │
 │  ★IRQポーリング開始★       │──TC-01実行 (IRQなし)
 │  (50ms間隔)                 │
 │                             │──TC-02実行
 │                             │   └─ assert_irq_signal()
 │                             │      PA24 = LOW (2秒)
 │  IRQ検出！                  │
 │──CHKIRQ(0x49)──────────────▶│
 │   Header: 0x14              │
 │   ID: 0x49                  │
 │                             │
 │◀───ATIRQ────────────────────│
 │   Header: 0x10              │
 │   ID: 0x6A                  │
 │   STATUS/FACE_N/CASH_VLU    │
 │                             │
 │                             │   PA24 = HIGH (自動復帰)
 │                             │
 │                             │──TC-03実行
 │                             │   ├─ 1回目 IRQ (2s)
 │  IRQ検出！                  │
 │──CHKIRQ──────────────────▶  │
 │◀───ATIRQ─────────────────│  │
 │                             │   ├─ 2回目 IRQ (2s)
 │  IRQ検出！                  │
 │──CHKIRQ──────────────────▶  │
 │◀───ATIRQ─────────────────│  │
 │                             │   └─ 3回目 IRQ (2s)
 │  IRQ検出！                  │
 │──CHKIRQ──────────────────▶  │
 │◀───ATIRQ─────────────────│  │
 │                             │
 │ (以下、TC-04~TC-06も同様)   │
 │                             │
 │  合計7回のIRQ検出完了       │──全TC完了
 │                             │
```

## タイミング詳細（修正版）

```
時刻  SOMA                     AXON
────────────────────────────────────────────
0s    PHASE2TEST送信           
1s    ACK受信                  ACK応答
      IRQポーリング開始        TC-01開始 (IRQなし)
2s                             TC-02開始
3s    IRQ#1検出(TC-02)         PA24=LOW (2s)
      CHKIRQ送信               
      ATIRQ受信                ATIRQ応答
5s                             PA24=HIGH
                               (1s待機)
6s                             TC-03開始
7s    IRQ#2検出(TC-03-1)       PA24=LOW (2s)
      CHKIRQ送信               
      ATIRQ受信                ATIRQ応答
9s                             PA24=HIGH
                               (1s待機)
10s   IRQ#3検出(TC-03-2)       PA24=LOW (2s)
      CHKIRQ送信               
      ATIRQ受信                ATIRQ応答
12s                            PA24=HIGH
                               (1s待機)
13s   IRQ#4検出(TC-03-3)       PA24=LOW (2s)
      CHKIRQ送信               
      ATIRQ受信                ATIRQ応答
15s                            PA24=HIGH
                               (1s待機)
16s                            TC-04開始
17s   IRQ#5検出(TC-04)         PA24=LOW (2s)
      CHKIRQ送信               
      ATIRQ受信                ATIRQ応答
19s                            PA24=HIGH
                               TC-05開始 (IRQなし)
20s                            TC-06開始
21s   IRQ#6検出(TC-06-1)       PA24=LOW (2s)
23s   IRQ#7検出(TC-06-2)       PA24=LOW (2s)
25s                            全TC完了
      結果サマリ表示
```

## 重要な修正点

### 1. ACK/ATIRQのヘッダー・ID
- ❌ 誤: `ACK(0x80)`, `ATIRQ(0x15)`
- ✅ 正: `ACK(Header=0x10, ID=0x00)`, `ATIRQ(Header=0x10, ID=0x6A)`

### 2. IRQ検出タイミング
- ❌ 誤: 10秒待機後にまとめて検出
- ✅ 正: リアルタイムポーリング（50ms間隔）で即座に検出

### 3. IRQ信号の自動クリア
- 各IRQ信号は2秒間LOW保持後、自動的にHIGHに復帰
- SOMA側は50msポーリングで確実に検出可能（2000ms ÷ 50ms = 40回のチャンス）

### 4. テストケース数
- TC-01: IRQなし
- TC-02: IRQ×1
- TC-03: IRQ×3
- TC-04: IRQ×1
- TC-05: IRQなし
- TC-06: IRQ×2 (仕様確認必要 - 現在は×3の可能性あり)
- **合計: 7回のIRQ検出**

## SOMA側実装の推奨アプローチ

```c
bool phase2_auto_test_sequence(uint8_t axon_port) {
    printf("=== Phase2 Auto Test: AXON Port %d ===\n", axon_port);
    
    // 1. PHASE2TESTコマンド送信
    if (!send_phase2test_command(axon_port)) {
        printf("ERROR: Failed to send PHASE2TEST command\n");
        return false;
    }
    
    // 2. ACK待機 (2秒タイムアウト)
    if (!wait_for_ack(2000)) {
        printf("ERROR: No ACK received\n");
        return false;
    }
    printf("ACK received. AXON auto test started.\n");
    
    // 3. IRQリアルタイム検出ループ
    uint8_t irq_count = 0;
    uint32_t start_time = get_time_ms();
    uint32_t timeout = 30000;  // 30秒タイムアウト
    
    printf("Starting IRQ detection (50ms polling)...\n");
    
    while (irq_count < 7 && (get_time_ms() - start_time) < timeout) {
        // IOExpander経由でPA24状態確認
        if (check_axon_irq(axon_port)) {
            printf("[IRQ #%d] Detected! Sending CHKIRQ...\n", irq_count + 1);
            
            // CHKIRQ送信
            if (send_chkirq(axon_port)) {
                // ATIRQ受信
                atirq_data_t atirq;
                if (receive_atirq(&atirq, 2000)) {
                    printf("  ATIRQ received: STATUS=0x%02X, FACE=%d, CASH=%d\n",
                           atirq.status, atirq.face_n, atirq.cash_vlu);
                    irq_count++;
                } else {
                    printf("  ERROR: ATIRQ timeout\n");
                }
            }
            
            // 次のIRQ検出まで待機 (IRQクリア時間考慮)
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        // ポーリング間隔
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // 4. 結果判定
    printf("\n=== Phase2 Test Result ===\n");
    printf("IRQ Detected: %d/7\n", irq_count);
    printf("Time Elapsed: %lu ms\n", get_time_ms() - start_time);
    
    if (irq_count == 7) {
        printf("Result: PASS ✓\n");
        return true;
    } else {
        printf("Result: FAIL ✗\n");
        return false;
    }
}
```

## 結論

- **リアルタイムIRQ検出方式**を採用することで、確実に全IRQ信号を検出可能
- AXON側の実装変更は不要（現状の2秒保持で十分）
- SOMA側でバックグラウンドポーリングを実装する必要あり
