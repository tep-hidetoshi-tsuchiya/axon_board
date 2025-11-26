/**
 * @file soma_axon_comm_test.c
 * @brief AXON側通信テスト実装
 * 
 * SOMA側からのコマンドに対する応答テストとログ記録
 */

#include "soma_axon_comm_test.h"
#include "driver/systick.h"
#include <stdio.h>
#include <string.h>

// テスト統計（グローバル）
static axon_test_stats_t g_test_stats = {0};

// テスト設定（グローバル）
static axon_test_config_t g_test_config = {
    .enable_test_mode = false,
    .log_all_commands = false,
    .force_nack_setaxon = false,
    .simulate_timeout = false,
    .verbose_status = false
};

// コマンド名テーブル
static const char* get_command_name(uint8_t cmd_id) {
    switch (cmd_id) {
        case 0x49: return "CHKIRQ";
        case 0x4A: return "SETAXON";
        case 0x50: return "NOP";
        case 0x11: return "SETOKEY";
        case 0x18: return "AFWUP";
        case 0xA5: return "CODEPKT";
        case 0xC4: return "ERRCHK";
        case 0x6A: return "ATIRQ";
        case 0x00: return "ACK";
        case 0x90: return "NACK";
        case 0xA6: return "CODEOK";
        case 0xA7: return "CODENG";
        case 0xC5: return "CODEFIN";
        default: return "UNKNOWN";
    }
}

/**
 * @brief テストモードを初期化
 */
void axon_test_init(void) {
    memset(&g_test_stats, 0, sizeof(axon_test_stats_t));
    g_test_stats.test_start_time_ms = get_systick_count_ms();
    
    printf("\r\n");
    printf("========================================\r\n");
    printf("  AXON Communication Test Mode\r\n");
    printf("========================================\r\n");
    printf("Test mode initialized\r\n");
    printf("Start time: %u ms\r\n", g_test_stats.test_start_time_ms);
    printf("========================================\r\n");
    printf("\r\n");
}

/**
 * @brief テストモードを有効化
 */
void axon_test_enable(const axon_test_config_t* config) {
    if (config != NULL) {
        memcpy(&g_test_config, config, sizeof(axon_test_config_t));
    } else {
        // デフォルト設定
        g_test_config.enable_test_mode = true;
        g_test_config.log_all_commands = true;
        g_test_config.force_nack_setaxon = false;
        g_test_config.simulate_timeout = false;
        g_test_config.verbose_status = true;
    }
    
    g_test_config.enable_test_mode = true;
    
    printf("\r\n");
    printf("[AXON_TEST] Test mode ENABLED\r\n");
    printf("  - Log all commands: %s\r\n", g_test_config.log_all_commands ? "YES" : "NO");
    printf("  - Force NACK (SETAXON): %s\r\n", g_test_config.force_nack_setaxon ? "YES" : "NO");
    printf("  - Simulate timeout: %s\r\n", g_test_config.simulate_timeout ? "YES" : "NO");
    printf("  - Verbose status: %s\r\n", g_test_config.verbose_status ? "YES" : "NO");
    printf("\r\n");
}

/**
 * @brief テストモードを無効化
 */
void axon_test_disable(void) {
    g_test_config.enable_test_mode = false;
    printf("\r\n[AXON_TEST] Test mode DISABLED\r\n\r\n");
}

/**
 * @brief テスト統計情報を取得
 */
void axon_test_get_stats(axon_test_stats_t* stats) {
    if (stats != NULL) {
        memcpy(stats, &g_test_stats, sizeof(axon_test_stats_t));
    }
}

/**
 * @brief テスト統計情報をリセット
 */
void axon_test_reset_stats(void) {
    uint32_t current_time = get_systick_count_ms();
    memset(&g_test_stats, 0, sizeof(axon_test_stats_t));
    g_test_stats.test_start_time_ms = current_time;
    
    printf("\r\n[AXON_TEST] Statistics reset\r\n\r\n");
}

/**
 * @brief テスト統計情報を出力
 */
