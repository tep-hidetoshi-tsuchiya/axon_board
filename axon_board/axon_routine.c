#ifndef AXON_BOARD
#define AXON_BOARD
#endif

#include "ti_msp_dl_config.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "driver_config.h"
#include "peripheral/led_utils.h"
#include "peripheral/msp_peripheral_config.h"
#include "uart_packet.h"
#include "event.h"
#include "soma_uart_test.h"

#define POLLING_INTERVAL_MS (500U)
#define SWITCH_DEBOUNCE_US  (50U)
#define DEBOUNCE_CYCLES_1US (CPUCLK_FREQ / 1000000U)
#define BUTTON_INTERVAL_MS  (250U)

typedef enum {
    STATE_NONE,
    STATE_NORMAL,
    STATE_SOL_ON,
    STATE_DIAL_DETECT,
    STATE_NOTIFY,
} terminal_status_t;

typedef struct {
    terminal_status_t status;
    uint8_t           amount;
    uint8_t           led_state;
    uint8_t           sol_state;
    uint8_t           dial_state;
    systick_t         last_poll_time;
    uint8_t           dial_detect;
    uint8_t           left_amount;   // 0-9
    uint8_t           right_amount;  // 0-9
} axon_status_t;

#ifdef AXON_BOARD
button_event_t g_rotary_event;

button_event_t g_escrow_event;      // エスクロ検知
button_event_t g_coindet_event;     // 現金検知
button_event_t g_connector_event;   // コネクタ検知(予備GPIO)
button_event_t g_soldout_event;     // 売り切れ検知SW
button_event_t g_door_event;        // ドア開閉検知

button_event_t g_button_1_event;
button_event_t g_button_2_event;

static inline void _change_status(axon_status_t* axon, terminal_status_t new_status) {
    if (axon->status != new_status &&
        (axon->status != STATE_SOL_ON || (new_status == STATE_NORMAL && axon->status == STATE_DIAL_DETECT) ||
         (new_status == STATE_SOL_ON && axon->status == STATE_DIAL_DETECT) ||
         (new_status == STATE_DIAL_DETECT && axon->status == STATE_SOL_ON))) {
        axon->status = new_status;
    }
}

static inline uint8_t _generate_uart_terminal_status_data(const uint8_t amount, const uint8_t led, const uint8_t sol,
                                                          const uint8_t dial) {
    uint8_t data = 0;

    data |= (amount & 0x0FU) << 4;  // bits 4-7: amount (0 to 15)
    data |= (led & 0x03U) << 2;     // bits 2-3: LED status (0 to 3)
    data |= (sol & 0x01U) << 1;     // bit 1: solenoid status (0 or 1)
    data |= (dial & 0x01U);         // bit 0: dial status (0 or 1)

    return data;
}

static inline void _parse_uart_terminal_status_data(const uint8_t data, uint8_t* amount, uint8_t* led, uint8_t* sol,
                                                    uint8_t* dial) {
    if (amount != NULL) {
        *amount = (data >> 4) & 0x0FU;  // bits 4-7: amount (0 to 15)
    }
    if (led != NULL) {
        *led = (data >> 2) & 0x03U;  // bits 2-3: LED status (0 to 3)
    }
    if (sol != NULL) {
        *sol = (data >> 1) & 0x01U;  // bit 1: solenoid status (0 or 1)
    }
    if (dial != NULL) {
        *dial = data & 0x01U;  // bit 0: dial status (0 or 1)
    }
}

static inline void _set_amount(const uint8_t amount) {
    // amountの設定
    // ここに実際のハードウェア制御コードを追加
}

static inline void _set_led_pins(const uint8_t state) {
    // LEDの設定
    // ここに実際のハードウェア制御コードを追加
}

static inline void _set_solenoid_pins(const uint8_t state) {
    // DL_GPIO_writePinsVal(DIAL_LOCK_SOL_PORT, DIAL_LOCK_SOL_PIN, state ? DIAL_LOCK_SOL_PIN : 0);
    DL_GPIO_writePinsVal(BLOCK_SOL_PORT, BLOCK_SOL_PIN, state ? BLOCK_SOL_PIN : 0);
}

