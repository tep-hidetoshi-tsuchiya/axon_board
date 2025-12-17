/**
 * @file axon_phase2_autotest.c
 * @brief Phase 2検証テスト機能実装（テストコード）
 * 
 * SOMA側Phase2検証テストと連携するためのテスト用コード
 * イベント発生時にIRQ信号(GPIO PA24)をアサートし、SOMA側に通知
 * 
 * 【テストモード】
 * - ダイヤル回転、コイン投入、扉開閉などをシミュレート
 * - 各イベントでIRQ信号を出力(Active-LOW)
 * - SOMA側からCHKIRQパケット受信でIRQクリア
 * 
 * @date 2025-11-25
 * @author GitHub Copilot (Claude Sonnet 4.5)
 * 
 * @note 【コメントアウト済み】このコードは現在使用されていません
 */

#if 0  // Phase2テストコード - 現在コメントアウト中

#include "axon_phase2_autotest.h"
#include "axon_phase2_verification.h"
#include "peripheral/msp_peripheral_config.h"
#include "axon_status.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// テスト用のダミーデータ
static uint8_t test_board_num = 1;

// IRQ信号状態
static volatile bool g_irq_asserted = false;

// 自動テストモード実行中フラグ
static volatile bool g_autotest_running = false;

// 前方宣言
static void delay_ms(uint32_t ms);

/**
 * @brief 自動テストモード中かどうかを取得
 */
bool axon_phase2_is_autotest_running(void) {
    return g_autotest_running;
}

/**
 * @brief IRQ信号アサート（自動テストモード専用）
 * @details Active-LOW信号で2秒間保持し、自動的にHIGHに復帰
 * 
 * 【自動テストモード専用の完全なIRQ制御】
 * この関数は自動テストモード専用で、IRQ信号の完全なライフサイクルを管理:
 * 1. GPIO PA24をLOWに設定（SOMAへの割り込み通知）
 * 2. 2000ms保持（SOMAの50msポーリング周期×40回分を保証）
 * 3. GPIO PA24をHIGHに復帰（自動クリア）
 * 
 * 【通常モードとの違い】
 * - 通常モード（axon_routine.c）: assert_irq_signal()はLOWのみ設定
 *   → HIGHへの復帰はs2a_packet.cのclear_irq_signal()が担当
 * - 自動テストモード（本関数）: LOW→2秒保持→HIGHまで完結
 *   → s2a_packet.cは介入しない（g_autotest_running=trueで条件スキップ）
 * 
 * 【タイミング要件】
 * 2000ms保持により、SOMAが確実にIRQ信号を検出できる:
 * - SOMAポーリング周期: 50ms
 * - 保持時間: 2000ms = 40ポーリング周期分
 * - 検出確率: 実質100%
 * 
 * @note この関数は自動テストモード中のみ使用。
 *       g_autotest_running=trueの間、s2a_packet.cのclear_irq_signal()は
 *       ATIRQ送信時にスキップされ、本関数による2秒保持が保証される。
 */
static void assert_irq_signal(void) {
    DL_GPIO_clearPins(UART_PORT, UART_IRQ_OUT_PIN);
    g_irq_asserted = true;
    printf("  [IRQ] Signal asserted (GPIO PA24 = LOW)\n");
    printf("  [IRQ] Holding signal for 2 seconds...\n");
    delay_ms(2000);  // 2秒間LOW保持
    DL_GPIO_setPins(UART_PORT, UART_IRQ_OUT_PIN);
    g_irq_asserted = false;
    printf("  [IRQ] Signal cleared (GPIO PA24 = HIGH)\n");
}

/**
 * @brief IRQ信号クリア
 * 
 * GPIO PA24をHIGHに戻す（CHKIRQ受信後に呼び出す想定）
 */
void axon_phase2_clear_irq_signal(void) {
    DL_GPIO_setPins(UART_PORT, UART_IRQ_OUT_PIN);
    g_irq_asserted = false;
    printf("  [IRQ] Signal cleared (GPIO PA24 = HIGH)\n");
}

/**
 * @brief IRQ信号状態取得
 */
bool axon_phase2_is_irq_asserted(void) {
    return g_irq_asserted;
}

/**
 * @brief ディレイ関数(ms)
 */
static void delay_ms(uint32_t ms) {
    delay_cycles(ms * (CPUCLK_FREQ / 1000));
}

/**
 * @brief 自動テスト実行
 */
