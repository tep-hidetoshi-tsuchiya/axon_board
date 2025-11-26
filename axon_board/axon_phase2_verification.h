/**
 * @file axon_phase2_verification.h
 * @brief Phase 2実機検証用ヘッダファイル
 * 
 * Phase 2で実装した以下の機能を実機で検証するための統計記録・制御機能を提供
 * - T2.1: fram_increment_counter() API
 * - T2.2: 7.6 ダイヤル回転検出
 * - T2.3: 7.7 通常購入処理
 * - T2.4: 7.8 非購入処理
 * - T2.5: 7.3-7.5 面番号変更
 * - T2.6: 7.9 運用動作
 * 
 * @date 2025-11-25
 * @author GitHub Copilot (Claude Sonnet 4.5)
 */

#ifndef AXON_PHASE2_VERIFICATION_H
#define AXON_PHASE2_VERIFICATION_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Phase 2実機検証統計構造体
 * 
 * 各機能の動作状況を記録し、検証結果の定量評価を可能にする
 */
typedef struct {
    // T2.1: fram_increment_counter() API
    uint32_t fram_increment_count;        ///< インクリメント呼び出し回数
    uint32_t fram_overflow_prevented;     ///< オーバーフロー防止回数（0xFFFFで停止）
    
    // T2.2: ダイヤル回転検出
    uint32_t rotation_detected_count;     ///< 回転検出成功回数（Complete状態到達）
    uint32_t rotation_state_transitions;  ///< 状態遷移回数（Idle→Pulse_1→...）
    uint32_t rotation_debounce_count;     ///< デバウンス処理回数（1ms以内の無視）
    uint32_t rotation_timeout_count;      ///< 回転タイムアウト回数（10秒以内に完了しない）
    
    // T2.3: 通常購入処理
    uint32_t cash_purchase_count;         ///< 現金決済完了回数
    uint32_t cashless_purchase_count;     ///< キャッシュレス決済完了回数
    uint32_t purchase_state_transitions;  ///< 購入状態遷移回数（12状態機械）
    
    // T2.4: 非購入処理
    uint32_t cash_refund_count;           ///< 現金返金回数（エスクロSW押下）
    uint32_t cashless_cancel_count;       ///< キャッシュレス中止回数
    uint32_t timeout_count;               ///< タイムアウト発生回数（投入→回転なし）
    
    // T2.5: 面番号変更
    uint32_t face_change_count;           ///< 面番号変更回数（長押し操作）
    uint32_t face_duplicate_prevented;    ///< 重複防止回数（同じ面番号を避ける）
    uint32_t face_zero_mode_count;        ///< 0面モード設定回数（無料配布）
    uint32_t face_data_inherited_count;   ///< データ継承回数（既存カウンタ保持）
    
    // T2.6: 運用動作
    uint32_t door_open_count;             ///< ドア開検知回数
    uint32_t door_close_count;            ///< ドア閉検知回数
    uint32_t sold_out_detected_count;     ///< 売り切れ検知回数（磁気センサー）
    uint32_t sold_out_cleared_count;      ///< 売り切れ解除回数（補充後）
    uint32_t led_control_count;           ///< LED制御回数（ON/OFF/点滅）
    
    // 通信統計（SOMA-AXON間）
    uint32_t chkirq_received;             ///< CHKIRQ受信回数
    uint32_t atirq_sent;                  ///< ATIRQ送信回数
    uint32_t setaxon_received;            ///< SETAXON受信回数
    uint32_t ack_sent;                    ///< ACK送信回数
    uint32_t nop_received;                ///< NOP受信回数（定期通信確認）
    
    // エラー統計
    uint32_t crc_error_count;             ///< CRC異常回数（通信エラー）
    uint32_t nack_sent;                   ///< NACK送信回数（処理失敗）
    uint32_t communication_timeout;       ///< 通信タイムアウト回数（100ms以内に応答なし）
    uint32_t invalid_state_transition;    ///< 不正状態遷移回数（状態機械エラー）
    
    // タイミング情報
    uint32_t last_verification_time_ms;   ///< 最終検証時刻（systickベース）
    uint32_t verification_start_time_ms;  ///< 検証開始時刻
    uint32_t total_verification_time_ms;  ///< 累積検証時間
    
} axon_phase2_stats_t;