static inline void _set_segment_led(GPIO_Regs* gpio, uint32_t segment_mask, const uint32_t bit_offset,
                                    const uint8_t amount) {
    // bit lines GFEDCBA
    // 7 ... 0 bits
    uint32_t seg_bits = 0;
    switch (amount) {
        case 0:
            seg_bits = 0b00111111;
            break;
        case 1:
            seg_bits = 0b00000110;
            break;
        case 2:
            seg_bits = 0b01011011;
            break;
        case 3:
            seg_bits = 0b01001111;
            break;
        case 4:
            seg_bits = 0b01100110;
            break;
        case 5:
            seg_bits = 0b01101101;
            break;
        case 6:
            seg_bits = 0b01111101;
            break;
        case 7:
            seg_bits = 0b00000111;
            break;
        case 8:
            seg_bits = 0b01111111;
            break;
        case 9:
            seg_bits = 0b01101111;
            break;
        default:
            break;
    }

    DL_GPIO_writePinsVal(gpio, segment_mask, (seg_bits << bit_offset) & segment_mask);
}

static inline void _set_segment_leds(const uint8_t amount1, const uint8_t amount2) {
    _set_segment_led(SEG1_PORT, SEG1_PINS_MASK, SEG1_BIT_OFFSET, amount1);
    _set_segment_led(SEG2_PORT, SEG2_PINS_MASK, SEG2_BIT_OFFSET, amount2);
}

static inline bool _check_debounce_complete(button_event_t* event, uint32_t debounce_us) {
    if (event->pressed == 1U && event->last_state == 0U && event->debounce_us == 0U) {
        // Rising edge detected, start debounce
        event->debounce_us = debounce_us;
        event->last_state  = 1U;

    } else if (event->pressed == 0U && event->last_state == 1U) {
        // Falling edge detected, no debounce needed
        event->last_state  = 0U;
        event->debounce_us = 0U;
    }

    if (event->debounce_us > 0U) {
        // Busy-wait debounce interval
        delay_cycles(DEBOUNCE_CYCLES_1US);
        event->debounce_us--;

        return false;
    }

    if (event->pressed == 1U && event->last_state == 1U) {
        // Stable pressed state detected
        return true;
    }

    return false;
}

#endif  // AXON_BOARD

