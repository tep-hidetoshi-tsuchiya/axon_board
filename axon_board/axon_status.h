#ifndef AXON_STATUS_H
#define AXON_STATUS_H

#include <stdint.h>

// ATIRQ STATUS bit definitions (Table 4-14準拠、16ビット)
// bit0: FACE有効フラグ (1=有効)
// bit1: 売り切れ (1=売り切れ)
// bit2: 電子マネーソレノイド (1=ON)
// bit3: 光センサー (1=検知)
// bit4: 返却ボタン (1=押下)
// bit5: 現金ブロック (1=ON)
// bit6: ドア開閉 (1=開)
// bit7-15: RFU
#define STATUS_FACE_VALID_BIT    (1U << 0)  // FACE有効
#define STATUS_SOLD_OUT_BIT      (1U << 1)  // 売り切れ
#define STATUS_EMONEY_SOL_BIT    (1U << 2)  // 電子マネーソレノイド
#define STATUS_LIGHT_SENSOR_BIT  (1U << 3)  // 光センサー
#define STATUS_RETURN_BTN_BIT    (1U << 4)  // 返却ボタン
#define STATUS_CASH_BLOCK_BIT    (1U << 5)  // 現金ブロック
#define STATUS_DOOR_OPEN_BIT     (1U << 6)  // ドア開閉

// 内部状態管理用（ATIRQには含まれない）
#define ROT_DET_BIT     (1U << 0)  // 回転検出（内部用）
#define BLK_ON_BIT      (1U << 1)  // ブロック中（内部用）
#define COIN_DET_BIT    (1U << 2)  // 硬貨検出（内部用）
#define ESCRW_DET_BIT   (1U << 3)  // 返金検出（内部用）
#define ERROR_STATE_BIT (1U << 6)  // エラー状態（内部用）
#define MAINT_MODE_BIT  (1U << 7)  // メンテナンスモード（内部用）

/**
 * @brief SOMA-AXON間で共有する状態構造体
 * @details ATIRQレスポンス生成時に参照する各種フラグを保持する。
 */
typedef struct {
    uint8_t  face_number;
    uint16_t cash_value;
    uint8_t  timeout_seconds;
    uint8_t  led_pattern;
    uint8_t  solenoid_on;
    uint8_t  block_solenoid_on;
    uint8_t  dial_rotated;
    uint8_t  coin_detected;
    uint8_t  escrow_detected;
    uint8_t  door_open;
    uint8_t  sold_out;
    uint8_t  error_state;
    uint8_t  maint_mode;
    // イベント優先度管理
    uint8_t  event_priority;  // 0=なし, 1=coin, 2=escrow, 3=dial, 4=error
} axon_status_shared_t;

extern volatile axon_status_shared_t g_axon_status_shared;

/**
 * @brief ATIRQ statusフィールドに詰めるビット列を生成（Table 4-14準拠）
 * @return 16ビットのSTATUS値（リトルエンディアン）
 */
static inline uint16_t axon_status_compose_bits(void) {
    const volatile axon_status_shared_t* s = &g_axon_status_shared;
    uint16_t status = 0U;

    // bit0: FACE有効フラグ（face_numberが0以外なら有効）
    if (s->face_number > 0) {
        status |= STATUS_FACE_VALID_BIT;
    }

    // bit1: 売り切れ
    if (s->sold_out) {
        status |= STATUS_SOLD_OUT_BIT;
    }

    // bit2: 電子マネーソレノイド（ダイヤルロックソレノイド）
    if (s->solenoid_on) {
        status |= STATUS_EMONEY_SOL_BIT;
    }

    // bit3: 光センサー（回転検出）
    if (s->dial_rotated) {
        status |= STATUS_LIGHT_SENSOR_BIT;
    }

    // bit4: 返却ボタン（エスクロ検出）
    if (s->escrow_detected) {
        status |= STATUS_RETURN_BTN_BIT;
    }

    // bit5: 現金ブロック
    if (s->block_solenoid_on) {
        status |= STATUS_CASH_BLOCK_BIT;
    }

    // bit6: ドア開閉
    if (s->door_open) {
        status |= STATUS_DOOR_OPEN_BIT;
    }

    // bit7-15: RFU（予約）

    return status;
}

/**
 * @brief 硬貨検知フラグをラッチ（ATIRQ送信まで保持）
 * @details 優先度1: coinイベント（低優先度）
 */
static inline void axon_status_latch_coin(void) {
    g_axon_status_shared.coin_detected = 1U;
    // 優先度管理: coinは優先度1（既存イベントより低い場合のみ更新）
    if (g_axon_status_shared.event_priority == 0) {
        g_axon_status_shared.event_priority = 1;
    }
}

/**
 * @brief エスクロ検知フラグをラッチ
 * @details 優先度2: escrowイベント（中優先度）
 */
static inline void axon_status_latch_escrow(void) {
    g_axon_status_shared.escrow_detected = 1U;
    // 優先度管理: escrowは優先度2（coinより高い）
    if (g_axon_status_shared.event_priority < 2) {
        g_axon_status_shared.event_priority = 2;
    }
}

/**
 * @brief ダイヤル回転検知フラグをラッチ
 * @details 優先度3: dialイベント（高優先度）
 */
static inline void axon_status_latch_dial(void) {
    g_axon_status_shared.dial_rotated = 1U;
    // 優先度管理: dialは優先度3（最も高い通常イベント）
    if (g_axon_status_shared.event_priority < 3) {
        g_axon_status_shared.event_priority = 3;
    }
}

/**
 * @brief エラー状態フラグをラッチ
 * @details 優先度4: errorイベント（最高優先度）
 */
static inline void axon_status_latch_error(void) {
    g_axon_status_shared.error_state = 1U;
    // 優先度管理: errorは優先度4（最高優先度、常に上書き）
    g_axon_status_shared.event_priority = 4;
}

/**
 * @brief ATIRQ送信後などにイベント系ラッチをクリア
 */
static inline void axon_status_clear_event_latches(void) {
    g_axon_status_shared.coin_detected = 0U;
    g_axon_status_shared.escrow_detected = 0U;
    g_axon_status_shared.dial_rotated = 0U;
    g_axon_status_shared.event_priority = 0U;
}

#endif /* AXON_STATUS_H */
