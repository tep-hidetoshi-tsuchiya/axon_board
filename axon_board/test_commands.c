/**
 * @file test_commands.c
 * @brief AXON側テストコマンド処理
 * 
 * UARTコンソールから送信されるテストコマンドを処理
 */

#include <stdio.h>
#include <string.h>
#include "soma_axon_comm_test.h"
#include "axon_phase2_autotest.h"
#include "axon_phase2_verification.h"

/**
 * @brief テストコマンドをチェックして実行
 * @param cmd_str コマンド文字列
 */
void axon_test_process_command(const char* cmd_str) {
    if (cmd_str == NULL) {
        return;
    }
    
    // "test on" - テストモード有効化
    if (strcmp(cmd_str, "test on") == 0) {
        axon_test_enable(NULL);
        return;
    }
    
    // "test off" - テストモード無効化
    if (strcmp(cmd_str, "test off") == 0) {
        axon_test_disable();
        return;
    }
    
    // "test stats" - 統計表示
    if (strcmp(cmd_str, "test stats") == 0) {
        axon_test_print_stats();
        return;
    }
    
    // "test reset" - 統計リセット
    if (strcmp(cmd_str, "test reset") == 0) {
        axon_test_reset_stats();
        return;
    }
    
    // "autotest" - Phase2自動テスト実行
    if (strcmp(cmd_str, "autotest") == 0) {
        printf("\n[CMD] Running Phase2 Auto Test...\n");
        axon_autotest_result_t result = axon_phase2_run_autotest();
        axon_phase2_print_autotest_result(&result);
        printf("\n=== Phase2 Detailed Statistics ===\n");
        axon_phase2_print_stats();
        return;
    }
}