void axon_routine_main(void* args) {
#ifdef AXON_BOARD
    systick_t         period        = 0;
    systick_t         notified_time = 0;
    axon_status_t     axon_state    = {0};
    bool              changed       = false;
    uart_s2a_packet_t s2a_packet    = {0};
    uint8_t           notified_inc  = 0;
    systick_t                last_led_update    = 0;  // LED更新用の最終時刻

    // ========== UARTループバックテスト実行 ==========
    // PA10-PA11をショート接続してからテスト実行
    // 注意: UART初期化はSYSCFG_DL_init()内の_msp_peripheral_uart_init()で完了済み
    // soma_uart_init()は呼ばない（SOMA側用の設定であり、AXON_BOARDのUART0と互換性なし）
    #if 0  // ループバックテストを有効化する場合は #if 1 に変更
    {
        bool loopback_result = axon_uart_loopback_test();
        if (!loopback_result) {
            // テスト失敗時は赤LED点灯して停止
            DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);  // RED LED ON
            printf("AXON UART Loopback Test FAILED!\n");
            while (1) {
                __WFI();
            }
        }
        printf("AXON UART Loopback Test PASSED!\n");
        // テスト成功 - 緑LED点灯後、通常動作へ移行
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_1);  // GREEN LED ON
        delay_cycles(32000000);  // 1秒待機
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_1);  // GREEN LED OFF
        delay_cycles(CPUCLK_FREQ / 2);  // 0.5秒待機
        
        // 36バイトフレーム完全検証テスト（CRC・フレーム同期・ISR処理）
        printf("\n--- Starting 36-byte Frame Test ---\n");
        if (!axon_36byte_frame_test()) {
            // テスト失敗 - 赤LED点滅
            printf("36-byte Frame Test FAILED!\n");
            while (1) {y
                DL_GPIO_togglePins(GPIOB, DL_GPIO_PIN_0);  // RED LED blink
                delay_cycles(16000000);  // 0.5秒
            }
        }
        printf("36-byte Frame Test PASSED!\n");
        // テスト成功 - 青LED点灯後、通常動作へ移行
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_2);  // BLUE LED ON
        delay_cycles(32000000);  // 1秒待機
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_2);  // BLUE LED OFF
        delay_cycles(CPUCLK_FREQ / 2);  // 0.5秒待機
    }
    #endif
    
    
    uart_packet_status_t status = init_uart_ports();
    if (status != UART_PACKET_STATUS_SUCCESS) {
        while (1) {
            __WFI();
        }
    }

    // 注意: soma_uart_init()は36バイトフレーム/AES暗号化通信用のテスト関数です。
    // 通常のSOMA-AXON間通信（3バイトパケット、0xFF header）とは互換性がありません。
    // 本番動作では呼び出さず、init_uart_ports()で初期化されたUARTドライバを使用してください。

    // ==========================================
    // RGB色フェード設定
    // ==========================================
    uint8_t selected_color_index = 5;       // 0: Off, 1: Red, 2: Green, 3: Blue, 4: Cyan, 5: Magenta, 6: Yellow, 7: White
    bool is_fast_blink = false;             // 点滅速度 (true: 高速250ms, false: 低速500ms)

    // set_led_rgb_pwm(TIM_LED_PWM_PERIOD_COUNT, TIM_LED_PWM_PERIOD_COUNT / 2, TIM_LED_PWM_PERIOD_COUNT);

    // 1. 起動時にSOMAに状態通知を行うために、IRQをたてる
    // 2. UARTでのコマンド受信待ちを行う
    // 3. ソレノイドONコマンドを受信したら、ソレノイドをONにし、ハンドル回転検出待ちへ
    // 4. ハンドル回転を検出したら、ソレノイドをOFFにし、状態通知を行い、再びUARTでのコマンド受信待ちへ
    // 5. 一定時間コマンドを受信しなかった場合、IRQをたて、状態通知を行う
    _change_status(&axon_state, STATE_NOTIFY);
    DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, UART_IRQ_OUT_PIN);

    _set_segment_leds(axon_state.left_amount, axon_state.right_amount);

    while (1) {
        // check dial rotation
        if (axon_state.status == STATE_SOL_ON) {
            bool dial_rotated = _check_debounce_complete(&g_rotary_event, SWITCH_DEBOUNCE_US);
            if (dial_rotated) {
                axon_state.sol_state   = 0;
                axon_state.dial_detect = 1;
                axon_state.dial_state  = 1;
                _change_status(&axon_state, STATE_DIAL_DETECT);
                _set_solenoid_pins(axon_state.sol_state);
                changed = true;
            }
        } else {
            // 0.1ms wait
            delay_cycles(CPUCLK_FREQ / 10000);
        }

        // select segment LEDs
        if (g_button_1_event.pressed) {
            if (g_button_1_event.last_press_time_ms + BUTTON_INTERVAL_MS < get_systick_count_ms()) {
                if (axon_state.left_amount < 9) {
                    axon_state.left_amount++;
                } else {
                    axon_state.left_amount = 0;
                }
                g_button_1_event.pressed            = 0;
                g_button_1_event.last_press_time_ms = get_systick_count_ms();
            }
        }

        if (g_button_2_event.pressed) {
            if (g_button_2_event.last_press_time_ms + BUTTON_INTERVAL_MS < get_systick_count_ms()) {
                if (axon_state.right_amount < 9) {
                    axon_state.right_amount++;
                } else {
                    axon_state.right_amount = 0;
                }
                g_button_2_event.pressed            = 0;
                g_button_2_event.last_press_time_ms = get_systick_count_ms();
            }
        }
        // // ESCROW SW
        // if (g_escrow_event.pressed) {
        //     // エスクロ検知時の処理
        //     if (g_escrow_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // エスクロ検知処理
        //         axon_state.left_amount = 1;  // デバッグ用
        //         axon_state.right_amount = 1; // デバッグ用
        //     }
        //     g_escrow_event.pressed            = 0;
        //     g_escrow_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_coindet_event.pressed) {
        //     // 現金検知時の処理
        //     if (g_coindet_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // 現金検知処理
        //         axon_state.left_amount = 3;  // デバッグ用
        //         axon_state.right_amount = 3; // デバッグ用
        //     }
        //     g_coindet_event.pressed            = 0;
        //     g_coindet_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_connector_event.pressed) {
        //     // コネクタ検知時の処理
        //     if (g_connector_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // コネクタ検知処理
        //         axon_state.left_amount = 5;  // デバッグ用
        //         axon_state.right_amount = 5; // デバッグ用
        //     }
        //     g_connector_event.pressed            = 0;
        //     g_connector_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_soldout_event.pressed) {
        //     // 売り切れ検知時の処理
        //     if (g_soldout_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // 売り切れ検知処理
        //         axon_state.left_amount = 7;  // デバッグ用
        //         axon_state.right_amount = 7; // デバッグ用
        //     }
        //     g_soldout_event.pressed            = 0;
        //     g_soldout_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_door_event.pressed) {
        //     // ドア開閉検知時の処理
        //     if (g_door_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // ドア開閉検知処理
        //         axon_state.left_amount = 9;  // デバッグ用
        //         axon_state.right_amount = 9; // デバッグ用
        //     }
        //     g_door_event.pressed            = 0;
        //     g_door_event.last_press_time_ms = get_systick_count_ms();
        // }

        _set_segment_leds(axon_state.left_amount, axon_state.right_amount);

        // ==========================================
        // SOMA UARTテスト: フレームチェック
        // ==========================================
        // 注意: soma_check_frame()はSOMA側の36バイトAES暗号化通信用
        // AXON_BOARDでは使用しない
        // soma_check_frame();

        // ==========================================
        // UART受信処理
        // ==========================================
        status = receive_uart_s2a_packet(&s2a_packet);
        
        if (status == UART_PACKET_STATUS_SUCCESS) {
            // 受信成功時は1msウェイト
            delay_cycles(CPUCLK_FREQ / 1000U);

            // 受信コマンドに応じた処理を実行
            switch (s2a_packet.command) {
                // ------------------------------------------
                // 状態取得要求
                // ------------------------------------------
                case UART_CMD_GET_STATUS_REQ: {
                    // 応答パケットを構築
                    s2a_packet.header  = UART_S2A_HEADER;
                    s2a_packet.command = UART_CMD_GET_STATUS_RESP;
                    s2a_packet.data    = _generate_uart_terminal_status_data(
                        0,                          // amount (未使用)
                        0,                          // led_state (未使用)
                        axon_state.sol_state,       // ソレノイド状態
                        axon_state.dial_detect      // ハンドル回転検出
                    );

                    // 応答を送信
                    status = send_uart_s2a_packet(&s2a_packet);
                    if (status == UART_PACKET_STATUS_SUCCESS) {
                        _change_status(&axon_state, STATE_NORMAL);
                        changed = false;
                        
                        // IRQ信号を下げる
                        DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);
                    }
                    break;
                }

                // ------------------------------------------
                // 状態設定要求
                // ------------------------------------------
                case UART_CMD_SET_STATUS_REQ: {
                    // 受信データをパース
                    _parse_uart_terminal_status_data(
                        s2a_packet.data,
                        &axon_state.amount,
                        &axon_state.led_state,
                        &axon_state.sol_state,
                        &axon_state.dial_state
                    );

                    // ACK応答パケットを構築
                    s2a_packet.header  = UART_S2A_HEADER;
                    s2a_packet.command = UART_CMD_SET_STATUS_RESP;
                    s2a_packet.data    = _generate_uart_terminal_status_data(
                        axon_state.amount,
                        axon_state.led_state,
                        axon_state.sol_state,
                        axon_state.dial_state
                    );

                    // ACK応答を送信
                    status = send_uart_s2a_packet(&s2a_packet);
                    if (status == UART_PACKET_STATUS_SUCCESS) {
                        // ハードウェア制御を更新
                        _set_amount(axon_state.amount);
                        _set_led_pins(axon_state.led_state);
                        _set_solenoid_pins(axon_state.sol_state);

                        // 状態遷移の処理
                        if (axon_state.sol_state) {
                            // ソレノイドON時: ハンドル回転検出待ちへ
                            _change_status(&axon_state, STATE_SOL_ON);
                            axon_state.dial_detect = 0;
                        } else if (axon_state.status != STATE_NOTIFY) {
                            // ソレノイドOFF時: 通常状態へ
                            _change_status(&axon_state, STATE_NORMAL);
                        }
                        
                        changed = true;
                    }
                    break;
                }

                // ------------------------------------------
                // 不明なコマンド
                // ------------------------------------------
                default:
                    // 無視する
                    break;
            }
        } else if (status == UART_PACKET_STATUS_TIMEOUT) {
            // タイムアウト: データなし(正常)
        } else if (status == UART_PACKET_STATUS_NO_DATA) {
            // データなし(正常)
        } else {
            // その他のエラー: 受信エラー
        }

        period = get_systick_count_ms();

        if (period - notified_time > POLLING_INTERVAL_MS || changed) {
            notified_time = period;
            changed = false;

            // uart irq enable
            _change_status(&axon_state, STATE_NOTIFY);
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, UART_IRQ_OUT_PIN);
        }

        // ==========================================
        // 1ms周期
        // ==========================================
        if ((period - last_led_update) >= 5) {      // 5msec周期
            // Full Color LED 点灯処理
            last_led_update = period;
            rgb_led_fade_update(selected_color_index, is_fast_blink, period);
        }
    }
#endif  // AXON_BOARD
}
