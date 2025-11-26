/**
 * @file soma_axon_comm_test.h
 * @brief AXON側通信テストヘッダー
 * 
 * SOMA側からのコマンドに対する応答テスト
 */

#ifndef SOMA_AXON_COMM_TEST_H_
#define SOMA_AXON_COMM_TEST_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief テスト統計情報
 */
typedef struct {
    uint32_t total_received;        // 総受信コマンド数
    uint32_t chkirq_count;          // CHKIRQ受信回数
    uint32_t setaxon_count;         // SETAXON受信回数
    uint32_t nop_count;             // NOP受信回数
    uint32_t setokey_count;         // SETOKEY受信回数
    uint32_t afwup_count;           // AFWUP受信回数
    uint32_t codepkt_count;         // CODEPKT受信回数
    uint32_t errchk_count;          // ERRCHK受信回数
    uint32_t unknown_count;         // 未知のコマンド
    uint32_t crc_error_count;       // CRCエラー数
    uint32_t atirq_sent;            // ATIRQ送信回数
    uint32_t ack_sent;              // ACK送信回数
    uint32_t nack_sent;             // NACK送信回数
    uint32_t test_start_time_ms;    // テスト開始時刻
    uint32_t last_command_time_ms;  // 最終コマンド受信時刻
} axon_test_stats_t;

/**
 * @brief テストモード設定
 */
typedef struct {
    bool enable_test_mode;          // テストモード有効
    bool log_all_commands;          // 全コマンドをログ出力
    bool force_nack_setaxon;        // SETAXON強制NACK（パラメータエラーテスト用）
    bool simulate_timeout;          // タイムアウトシミュレーション（応答しない）
    bool verbose_status;            // STATUS詳細表示
} axon_test_config_t;

/**
 * @brief テストモードを初期化
 */
void axon_test_init(void);

/**
 * @brief テストモードを有効化
 * @param config テスト設定（NULLの場合はデフォルト）
 */
void axon_test_enable(const axon_test_config_t* config);

/**
 * @brief テストモードを無効化
 */
void axon_test_disable(void);

/**
 * @brief テスト統計情報を取得
 * @param stats 統計情報格納先
 */
void axon_test_get_stats(axon_test_stats_t* stats);

/**
 * @brief テスト統計情報をリセット
 */
void axon_test_reset_stats(void);

/**
 * @brief テスト統計情報を出力
 */
void axon_test_print_stats(void);

/**
 * @brief コマンド受信を記録（内部使用）
 * @param cmd_id コマンドID
 */
void axon_test_record_command(uint8_t cmd_id);

/**
 * @brief 応答送信を記録（内部使用）
 * @param response_id 応答ID
 */
void axon_test_record_response(uint8_t response_id);

/**
 * @brief CRCエラーを記録（内部使用）
 */
void axon_test_record_crc_error(void);

/**
 * @brief テストモードが有効かチェック
 * @return true: 有効, false: 無効
 */
bool axon_test_is_enabled(void);

/**
 * @brief SETAXON強制NACKモードかチェック
 * @return true: 強制NACK, false: 通常動作
 */
bool axon_test_force_nack_setaxon(void);

/**
 * @brief タイムアウトシミュレーションかチェック
 * @return true: 応答しない, false: 通常応答
 */
bool axon_test_simulate_timeout(void);

/**
 * @brief 詳細ログ出力が有効かチェック
 * @return true: 有効, false: 無効
 */
bool axon_test_verbose_logging(void);

#endif /* SOMA_AXON_COMM_TEST_H_ */