axon_autotest_result_t axon_phase2_run_autotest(void) {
    axon_autotest_result_t result;
    memset(&result, 0, sizeof(result));
    
    // 自動テストモード開始
    g_autotest_running = true;
    
    printf("\n");
    printf("========================================\n");
    printf("  AXON Phase2 Auto Test Started\n");
    printf("========================================\n");
    printf("Testing all Phase 2 functions...\n");
    printf("Expected duration: ~15 seconds\n");
    printf("IRQ count: 4 (TC-02×1, TC-03×1, TC-04×1, TC-06×1)\n");
    printf("Each IRQ: 2s LOW hold + auto HIGH restore\n");
    printf("\n");
    
    // TC-01: FRAMカウンタAPI
    printf("[TC-01] FRAM Counter API Test...\n");
    result.tc01_fram_api = axon_autotest_tc01_fram_api();
    printf("  Result: %s\n\n", result.tc01_fram_api ? "PASS" : "FAIL");
    delay_ms(1000);  // テストケース間隔
    
    // TC-02: ダイヤル回転検出
    printf("[TC-02] Dial Rotation Detection Test...\n");
    result.tc02_rotation_detect = axon_autotest_tc02_rotation_detect();
    printf("  Result: %s\n\n", result.tc02_rotation_detect ? "PASS" : "FAIL");
    delay_ms(1000);  // テストケース間隔
    
    // TC-03: 購入フロー
    printf("[TC-03] Purchase Flow Test...\n");
    result.tc03_purchase_flow = axon_autotest_tc03_purchase_flow();
    printf("  Result: %s\n\n", result.tc03_purchase_flow ? "PASS" : "FAIL");
    delay_ms(1000);  // テストケース間隔
    
    // TC-04: 非購入フロー
    printf("[TC-04] Non-Purchase Flow Test...\n");
    result.tc04_non_purchase = axon_autotest_tc04_non_purchase();
    printf("  Result: %s\n\n", result.tc04_non_purchase ? "PASS" : "FAIL");
    delay_ms(1000);  // テストケース間隔
    
    // TC-05: 面番号変更
    printf("[TC-05] Face Number Change Test...\n");
    result.tc05_face_change = axon_autotest_tc05_face_change();
    printf("  Result: %s\n\n", result.tc05_face_change ? "PASS" : "FAIL");
    delay_ms(1000);  // テストケース間隔
    
    // TC-06: 運用動作
    printf("[TC-06] Operation Test...\n");
    result.tc06_operation = axon_autotest_tc06_operation();
    printf("  Result: %s\n\n", result.tc06_operation ? "PASS" : "FAIL");
    
    // 結果集計
    result.total_tests = 6;
    result.passed_tests = 0;
    if (result.tc01_fram_api) result.passed_tests++;
    if (result.tc02_rotation_detect) result.passed_tests++;
    if (result.tc03_purchase_flow) result.passed_tests++;
    if (result.tc04_non_purchase) result.passed_tests++;
    if (result.tc05_face_change) result.passed_tests++;
    if (result.tc06_operation) result.passed_tests++;
    
    // 自動テストモード終了
    g_autotest_running = false;
    
    return result;
}

/**
 * @brief 自動テスト結果表示
 */
void axon_phase2_print_autotest_result(const axon_autotest_result_t* result) {
    printf("\n");
    printf("========================================\n");
    printf("  AXON Phase2 Auto Test Summary\n");
    printf("========================================\n");
    printf("Total Tests:     %u\n", result->total_tests);
    printf("Passed:          %u (%u%%)\n", result->passed_tests, 
           (result->passed_tests * 100) / result->total_tests);
    printf("Failed:          %u\n", result->total_tests - result->passed_tests);
    printf("\n");
    printf("Test Results:\n");
    printf("  TC-01 FRAM API:          %s\n", result->tc01_fram_api ? "PASS" : "FAIL");
    printf("  TC-02 Rotation Detect:   %s\n", result->tc02_rotation_detect ? "PASS" : "FAIL");
    printf("  TC-03 Purchase Flow:     %s\n", result->tc03_purchase_flow ? "PASS" : "FAIL");
    printf("  TC-04 Non-Purchase:      %s\n", result->tc04_non_purchase ? "PASS" : "FAIL");
    printf("  TC-05 Face Change:       %s\n", result->tc05_face_change ? "PASS" : "FAIL");
    printf("  TC-06 Operation:         %s\n", result->tc06_operation ? "PASS" : "FAIL");
    printf("========================================\n");
    
    if (result->passed_tests == result->total_tests) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n\n");
    } else {
        printf("\n❌ SOME TESTS FAILED\n\n");
    }
}

/**
 * @brief TC-01: FRAMカウンタAPIテスト（シミュレーション）
 * 
 * 注: AXON側にはFRAM実装がないため、統計記録のみでテスト
 */
