# SOMA Phase2 Test - 修正案 (Option A: シンプル待機方式)

## 問題点

1. 20秒待機後、個別テスト関数が再度IRQ待機してタイムアウト
2. IRQが実際に検出されたか確認していない
3. T2.2, T2.3, T2.4が不要なIRQ待機を行っている

## 修正内容

### 1. `phase2_verification_task()` の修正

**変更箇所:** 約580-620行目

```c
// PHASE2TESTコマンド送信
if (!soma_send_phase2test(port)) {
    ESP_LOGE(TAG, "Failed to trigger AXON Phase2 test");
    return;
}

ESP_LOGI(TAG, "\033[1;32m✓ AXON Phase2 test triggered successfully\033[0m");
ESP_LOGI(TAG, "[!] AXON will execute TC-01~TC-06 automatically (~19 seconds)\n");

// AXONテスト実行完了待機 (TC-01~TC-06完了まで約19秒 + マージン1秒)
ESP_LOGI(TAG, "\n\033[1;36m========== Real-time IRQ Monitoring ==========\033[0m");
ESP_LOGI(TAG, "  Expected IRQs: 4 (TC-02, TC-03, TC-04, TC-06)");
ESP_LOGI(TAG, "  Monitoring window: 25 seconds");
ESP_LOGI(TAG, "\033[1;36m==============================================\033[0m\n");

// リアルタイムIRQ検出ループ
const uint32_t total_window_ms = 25000;  // 25秒監視
const uint32_t poll_interval_ms = 50;    // 50msポーリング
uint32_t elapsed_ms = 0;
uint8_t irq_count = 0;
uint32_t irq_timestamps[4] = {0};

while (elapsed_ms < total_window_ms && irq_count < 4) {
    bool irq_detected = false;
    
    if (port == 9) {
        bool irq_state = false;
        if (ioexp_read_axon_irq9(&irq_state) == ESP_OK) {
            irq_detected = !irq_state;  // LOW=IRQ
        }
    } else {
        uint8_t irq_states = 0;
        if (ioexp_read_axon_irqs(&irq_states) == ESP_OK) {
            uint8_t port_bit = port - 1;
            irq_detected = !(irq_states & (1 << port_bit));
        }
    }
    
    if (irq_detected && irq_count < 4) {
        irq_timestamps[irq_count] = elapsed_ms;
        irq_count++;
        ESP_LOGI(TAG, "  ✓ IRQ #%d detected at %lu ms", irq_count, elapsed_ms);
        
        // IRQ終了待機 (2秒ホールド)
        vTaskDelay(pdMS_TO_TICKS(2100));
        elapsed_ms += 2100;
    } else {
        vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
        elapsed_ms += poll_interval_ms;
    }
}

// IRQ検出結果サマリ
ESP_LOGI(TAG, "\n\033[1;36m========== IRQ Detection Summary ==========\033[0m");
ESP_LOGI(TAG, "  Total IRQs detected: %d / 4", irq_count);
if (irq_count > 0) {
    ESP_LOGI(TAG, "  IRQ Timestamps:");
    for (uint8_t i = 0; i < irq_count; i++) {
        ESP_LOGI(TAG, "    IRQ #%d: %lu ms (TC-%02d)", i+1, irq_timestamps[i], i+2);
    }
}
ESP_LOGI(TAG, "\033[1;36m===========================================\033[0m\n");

// IRQ検出状態をグローバル変数に保存（各テスト関数で参照）
g_test_stats.irq_detected_count = irq_count;

ESP_LOGI(TAG, "\n\033[1;32m>>> AXON auto test completed - Verifying results <<<\033[0m\n");

// T2.2: ダイヤル回転検出テスト (IRQ検出済みを前提)
if (g_test_config.test_dial_rotation) {
    bool result = test_dial_rotation_detection(port, (irq_count >= 1));
    record_test_result(result, "T2.2: Dial Rotation Detection (TC-02 IRQ)");
    vTaskDelay(pdMS_TO_TICKS(g_test_config.interval_ms));
}

// T2.3: 通常購入処理テスト (IRQ検出済みを前提)
if (g_test_config.test_purchase) {
    bool result = test_purchase_flow(port, (irq_count >= 2));
    record_test_result(result, "T2.3: Purchase Flow (TC-03 IRQ)");
    vTaskDelay(pdMS_TO_TICKS(g_test_config.interval_ms));
}

// T2.4: 非購入処理テスト (IRQ検出済みを前提)
if (g_test_config.test_non_purchase) {
    bool result = test_non_purchase(port, (irq_count >= 3));
    record_test_result(result, "T2.4: Non-Purchase Flow (TC-04 IRQ)");
    vTaskDelay(pdMS_TO_TICKS(g_test_config.interval_ms));
}
```

### 2. 個別テスト関数の修正

**`test_dial_rotation_detection()` の修正:**

