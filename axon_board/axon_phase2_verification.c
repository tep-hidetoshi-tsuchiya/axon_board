/**
 * @file axon_phase2_verification.c
 * @brief Phase 2実機検証用実装ファイル
 * 
 * Phase 2で実装した機能の動作状況を記録し、統計レポートを生成
 * 
 * @date 2025-11-25
 * @author GitHub Copilot (Claude Sonnet 4.5)
 */

#include "axon_phase2_verification.h"
#include <string.h>
#include <stdio.h>
#include "ti_msp_dl_config.h"

// ========================================
// 内部変数
// ========================================

/// グローバル統計変数
static axon_phase2_stats_t g_phase2_stats;

/// グローバル制御変数
static axon_phase2_config_t g_phase2_config;

// ========================================
// 内部関数プロトタイプ
// ========================================

static uint32_t get_system_time_ms(void);
static void log_event(const char* event_name);

// ========================================
// 初期化・制御関数
// ========================================

void axon_phase2_verification_init(uint8_t port) {
    // 統計カウンタ初期化
    memset(&g_phase2_stats, 0, sizeof(axon_phase2_stats_t));
    
    // 制御構造体初期化
    memset(&g_phase2_config, 0, sizeof(axon_phase2_config_t));
    g_phase2_config.target_port = port;
    g_phase2_config.verification_mode_enabled = false;
    g_phase2_config.auto_response_enabled = false;
    g_phase2_config.detailed_logging_enabled = false;
    g_phase2_config.continuous_monitoring = false;
    g_phase2_config.current_test_phase = 0;
    g_phase2_config.monitoring_interval_ms = 1000; // デフォルト1秒
}

void axon_phase2_verification_start(void) {
    g_phase2_config.verification_mode_enabled = true;
    g_phase2_stats.verification_start_time_ms = get_system_time_ms();
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Verification started at %u ms\n", 
               g_phase2_stats.verification_start_time_ms);
        printf("[PHASE2_VERIFY] Target PORT: %d\n", g_phase2_config.target_port);
    }
}

void axon_phase2_verification_stop(void) {
    g_phase2_config.verification_mode_enabled = false;
    g_phase2_stats.last_verification_time_ms = get_system_time_ms();
    g_phase2_stats.total_verification_time_ms = 
        g_phase2_stats.last_verification_time_ms - g_phase2_stats.verification_start_time_ms;
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Verification stopped at %u ms\n", 
               g_phase2_stats.last_verification_time_ms);
        printf("[PHASE2_VERIFY] Total time: %u ms\n", 
               g_phase2_stats.total_verification_time_ms);
        axon_phase2_print_stats();
    }
}

void axon_phase2_verification_reset_stats(void) {
    uint32_t start_time = g_phase2_stats.verification_start_time_ms;
    memset(&g_phase2_stats, 0, sizeof(axon_phase2_stats_t));
    g_phase2_stats.verification_start_time_ms = start_time;
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Statistics reset\n");
    }
}

// ========================================
// T2.1: FRAMカウンタAPI記録関数
// ========================================

void axon_phase2_record_fram_increment(bool overflow_prevented) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.fram_increment_count++;
    if (overflow_prevented) {
        g_phase2_stats.fram_overflow_prevented++;
    }
    
    log_event("FRAM_INCREMENT");
    
    if (g_phase2_config.detailed_logging_enabled && overflow_prevented) {
        printf("[PHASE2_VERIFY] FRAM overflow prevented (count=%u)\n", 
               g_phase2_stats.fram_overflow_prevented);
    }
}

// ========================================
// T2.2: ダイヤル回転検出記録関数
// ========================================

void axon_phase2_record_rotation_detected(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.rotation_detected_count++;
    log_event("ROTATION_DETECTED");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Rotation detected (total=%u)\n", 
               g_phase2_stats.rotation_detected_count);
    }
}

void axon_phase2_record_rotation_state_transition(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.rotation_state_transitions++;
    log_event("ROTATION_STATE_TRANSITION");
}

void axon_phase2_record_rotation_debounce(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.rotation_debounce_count++;
    log_event("ROTATION_DEBOUNCE");
}