/**
 * @brief Phase 2検証制御構造体
 * 
 * 検証モードの動作パラメータを管理
 */
typedef struct {
    bool verification_mode_enabled;       ///< 検証モード有効フラグ
    bool auto_response_enabled;           ///< 自動応答有効フラグ（CHKIRQに自動ATIRQ）
    bool detailed_logging_enabled;        ///< 詳細ログ有効フラグ（UART出力）
    bool continuous_monitoring;           ///< 連続監視モード（定期ATIRQ送信）
    uint8_t target_port;                  ///< 対象ポート番号（1-9）
    uint8_t current_test_phase;           ///< 現在のテストフェーズ (1-6)
    uint16_t monitoring_interval_ms;      ///< 監視間隔（連続監視モード時、デフォルト1000ms）
} axon_phase2_config_t;

/**
 * @brief 非購入処理の理由コード
 */
typedef enum {
    NON_PURCHASE_REASON_REFUND = 0,       ///< 現金返金（エスクロSW押下）
    NON_PURCHASE_REASON_CANCEL = 1,       ///< キャッシュレス中止
    NON_PURCHASE_REASON_TIMEOUT = 2,      ///< タイムアウト
} non_purchase_reason_t;

// ========================================
// API関数宣言
// ========================================

/**
 * @brief Phase 2検証モード初期化
 * 
 * 統計カウンタをゼロクリアし、検証モードを初期状態にする
 * 
 * @param port 対象ポート番号（1-9）
 */
void axon_phase2_verification_init(uint8_t port);

/**
 * @brief 検証モード開始
 * 
 * verification_mode_enabled = true に設定し、統計記録を開始
 * 検証開始時刻を記録
 */
void axon_phase2_verification_start(void);

/**
 * @brief 検証モード停止
 * 
 * verification_mode_enabled = false に設定し、統計記録を停止
 * 最終統計レポートを出力
 */
void axon_phase2_verification_stop(void);

/**
 * @brief 統計カウンタリセット
 * 
 * 全統計カウンタを0にリセット（検証再実行時に使用）
 */
void axon_phase2_verification_reset_stats(void);

// ========================================
// T2.1: FRAMカウンタAPI記録関数
// ========================================

/**
 * @brief FRAMインクリメント記録
 * 
 * fram_increment_counter() 呼び出し時に記録
 * 
 * @param overflow_prevented true: オーバーフロー防止動作した（値が0xFFFF）
 */
void axon_phase2_record_fram_increment(bool overflow_prevented);

// ========================================
// T2.2: ダイヤル回転検出記録関数
// ========================================

/**
 * @brief ダイヤル回転検出記録
 * 
 * Complete状態到達時に呼び出し
 */
void axon_phase2_record_rotation_detected(void);

/**
 * @brief ダイヤル回転状態遷移記録
 * 
 * 状態機械が遷移するたびに呼び出し（Idle→Pulse_1→Interval→Pulse_2→Complete）
 */
void axon_phase2_record_rotation_state_transition(void);

/**
 * @brief デバウンス処理記録
 * 
 * 1ms以内の信号変化を無視した時に呼び出し
 */
void axon_phase2_record_rotation_debounce(void);

/**
 * @brief 回転タイムアウト記録
 * 
 * 10秒以内に回転が完了しなかった時に呼び出し
 */
void axon_phase2_record_rotation_timeout(void);

// ========================================
// T2.3: 購入処理記録関数
// ========================================

/**
 * @brief 購入完了記録
 * 
 * 購入処理が完了した時に呼び出し
 * 
 * @param is_cashless true: キャッシュレス決済, false: 現金決済
 */
void axon_phase2_record_purchase(bool is_cashless);

/**
 * @brief 購入状態遷移記録
 * 
 * 12状態機械が遷移するたびに呼び出し
 */
void axon_phase2_record_purchase_state_transition(void);

// ========================================
// T2.4: 非購入処理記録関数
// ========================================