void axon_test_print_stats(void) {
    uint32_t current_time = get_systick_count_ms();
    uint32_t duration_ms = current_time - g_test_stats.test_start_time_ms;
    uint32_t total_responses = g_test_stats.atirq_sent + g_test_stats.ack_sent + g_test_stats.nack_sent;
    
    printf("\r\n");
    printf("========================================\r\n");
    printf("  AXON Communication Test Statistics\r\n");
    printf("========================================\r\n");
    printf("\r\n");
    
    // 受信コマンド統計
    printf("--- Received Commands ---\r\n");
    printf("Total:       %u\r\n", g_test_stats.total_received);
    printf("CHKIRQ:      %u\r\n", g_test_stats.chkirq_count);
    printf("SETAXON:     %u\r\n", g_test_stats.setaxon_count);
    printf("NOP:         %u\r\n", g_test_stats.nop_count);
    printf("SETOKEY:     %u\r\n", g_test_stats.setokey_count);
    printf("AFWUP:       %u\r\n", g_test_stats.afwup_count);
    printf("CODEPKT:     %u\r\n", g_test_stats.codepkt_count);
    printf("ERRCHK:      %u\r\n", g_test_stats.errchk_count);
    printf("Unknown:     %u\r\n", g_test_stats.unknown_count);
    printf("\r\n");
    
    // 送信応答統計
    printf("--- Sent Responses ---\r\n");
    printf("Total:       %u\r\n", total_responses);
    printf("ATIRQ:       %u\r\n", g_test_stats.atirq_sent);
    printf("ACK:         %u\r\n", g_test_stats.ack_sent);
    printf("NACK:        %u\r\n", g_test_stats.nack_sent);
    printf("\r\n");
    
    // エラー統計
    printf("--- Errors ---\r\n");
    printf("CRC Errors:  %u\r\n", g_test_stats.crc_error_count);
    printf("\r\n");
    
    // 時間統計
    printf("--- Timing ---\r\n");
    printf("Duration:    %u ms\r\n", duration_ms);
    printf("Last CMD:    %u ms ago\r\n", 
           current_time - g_test_stats.last_command_time_ms);
    
    // 通信レート
    if (duration_ms > 0) {
        float cmd_rate = (float)g_test_stats.total_received / (float)duration_ms * 1000.0f;
        float resp_rate = (float)total_responses / (float)duration_ms * 1000.0f;
        printf("CMD Rate:    %.2f cmd/sec\r\n", cmd_rate);
        printf("RESP Rate:   %.2f resp/sec\r\n", resp_rate);
    }
    
    printf("\r\n");
    printf("========================================\r\n");
    printf("\r\n");
}

/**
 * @brief コマンド受信を記録
 */
void axon_test_record_command(uint8_t cmd_id) {
    if (!g_test_config.enable_test_mode) {
        return;
    }
    
    g_test_stats.total_received++;
    g_test_stats.last_command_time_ms = get_systick_count_ms();
    
    // コマンド別カウント
    switch (cmd_id) {
        case 0x49: g_test_stats.chkirq_count++; break;
        case 0x4A: g_test_stats.setaxon_count++; break;
        case 0x50: g_test_stats.nop_count++; break;
        case 0x11: g_test_stats.setokey_count++; break;
        case 0x18: g_test_stats.afwup_count++; break;
        case 0xA5: g_test_stats.codepkt_count++; break;
        case 0xC4: g_test_stats.errchk_count++; break;
        default: g_test_stats.unknown_count++; break;
    }
    
    // ログ出力
    if (g_test_config.log_all_commands) {
        printf("[AXON_TEST] RX: %s (0x%02X) | Total: %u | Time: %u ms\r\n",
               get_command_name(cmd_id),
               cmd_id,
               g_test_stats.total_received,
               g_test_stats.last_command_time_ms);
    }
}

/**
 * @brief 応答送信を記録
 */
void axon_test_record_response(uint8_t response_id) {
    if (!g_test_config.enable_test_mode) {
        return;
    }
    
    // 応答別カウント
    switch (response_id) {
        case 0x6A: g_test_stats.atirq_sent++; break;
        case 0x00: g_test_stats.ack_sent++; break;
        case 0x90: g_test_stats.nack_sent++; break;
        default: break;
    }
    
    // ログ出力
    if (g_test_config.log_all_commands) {
        printf("[AXON_TEST] TX: %s (0x%02X)\r\n",
               get_command_name(response_id),
               response_id);
    }
}

/**
 * @brief CRCエラーを記録
 */
void axon_test_record_crc_error(void) {
    if (!g_test_config.enable_test_mode) {
        return;
    }
    
    g_test_stats.crc_error_count++;
    
    if (g_test_config.log_all_commands) {
        printf("[AXON_TEST] CRC ERROR | Total errors: %u\r\n",
               g_test_stats.crc_error_count);
    }
}

/**
 * @brief テストモードが有効かチェック
 */
bool axon_test_is_enabled(void) {
    return g_test_config.enable_test_mode;
}

/**
 * @brief SETAXON強制NACKモードかチェック
 */
bool axon_test_force_nack_setaxon(void) {
    return g_test_config.enable_test_mode && g_test_config.force_nack_setaxon;
}

/**
 * @brief タイムアウトシミュレーションかチェック
 */
bool axon_test_simulate_timeout(void) {
    return g_test_config.enable_test_mode && g_test_config.simulate_timeout;
}

/**
 * @brief 詳細ログ出力が有効かチェック
 */
bool axon_test_verbose_logging(void) {
    return g_test_config.enable_test_mode && g_test_config.verbose_status;
}