bool axon_autotest_tc01_fram_api(void) {
    printf("  Testing FRAM increment (Simulation)...\n");
    
    // Phase2検証記録（シミュレーション）
    axon_phase2_record_fram_increment(false);
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->fram_increment_count == 0) {
        printf("  ERROR: FRAM increment not recorded\n");
        return false;
    }
    
    printf("  ✓ FRAM API simulation working (increments=%u)\n", stats->fram_increment_count);
    return true;
}

/**
 * @brief TC-02: ダイヤル回転検出テスト（シミュレーション）
 */
bool axon_autotest_tc02_rotation_detect(void) {
    printf("  Simulating rotation detection...\n");
    
    // 状態遷移シミュレート（5回）
    for (int i = 0; i < 5; i++) {
        axon_phase2_record_rotation_state_transition();
        printf("    Rotation state transition %d/5\n", i + 1);
        delay_ms(200);  // 状態遷移間隔
    }
    
    // 回転検出記録 & IRQ信号アサート
    printf("  Dial rotation detected!\n");
    axon_phase2_record_rotation_detected();
    assert_irq_signal();  // SOMA側に通知（2秒保持→自動クリア）
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->rotation_detected_count == 0) {
        printf("  ERROR: Rotation not recorded\n");
        return false;
    }
    
    if (stats->rotation_state_transitions < 5) {
        printf("  ERROR: State transitions not recorded\n");
        return false;
    }
    
    printf("  ✓ Rotation detection working (detected=%u, transitions=%u)\n",
           stats->rotation_detected_count, stats->rotation_state_transitions);
    return true;
}

/**
 * @brief TC-03: 購入フローテスト（シミュレーション）
 */
bool axon_autotest_tc03_purchase_flow(void) {
    printf("  Simulating purchase flow...\n");
    
    // 現金購入シミュレート（記録のみ、IRQなし）
    printf("    Coin inserted (simulated)\n");
    axon_phase2_record_purchase_state_transition();  // 状態遷移
    printf("    Cash purchase completed\n");
    axon_phase2_record_purchase(false);  // 現金購入完了
    
    // キャッシュレス購入シミュレート（記録のみ、IRQなし）
    printf("    Cashless payment (simulated)\n");
    axon_phase2_record_purchase_state_transition();
    printf("    Cashless purchase completed\n");
    axon_phase2_record_purchase(true);  // キャッシュレス購入完了
    
    // TC-03完了時に1回だけIRQ送信
    printf("  Purchase flow test completed - Sending IRQ\n");
    assert_irq_signal();  // 購入フローテスト完了通知（2秒保持→自動クリア）
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->cash_purchase_count == 0) {
        printf("  ERROR: Cash purchase not recorded\n");
        return false;
    }
    
    if (stats->cashless_purchase_count == 0) {
        printf("  ERROR: Cashless purchase not recorded\n");
        return false;
    }
    
    printf("  ✓ Purchase flow working (cash=%u, cashless=%u)\n",
           stats->cash_purchase_count, stats->cashless_purchase_count);
    return true;
}

/**
 * @brief TC-04: 非購入フローテスト（シミュレーション）
 */
bool axon_autotest_tc04_non_purchase(void) {
    printf("  Simulating non-purchase flow...\n");
    
    // 現金返金シミュレート（記録のみ、IRQなし）
    printf("    Escrow SW pressed (refund)\n");
    axon_phase2_record_non_purchase(NON_PURCHASE_REASON_REFUND);
    printf("    Cash refunded\n");
    
    // キャッシュレス中止シミュレート（記録のみ、IRQなし）
    printf("    Cashless transaction cancelled\n");
    axon_phase2_record_non_purchase(NON_PURCHASE_REASON_CANCEL);
    delay_ms(300);
    
    // タイムアウトシミュレート（記録のみ、IRQなし）
    printf("    Transaction timeout\n");
    axon_phase2_record_non_purchase(NON_PURCHASE_REASON_TIMEOUT);
    delay_ms(300);
    
    // TC-04完了時に1回だけIRQ送信
    printf("  Non-purchase flow test completed - Sending IRQ\n");
    assert_irq_signal();  // 非購入フローテスト完了通知（2秒保持→自動クリア）
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->cash_refund_count == 0) {
        printf("  ERROR: Cash refund not recorded\n");
        return false;
    }
    
    if (stats->cashless_cancel_count == 0) {
        printf("  ERROR: Cashless cancel not recorded\n");
        return false;
    }
    
    if (stats->timeout_count == 0) {
        printf("  ERROR: Timeout not recorded\n");
        return false;
    }
    
    printf("  ✓ Non-purchase flow working (refund=%u, cancel=%u, timeout=%u)\n",
           stats->cash_refund_count, stats->cashless_cancel_count, stats->timeout_count);
    return true;
}