/**
 * @brief 非購入処理記録
 * 
 * 返金・中止・タイムアウトが発生した時に呼び出し
 * 
 * @param reason 非購入理由（NON_PURCHASE_REASON_xxx）
 */
void axon_phase2_record_non_purchase(non_purchase_reason_t reason);

// ========================================
// T2.5: 面番号変更記録関数
// ========================================

/**
 * @brief 面番号変更記録
 * 
 * 面番号変更が完了した時に呼び出し
 * 
 * @param is_duplicate true: 重複防止動作した
 * @param is_zero_mode true: 0面モード（無料配布）に設定
 */
void axon_phase2_record_face_change(bool is_duplicate, bool is_zero_mode);

/**
 * @brief データ継承記録
 * 
 * 面番号変更時に既存カウンタ値を保持した時に呼び出し
 */
void axon_phase2_record_face_data_inherited(void);

// ========================================
// T2.6: 運用動作記録関数
// ========================================

/**
 * @brief ドア開閉イベント記録
 * 
 * ドアセンサー変化時に呼び出し
 * 
 * @param is_open true: ドア開, false: ドア閉
 */
void axon_phase2_record_door_event(bool is_open);

/**
 * @brief 売り切れ検知記録
 * 
 * 磁気センサーが売り切れ状態を検知した時に呼び出し
 */
void axon_phase2_record_sold_out(void);

/**
 * @brief 売り切れ解除記録
 * 
 * 補充後、売り切れ状態が解除された時に呼び出し
 */
void axon_phase2_record_sold_out_cleared(void);

/**
 * @brief LED制御記録
 * 
 * LEDのON/OFF/点滅制御を実行した時に呼び出し
 */
void axon_phase2_record_led_control(void);

// ========================================
// 通信・エラー記録関数
// ========================================

/**
 * @brief CHKIRQ受信記録
 */
void axon_phase2_record_chkirq_received(void);

/**
 * @brief ATIRQ送信記録
 */
void axon_phase2_record_atirq_sent(void);

/**
 * @brief SETAXON受信記録
 */
void axon_phase2_record_setaxon_received(void);

/**
 * @brief ACK送信記録
 */
void axon_phase2_record_ack_sent(void);

/**
 * @brief NOP受信記録
 */
void axon_phase2_record_nop_received(void);

/**
 * @brief CRC異常記録
 */
void axon_phase2_record_crc_error(void);

/**
 * @brief NACK送信記録
 */
void axon_phase2_record_nack_sent(void);

/**
 * @brief 通信タイムアウト記録
 */
void axon_phase2_record_communication_timeout(void);

/**
 * @brief 不正状態遷移記録
 */
void axon_phase2_record_invalid_state_transition(void);

// ========================================
// 統計取得・レポート関数
// ========================================

/**
 * @brief 統計レポート出力
 * 
 * UART経由で詳細な統計情報を出力（detailed_logging_enabled時）
 */
void axon_phase2_print_stats(void);

/**
 * @brief 統計構造体ポインタ取得
 * 
 * @return axon_phase2_stats_t* 統計構造体へのポインタ
 */
axon_phase2_stats_t* axon_phase2_get_stats(void);

/**
 * @brief 検証モード有効確認
 * 
 * @return true: 検証モード有効, false: 無効
 */
bool axon_phase2_is_enabled(void);

/**
 * @brief 自動応答モード設定
 * 
 * @param enabled true: 自動応答有効, false: 無効
 */
void axon_phase2_set_auto_response(bool enabled);

/**
 * @brief 詳細ログ設定
 * 
 * @param enabled true: 詳細ログ有効, false: 無効
 */
void axon_phase2_set_detailed_logging(bool enabled);

/**
 * @brief 連続監視モード設定
 * 
 * @param enabled true: 連続監視有効, false: 無効
 * @param interval_ms 監視間隔（ミリ秒）
 */
void axon_phase2_set_continuous_monitoring(bool enabled, uint16_t interval_ms);

/**
 * @brief 現在のテストフェーズ設定
 * 
 * @param phase テストフェーズ番号（1-6）
 */
void axon_phase2_set_test_phase(uint8_t phase);

#endif // AXON_PHASE2_VERIFICATION_H