void axon_phase2_record_rotation_timeout(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.rotation_timeout_count++;
    log_event("ROTATION_TIMEOUT");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] ERROR: Rotation timeout (total=%u)\n", 
               g_phase2_stats.rotation_timeout_count);
    }
}

// ========================================
// T2.3: 購入処理記録関数
// ========================================

void axon_phase2_record_purchase(bool is_cashless) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    if (is_cashless) {
        g_phase2_stats.cashless_purchase_count++;
        log_event("CASHLESS_PURCHASE");
    } else {
        g_phase2_stats.cash_purchase_count++;
        log_event("CASH_PURCHASE");
    }
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Purchase completed (%s, total=%u/%u)\n",
               is_cashless ? "Cashless" : "Cash",
               g_phase2_stats.cash_purchase_count,
               g_phase2_stats.cashless_purchase_count);
    }
}

void axon_phase2_record_purchase_state_transition(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.purchase_state_transitions++;
    log_event("PURCHASE_STATE_TRANSITION");
}

// ========================================
// T2.4: 非購入処理記録関数
// ========================================

void axon_phase2_record_non_purchase(non_purchase_reason_t reason) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    switch (reason) {
        case NON_PURCHASE_REASON_REFUND:
            g_phase2_stats.cash_refund_count++;
            log_event("CASH_REFUND");
            if (g_phase2_config.detailed_logging_enabled) {
                printf("[PHASE2_VERIFY] Cash refund (total=%u)\n", 
                       g_phase2_stats.cash_refund_count);
            }
            break;
            
        case NON_PURCHASE_REASON_CANCEL:
            g_phase2_stats.cashless_cancel_count++;
            log_event("CASHLESS_CANCEL");
            if (g_phase2_config.detailed_logging_enabled) {
                printf("[PHASE2_VERIFY] Cashless cancel (total=%u)\n", 
                       g_phase2_stats.cashless_cancel_count);
            }
            break;
            
        case NON_PURCHASE_REASON_TIMEOUT:
            g_phase2_stats.timeout_count++;
            log_event("PURCHASE_TIMEOUT");
            if (g_phase2_config.detailed_logging_enabled) {
                printf("[PHASE2_VERIFY] Purchase timeout (total=%u)\n", 
                       g_phase2_stats.timeout_count);
            }
            break;
    }
}

// ========================================
// T2.5: 面番号変更記録関数
// ========================================

void axon_phase2_record_face_change(bool is_duplicate, bool is_zero_mode) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.face_change_count++;
    
    if (is_duplicate) {
        g_phase2_stats.face_duplicate_prevented++;
    }
    
    if (is_zero_mode) {
        g_phase2_stats.face_zero_mode_count++;
    }
    
    log_event("FACE_CHANGE");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Face change (total=%u, duplicate=%u, zero_mode=%u)\n",
               g_phase2_stats.face_change_count,
               g_phase2_stats.face_duplicate_prevented,
               g_phase2_stats.face_zero_mode_count);
    }
}

void axon_phase2_record_face_data_inherited(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.face_data_inherited_count++;
    log_event("FACE_DATA_INHERITED");
}

// ========================================
// T2.6: 運用動作記録関数
// ========================================

void axon_phase2_record_door_event(bool is_open) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    if (is_open) {
        g_phase2_stats.door_open_count++;
        log_event("DOOR_OPEN");
    } else {
        g_phase2_stats.door_close_count++;
        log_event("DOOR_CLOSE");
    }
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Door %s (open=%u, close=%u)\n",
               is_open ? "opened" : "closed",
               g_phase2_stats.door_open_count,
               g_phase2_stats.door_close_count);
    }
}

void axon_phase2_record_sold_out(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.sold_out_detected_count++;
    log_event("SOLD_OUT");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Sold out detected (total=%u)\n", 
               g_phase2_stats.sold_out_detected_count);
    }
}

void axon_phase2_record_sold_out_cleared(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.sold_out_cleared_count++;
    log_event("SOLD_OUT_CLEARED");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Sold out cleared (total=%u)\n", 
               g_phase2_stats.sold_out_cleared_count);
    }
}

void axon_phase2_record_led_control(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.led_control_count++;
    log_event("LED_CONTROL");
}