/**
 * @brief TC-05: 面番号変更テスト（シミュレーション）
 */
bool axon_autotest_tc05_face_change(void) {
    printf("  Simulating face number change...\n");
    
    // 通常の面番号変更
    printf("    Face number changed (normal)\n");
    axon_phase2_record_face_change(false, false);
    delay_ms(300);
    
    // 重複防止ケース
    printf("    Face change with duplicate prevention\n");
    axon_phase2_record_face_change(true, false);
    delay_ms(300);
    
    // 0面モード設定
    printf("    Face zero mode set\n");
    axon_phase2_record_face_change(false, true);
    delay_ms(300);
    
    // データ継承
    printf("    Face data inherited\n");
    axon_phase2_record_face_data_inherited();
    delay_ms(300);
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->face_change_count < 3) {
        printf("  ERROR: Face changes not recorded\n");
        return false;
    }
    
    if (stats->face_duplicate_prevented == 0) {
        printf("  ERROR: Duplicate prevention not recorded\n");
        return false;
    }
    
    if (stats->face_zero_mode_count == 0) {
        printf("  ERROR: Zero mode not recorded\n");
        return false;
    }
    
    printf("  ✓ Face change working (changes=%u, duplicate=%u, zero_mode=%u)\n",
           stats->face_change_count, stats->face_duplicate_prevented, stats->face_zero_mode_count);
    return true;
}

/**
 * @brief TC-06: 運用動作テスト（シミュレーション）
 */
bool axon_autotest_tc06_operation(void) {
    printf("  Simulating operation events...\n");
    
    // ドア開閉シミュレート（記録のみ、IRQなし）
    printf("    Door opened\n");
    axon_phase2_record_door_event(true);   // ドア開
    
    printf("    Door closed\n");
    axon_phase2_record_door_event(false);  // ドア閉
    
    // 売り切れ検知シミュレート（記録のみ、IRQなし）
    printf("    Sold out detected\n");
    axon_phase2_record_sold_out();
    
    printf("    Sold out cleared\n");
    axon_phase2_record_sold_out_cleared();
    delay_ms(300);
    
    // LED制御シミュレート（記録のみ、IRQなし）
    printf("    LED control executed\n");
    axon_phase2_record_led_control();
    delay_ms(300);
    
    // TC-06完了時に1回だけIRQ送信
    printf("  Operation test completed - Sending IRQ\n");
    assert_irq_signal();  // 運用動作テスト完了通知（2秒保持→自動クリア）
    
    // 統計確認
    axon_phase2_stats_t* stats = axon_phase2_get_stats();
    if (stats->door_open_count == 0) {
        printf("  ERROR: Door open not recorded\n");
        return false;
    }
    
    if (stats->door_close_count == 0) {
        printf("  ERROR: Door close not recorded\n");
        return false;
    }
    
    if (stats->sold_out_detected_count == 0) {
        printf("  ERROR: Sold out not recorded\n");
        return false;
    }
    
    if (stats->led_control_count == 0) {
        printf("  ERROR: LED control not recorded\n");
        return false;
    }
    
    printf("  ✓ Operation working (door_open=%u, door_close=%u, sold_out=%u, led=%u)\n",
           stats->door_open_count, stats->door_close_count, 
           stats->sold_out_detected_count, stats->led_control_count);
    return true;
}

#endif  // Phase2テストコード終了

// ========== スタブ関数（Phase2テストコードがコメントアウト中でも他ファイルからリンク可能にする） ==========

#if 1  // スタブ関数は常に有効

#include "axon_phase2_autotest.h"
#include <stdbool.h>
#include <string.h>

/**
 * @brief 自動テストモード中かどうかを取得（スタブ）
 */
bool axon_phase2_is_autotest_running(void) {
    return false;  // 常にfalseを返す（テスト無効）
}

/**
 * @brief 自動テスト実行（スタブ）
 */
axon_autotest_result_t axon_phase2_run_autotest(void) {
    axon_autotest_result_t result;
    memset(&result, 0, sizeof(result));
    return result;
}

/**
 * @brief 自動テスト結果表示（スタブ）
 */
void axon_phase2_print_autotest_result(const axon_autotest_result_t* result) {
    // 何もしない
    (void)result;
}

#endif  // スタブ関数終了
