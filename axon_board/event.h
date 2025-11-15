#ifndef __MSPM0_EVENT_H__
#define __MSPM0_EVENT_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/systick.h"

/*
 * POC用の仮定義
 */

/// @brief POC用 周辺機器入力イベント
/// @note プルアップなどの回路側の仕様はisr側で吸収すること
struct peripheral_input_event {
    volatile uint8_t pressed;             // 0:押されていない, 1:押された
    systick_t        phase_time;          // ボタンの状態が変化した時間
    uint8_t          last_state;          // 最後の状態
    uint32_t         debounce_us;         // デバウンス用設定値
    systick_t        last_press_time_ms;  // 最後に押された時間(ms)
};

typedef struct peripheral_input_event button_event_t;

/// @brief POC用 LED状態
typedef struct {
    uint8_t   status;      // 0:OFF, 1:PWM, 2:ON
    uint8_t   color;       // 0:赤, 1:緑, 2:青
    uint16_t  count;       // LEDの点滅回数
    uint16_t  brightness;  // LEDの明るさ(duty比 or ON_OFF:0x00 or 0xFFFF)
    systick_t interval;    // LEDの変化間隔
} led_status_t;

/**
 * UART IRQマスク定義
 */

#define UART_IRQ_UART1_MASK (1U << 0)
#define UART_IRQ_UART2_MASK (1U << 1)
#define UART_IRQ_UART3_MASK (1U << 2)
#define UART_IRQ_UART4_MASK (1U << 3)
#define UART_IRQ_UART5_MASK (1U << 4)
#define UART_IRQ_UART6_MASK (1U << 5)
#define UART_IRQ_UART7_MASK (1U << 6)
#define UART_IRQ_UART8_MASK (1U << 7)
#define UART_IRQ_UART9_MASK (1U << 8)
#define UART_IRQ_ALL_MASK   (0x1FFU)

/// @brief UART割り込み状態構造体
/// @note 優先順位は UART_IRQ_UART1_MASK > UART_IRQ_UART2_MASK > ... > UART_IRQ_UART9_MASK
typedef struct {
    uint16_t irq_flags;
} uart_status_t;

/// @brief UART割り込みフラグを設定する
/// @param status UART割り込み状態構造体
/// @param mask 設定するビット (UART_IRQ_UARTx_MASK)
static inline void uart_status_set(uart_status_t* status, uint16_t mask) {
    if (status != NULL) {
        status->irq_flags |= mask;
    }
}

/// @brief UART割り込みフラグをクリアする
/// @param status UART割り込み状態構造体
/// @param mask クリアするビット (UART_IRQ_UARTx_MASK)
static inline void uart_status_clear(uart_status_t* status, uint16_t mask) {
    if (status != NULL) {
        status->irq_flags &= (uint16_t)(~mask);
    }
}

/// @brief UART割り込みフラグが立っているか判定する
/// @param status UART割り込み状態構造体
/// @param mask 判定するビット (UART_IRQ_UARTx_MASK)
/// @return 立っている場合はtrue、その他はfalse
static inline bool uart_status_check(const uart_status_t* status, uint16_t mask) {
    if (status == NULL) {
        return false;
    }
    return (status->irq_flags & mask) != 0U;
}

extern led_status_t   g_led_status;
extern button_event_t g_escrow_event;
extern button_event_t g_rotary_event;
extern uart_status_t  g_uart_status;
extern button_event_t g_button_1_event;
#ifdef AXON_BOARD
extern button_event_t g_button_2_event;
extern button_event_t g_coindet_event;
extern button_event_t g_connector_event;
extern button_event_t g_soldout_event;
extern button_event_t g_door_event;
#endif

#endif /* __MSPM0_EVENT_H__ */