// ========================================
// 通信・エラー記録関数
// ========================================

void axon_phase2_record_chkirq_received(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.chkirq_received++;
    log_event("CHKIRQ_RECEIVED");
}

void axon_phase2_record_atirq_sent(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.atirq_sent++;
    log_event("ATIRQ_SENT");
}

void axon_phase2_record_setaxon_received(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.setaxon_received++;
    log_event("SETAXON_RECEIVED");
}

void axon_phase2_record_ack_sent(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.ack_sent++;
    log_event("ACK_SENT");
}

void axon_phase2_record_nop_received(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.nop_received++;
    log_event("NOP_RECEIVED");
}

void axon_phase2_record_crc_error(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.crc_error_count++;
    log_event("CRC_ERROR");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] ERROR: CRC error (total=%u)\n", 
               g_phase2_stats.crc_error_count);
    }
}

void axon_phase2_record_nack_sent(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.nack_sent++;
    log_event("NACK_SENT");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] NACK sent (total=%u)\n", 
               g_phase2_stats.nack_sent);
    }
}

void axon_phase2_record_communication_timeout(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.communication_timeout++;
    log_event("COMMUNICATION_TIMEOUT");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] ERROR: Communication timeout (total=%u)\n", 
               g_phase2_stats.communication_timeout);
    }
}

void axon_phase2_record_invalid_state_transition(void) {
    if (!g_phase2_config.verification_mode_enabled) return;
    
    g_phase2_stats.invalid_state_transition++;
    log_event("INVALID_STATE_TRANSITION");
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] ERROR: Invalid state transition (total=%u)\n", 
               g_phase2_stats.invalid_state_transition);
    }
}

// ========================================
// 統計取得・レポート関数
// ========================================

void axon_phase2_print_stats(void) {
    printf("\n");
    printf("========================================\n");
    printf("  Phase 2 Verification Statistics\n");
    printf("========================================\n");
    printf("Target PORT: %d\n", g_phase2_config.target_port);
    printf("Test Phase: %d\n", g_phase2_config.current_test_phase);
    printf("Total Time: %u ms\n", g_phase2_stats.total_verification_time_ms);
    printf("\n");
    
    printf("--- T2.1: FRAM Counter API ---\n");
    printf("  Increment Count:       %u\n", g_phase2_stats.fram_increment_count);
    printf("  Overflow Prevented:    %u\n", g_phase2_stats.fram_overflow_prevented);
    printf("\n");
    
    printf("--- T2.2: Dial Rotation Detection ---\n");
    printf("  Rotation Detected:     %u\n", g_phase2_stats.rotation_detected_count);
    printf("  State Transitions:     %u\n", g_phase2_stats.rotation_state_transitions);
    printf("  Debounce Count:        %u\n", g_phase2_stats.rotation_debounce_count);
    printf("  Timeout Count:         %u\n", g_phase2_stats.rotation_timeout_count);
    printf("\n");
    
    printf("--- T2.3: Purchase Processing ---\n");
    printf("  Cash Purchase:         %u\n", g_phase2_stats.cash_purchase_count);
    printf("  Cashless Purchase:     %u\n", g_phase2_stats.cashless_purchase_count);
    printf("  State Transitions:     %u\n", g_phase2_stats.purchase_state_transitions);
    printf("\n");
    
    printf("--- T2.4: Non-Purchase Processing ---\n");
    printf("  Cash Refund:           %u\n", g_phase2_stats.cash_refund_count);
    printf("  Cashless Cancel:       %u\n", g_phase2_stats.cashless_cancel_count);
    printf("  Timeout:               %u\n", g_phase2_stats.timeout_count);
    printf("\n");
    
    printf("--- T2.5: Face Number Change ---\n");
    printf("  Face Change:           %u\n", g_phase2_stats.face_change_count);
    printf("  Duplicate Prevented:   %u\n", g_phase2_stats.face_duplicate_prevented);
    printf("  Zero Mode:             %u\n", g_phase2_stats.face_zero_mode_count);
    printf("  Data Inherited:        %u\n", g_phase2_stats.face_data_inherited_count);
    printf("\n");
    
    printf("--- T2.6: Operation ---\n");
    printf("  Door Open:             %u\n", g_phase2_stats.door_open_count);
    printf("  Door Close:            %u\n", g_phase2_stats.door_close_count);
    printf("  Sold Out Detected:     %u\n", g_phase2_stats.sold_out_detected_count);
    printf("  Sold Out Cleared:      %u\n", g_phase2_stats.sold_out_cleared_count);
    printf("  LED Control:           %u\n", g_phase2_stats.led_control_count);
    printf("\n");
    
    printf("--- Communication Statistics ---\n");
    printf("  CHKIRQ Received:       %u\n", g_phase2_stats.chkirq_received);
    printf("  ATIRQ Sent:            %u\n", g_phase2_stats.atirq_sent);
    printf("  SETAXON Received:      %u\n", g_phase2_stats.setaxon_received);
    printf("  ACK Sent:              %u\n", g_phase2_stats.ack_sent);
    printf("  NOP Received:          %u\n", g_phase2_stats.nop_received);
    printf("\n");
    
    printf("--- Error Statistics ---\n");
    printf("  CRC Error:             %u\n", g_phase2_stats.crc_error_count);
    printf("  NACK Sent:             %u\n", g_phase2_stats.nack_sent);
    printf("  Communication Timeout: %u\n", g_phase2_stats.communication_timeout);
    printf("  Invalid State Trans:   %u\n", g_phase2_stats.invalid_state_transition);
    printf("\n");
    
    // 成功率計算
    uint32_t total_comm = g_phase2_stats.chkirq_received + g_phase2_stats.setaxon_received;
    uint32_t total_errors = g_phase2_stats.crc_error_count + 
                            g_phase2_stats.nack_sent + 
                            g_phase2_stats.communication_timeout;
    
    if (total_comm > 0) {
        float success_rate = (float)(total_comm - total_errors) / (float)total_comm * 100.0f;
        printf("--- Success Rate ---\n");
        printf("  Total Commands:        %u\n", total_comm);
        printf("  Total Errors:          %u\n", total_errors);
        printf("  Success Rate:          %.2f%%\n", success_rate);
        printf("\n");
    }
    
    printf("========================================\n\n");
}

