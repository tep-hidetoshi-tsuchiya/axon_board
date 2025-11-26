/**
 * @file axon_phase2_autotest.h
 * @brief Phase 2自動テスト機能ヘッダファイル
 * 
 * AXON単体で実行可能な自己診断テスト機能を提供
 * ボタン長押し（5秒）で自動テスト開始
 * 
 * @date 2025-11-25
 * @author GitHub Copilot (Claude Sonnet 4.5)
 */

#ifndef AXON_PHASE2_AUTOTEST_H
#define AXON_PHASE2_AUTOTEST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 自動テスト結果構造体
 */
typedef struct {
    bool tc01_fram_api;           ///< TC-01: FRAMカウンタAPI
    bool tc02_rotation_detect;    ///< TC-02: ダイヤル回転検出
    bool tc03_purchase_flow;      ///< TC-03: 購入フロー
    bool tc04_non_purchase;       ///< TC-04: 非購入フロー
    bool tc05_face_change;        ///< TC-05: 面番号変更
    bool tc06_operation;          ///< TC-06: 運用動作
    uint8_t total_tests;          ///< 総テスト数
    uint8_t passed_tests;         ///< 合格テスト数
} axon_autotest_result_t;

/**
 * @brief 自動テスト実行
 * 
 * Phase2の全機能を自動的にテストし、結果を返す
 * 
 * @return axon_autotest_result_t テスト結果
 */
axon_autotest_result_t axon_phase2_run_autotest(void);

/**
 * @brief 自動テスト結果表示
 * 
 * @param result テスト結果構造体
 */
void axon_phase2_print_autotest_result(const axon_autotest_result_t* result);

/**
 * @brief IRQ信号クリア
 * 
 * SOMA側からCHKIRQパケットを受信した後に呼び出す
 * GPIO PA24をHIGHに戻してIRQ信号を解除
 */
void axon_phase2_clear_irq_signal(void);

/**
 * @brief IRQ信号状態取得
 * 
 * @return true: IRQアサート中, false: IRQ未アサート
 */
bool axon_phase2_is_irq_asserted(void);

/**
 * @brief 自動テストモード中かどうかを取得
 * 
 * @return true: 自動テスト実行中, false: 通常動作中
 */
bool axon_phase2_is_autotest_running(void);

/**
 * @brief TC-01: FRAMカウンタAPIテスト
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc01_fram_api(void);

/**
 * @brief TC-02: ダイヤル回転検出テスト（シミュレーション）
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc02_rotation_detect(void);

/**
 * @brief TC-03: 購入フローテスト（シミュレーション）
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc03_purchase_flow(void);

/**
 * @brief TC-04: 非購入フローテスト（シミュレーション）
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc04_non_purchase(void);

/**
 * @brief TC-05: 面番号変更テスト（シミュレーション）
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc05_face_change(void);

/**
 * @brief TC-06: 運用動作テスト（シミュレーション）
 * 
 * @return true: 合格, false: 不合格
 */
bool axon_autotest_tc06_operation(void);

#endif // AXON_PHASE2_AUTOTEST_H