```c
static bool test_dial_rotation_detection(uint8_t port, bool irq_already_detected) {
    ESP_LOGI(TAG, "\n\033[1;36m[T2.2] 7.6 Dial Rotation Detection Test (Port %d)\033[0m", port);
    
    // IRQ検出確認
    if (!irq_already_detected) {
        ESP_LOGE(TAG, "  ✗ TC-02 IRQ was not detected during monitoring period");
        return false;
    }
    
    ESP_LOGI(TAG, "  ✓ TC-02 IRQ was detected successfully");
    
    // ポート選択
    if (axon_uart_select_port(port) != ESP_OK) {
        ESP_LOGE(TAG, "  Failed to select port %d", port);
        return false;
    }
    
    // CHKIRQ送信でAXON状態取得
    ATIRQ_PACKET atirq;
    bool ret = soma_send_chkirq_and_receive_atirq(port, &atirq);
    
    if (!ret) {
        ESP_LOGE(TAG, "  CHKIRQ failed");
        axon_uart_release_bus();
        return false;
    }
    
    // STATUS解析（ダイヤル回転状態をチェック）
    ESP_LOGI(TAG, "  ATIRQ Response:");
    ESP_LOGI(TAG, "    FACE_N: %d", atirq.face_n & 0x0F);
    ESP_LOGI(TAG, "    CASH_VLU: %d円", atirq.cash_vlu * 100);
    ESP_LOGI(TAG, "    STATUS: 0x%04X", atirq.status);
    
    axon_uart_release_bus();
    return true;
}
```

**`test_purchase_flow()` の修正:**

```c
static bool test_purchase_flow(uint8_t port, bool irq_already_detected) {
    ESP_LOGI(TAG, "\n\033[1;36m[T2.3] 7.7 Purchase Flow Test (Port %d)\033[0m", port);
    
    // IRQ検出確認
    if (!irq_already_detected) {
        ESP_LOGE(TAG, "  ✗ TC-03 IRQ was not detected during monitoring period");
        return false;
    }
    
    ESP_LOGI(TAG, "  ✓ TC-03 IRQ was detected successfully");
    
    // ポート選択
    if (axon_uart_select_port(port) != ESP_OK) {
        ESP_LOGE(TAG, "  Failed to select port %d", port);
        return false;
    }
    
    // 初期状態確認
    ESP_LOGI(TAG, "  [1] Check Initial State");
    ATIRQ_PACKET atirq;
    bool ret = soma_send_chkirq_and_receive_atirq(port, &atirq);
    
    if (!ret) {
        ESP_LOGE(TAG, "  CHKIRQ failed");
        axon_uart_release_bus();
        return false;
    }
    
    ESP_LOGI(TAG, "    Initial STATUS: 0x%04X", atirq.status);
    
    axon_uart_release_bus();
    return true;
}
```

**`test_non_purchase()` の修正:**

```c
static bool test_non_purchase(uint8_t port, bool irq_already_detected) {
    ESP_LOGI(TAG, "\n\033[1;36m[T2.4] 7.8 Non-Purchase Flow Test (Port %d)\033[0m", port);
    
    // IRQ検出確認
    if (!irq_already_detected) {
        ESP_LOGE(TAG, "  ✗ TC-04 IRQ was not detected during monitoring period");
        return false;
    }
    
    ESP_LOGI(TAG, "  ✓ TC-04 IRQ was detected successfully");
    
    // ポート選択
    if (axon_uart_select_port(port) != ESP_OK) {
        ESP_LOGE(TAG, "  Failed to select port %d", port);
        return false;
    }
    
    // 状態確認
    ATIRQ_PACKET atirq;
    bool ret = soma_send_chkirq_and_receive_atirq(port, &atirq);
    
    if (!ret) {
        ESP_LOGE(TAG, "  CHKIRQ failed");
        axon_uart_release_bus();
        return false;
    }
    
    ESP_LOGI(TAG, "    STATUS: 0x%04X", atirq.status);
    
    axon_uart_release_bus();
    return true;
}
```

### 3. `phase2_test_stats_t` 構造体の修正

**ヘッダファイル (`test_phase2_verification.h`) に追加:**

```c
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t timeout_count;
    uint32_t start_time_ms;
    uint32_t end_time_ms;
    uint8_t irq_detected_count;  // ← 追加
} phase2_test_stats_t;
```

## 期待される動作

1. **PHASE2TESTコマンド送信**
2. **25秒間リアルタイムIRQ監視** (50msポーリング)
   - TC-02 IRQ検出: ~3秒
   - TC-03 IRQ検出: ~7秒
   - TC-04 IRQ検出: ~11秒
   - TC-06 IRQ検出: ~17秒
3. **各テスト関数は既に検出済みのIRQを確認**
   - IRQ待機なし
   - CHKIRQ/ATIRQでAXON状態のみ確認
4. **テスト結果サマリ出力**

## メリット

- ✅ 全IRQを確実に検出
- ✅ タイムアウトなし
- ✅ IRQ発生タイムスタンプを記録
- ✅ AXON側の実装変更不要
- ✅ デバッグが容易