axon_phase2_stats_t* axon_phase2_get_stats(void) {
    return &g_phase2_stats;
}

bool axon_phase2_is_enabled(void) {
    return g_phase2_config.verification_mode_enabled;
}

void axon_phase2_set_auto_response(bool enabled) {
    g_phase2_config.auto_response_enabled = enabled;
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Auto response %s\n", enabled ? "enabled" : "disabled");
    }
}

void axon_phase2_set_detailed_logging(bool enabled) {
    g_phase2_config.detailed_logging_enabled = enabled;
}

void axon_phase2_set_continuous_monitoring(bool enabled, uint16_t interval_ms) {
    g_phase2_config.continuous_monitoring = enabled;
    g_phase2_config.monitoring_interval_ms = interval_ms;
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Continuous monitoring %s (interval=%u ms)\n", 
               enabled ? "enabled" : "disabled", interval_ms);
    }
}

void axon_phase2_set_test_phase(uint8_t phase) {
    g_phase2_config.current_test_phase = phase;
    
    if (g_phase2_config.detailed_logging_enabled) {
        printf("[PHASE2_VERIFY] Test phase set to %d\n", phase);
    }
}

// ========================================
// 内部関数実装
// ========================================

/**
 * @brief システム時刻取得（ミリ秒）
 * 
 * @note 実装はプラットフォーム依存（systick等を使用）
 * @return uint32_t 現在時刻（ミリ秒）
 */
static uint32_t get_system_time_ms(void) {
    // TODO: systickベースの時刻取得実装
    // 仮実装（実際のsystickカウンタ使用に置き換える）
    return 0; // 実装待ち
}

/**
 * @brief イベントログ出力
 * 
 * @param event_name イベント名
 */
static void log_event(const char* event_name) {
    if (!g_phase2_config.detailed_logging_enabled) return;
    
    uint32_t current_time = get_system_time_ms();
    printf("[%u ms] %s\n", current_time, event_name);
}
