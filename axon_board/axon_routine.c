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
#include "protocol/src/s2a_packet.h"  // CHKIRQ/SETAXON等のハンドラ
#include "axon_status.h"  // AXON状態管理構造体

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

// デバッグ: 起動5秒後にCOIN_DET疑似入力
static uint8_t g_coin_test_triggered = 0;
button_event_t g_connector_event;   // コネクタ検知(予備GPIO)
button_event_t g_soldout_event;     // 売り切れ検知SW
button_event_t g_door_event;        // ドア開閉検知

button_event_t g_button_1_event;
button_event_t g_button_2_event;

// SOMA-AXON共有状態（protocol/src/s2a_packet.cから参照）
volatile axon_status_shared_t g_axon_status_shared = {0};

// 7セグLED表示値（protocol/src/s2a_packet.cから参照）
uint8_t g_left_amount = 0;
uint8_t g_right_amount = 0;

// AXON状態管理（protocol/src/s2a_packet.cから参照）
axon_status_t g_axon_state = {0};

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

// 外部から呼び出せる7セグLED更新関数（s2a_packet.cから使用）
void update_segment_leds(const uint8_t amount1, const uint8_t amount2) {
    _set_segment_leds(amount1, amount2);
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
            while (1) {
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
    
    // UART0設定情報とPort1/Port2構成を表示
    printf("\r\n");
    printf("[UART0 Configuration]\r\n");
    printf("Base Address: 0x%08X (UART0)\r\n", (unsigned int)S2A_UART_INST);
    printf("Baudrate    : 115200 bps\r\n");
    printf("Format      : 8N1 (8bit, No parity, 1 stop)\r\n");
    printf("SOMA Comm   : PA10(TX/PINCM21), PA11(RX/PINCM22)\r\n");
    printf("\r\n");
    printf("[Port Configuration]\r\n");
    printf("Port1 (AXON): Uses UART0 for SOMA communication\r\n");
    printf("              TX via PA10, RX via PA11\r\n");
    printf("Port2 (AXON): Uses UART0 for SOMA communication\r\n");
    printf("              TX via PA10, RX via PA11\r\n");
    printf("Note: Both ports share same UART0 hardware\r\n");
    printf("      Multiplexed via MUX control signals\r\n");
    printf("\r\n");
    printf("[ISR Configuration]\r\n");
    printf("DL_UART_IIDX_RX = 0x%X (expected for RX interrupt)\r\n", DL_UART_IIDX_RX);
    printf("========================================\r\n");
    printf("\r\n");
    
    // ========== UART初期化について ==========
    // AXON_BOARD: UART0は既にSYSCFG_DL_init()内の_msp_peripheral_uart_init()で初期化済み
    //             ISRベース（UART0_IRQHandler）で36バイトフレーム受信を処理
    //             init_uart_ports()は呼ばない（SOMA_BOARD専用、DMA方式と競合）
    // 
    // SOMA_BOARD: init_uart_ports()でUART1/UART2をDMA+コールバック方式で初期化
    //             3バイトパケット通信に使用
    #ifdef SOMA_BOARD
    uart_packet_status_t status = init_uart_ports();
    if (status != UART_PACKET_STATUS_SUCCESS) {
        while (1) {
            __WFI();
        }
    }
    #else
    // AXON_BOARD: 起動時にUART0 RX FIFOをクリア（Port切り替え時の残留データ対策）
    NVIC_DisableIRQ(UART0_INT_IRQn);  // クリア中は割り込み無効
    
    // フェーズ1: 既存FIFOデータをクリア
    uint32_t clear_count = 0;
    while (!DL_UART_isRXFIFOEmpty(S2A_UART_INST) && clear_count < 100) {
        DL_UART_receiveData(S2A_UART_INST);
        clear_count++;
    }
    
    // フェーズ2: 500ms待機して遅延到着データも受信
    delay_cycles(CPUCLK_FREQ / 2);  // 500ms待機
    while (!DL_UART_isRXFIFOEmpty(S2A_UART_INST) && clear_count < 200) {
        DL_UART_receiveData(S2A_UART_INST);
        clear_count++;
    }
    
    // 受信状態変数もリセット
    extern volatile uint8_t rx_index;
    extern volatile uint32_t debug_rx_count;
    extern volatile uint32_t debug_complete_count;
    rx_index = 0;
    debug_rx_count = 0;
    debug_complete_count = 0;
    
    NVIC_EnableIRQ(UART0_INT_IRQn);  // クリア完了後、割り込み有効化
    printf("UART0 RX FIFO cleared (%lu bytes, with 500ms delay)\r\n", (unsigned long)clear_count);
    #endif

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
    _change_status(&g_axon_state, STATE_NOTIFY);
    DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, UART_IRQ_OUT_PIN);

    _set_segment_leds(g_axon_state.left_amount, g_axon_state.right_amount);

    while (1) {
        // デバッグ: 5秒毎にCOIN_DET疑似入力
        {
            static systick_t last_coin_test_time = 0;
            systick_t current_time = get_systick_count_ms();
            if (current_time - last_coin_test_time >= 5000) {
                last_coin_test_time = current_time;
                g_coindet_event.pressed = 1;
                g_coindet_event.phase_time = current_time;
                printf("[DEBUG] Simulated COIN_DET input at %lu ms\r\n", (unsigned long)current_time);
            }
        }
        
        // check dial rotation
        if (g_axon_state.status == STATE_SOL_ON) {
            bool dial_rotated = _check_debounce_complete(&g_rotary_event, SWITCH_DEBOUNCE_US);
            if (dial_rotated) {
                g_axon_state.sol_state   = 0;
                g_axon_state.dial_detect = 1;
                g_axon_state.dial_state  = 1;
                _change_status(&g_axon_state, STATE_DIAL_DETECT);
                _set_solenoid_pins(g_axon_state.sol_state);
                changed = true;
            }
        } else {
            // 0.1ms wait
            delay_cycles(CPUCLK_FREQ / 10000);
        }

        // select segment LEDs
        if (g_button_1_event.pressed) {
            if (g_button_1_event.last_press_time_ms + BUTTON_INTERVAL_MS < get_systick_count_ms()) {
                if (g_axon_state.left_amount < 9) {
                    g_axon_state.left_amount++;
                } else {
                    g_axon_state.left_amount = 0;
                }
                g_left_amount = g_axon_state.left_amount;  // グローバル変数も同期
                g_button_1_event.pressed            = 0;
                g_button_1_event.last_press_time_ms = get_systick_count_ms();
            }
        }

        if (g_button_2_event.pressed) {
            if (g_button_2_event.last_press_time_ms + BUTTON_INTERVAL_MS < get_systick_count_ms()) {
                if (g_axon_state.right_amount < 9) {
                    g_axon_state.right_amount++;
                } else {
                    g_axon_state.right_amount = 0;
                }
                g_right_amount = g_axon_state.right_amount;  // グローバル変数も同期
                g_button_2_event.pressed            = 0;
                g_button_2_event.last_press_time_ms = get_systick_count_ms();
            }
        }
        // // ESCROW SW
        // if (g_escrow_event.pressed) {
        //     // エスクロ検知時の処理
        //     if (g_escrow_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // エスクロ検知処理
        //         g_axon_state.left_amount = 1;  // デバッグ用
        //         g_axon_state.right_amount = 1; // デバッグ用
        //     }
        //     g_escrow_event.pressed            = 0;
        //     g_escrow_event.last_press_time_ms = get_systick_count_ms();
        // }

        if (g_coindet_event.pressed) {
            // 現金検知時の処理: STATUS bit3をLatch、IRQ信号をSOMAに送信
            axon_status_latch_coin();  // STATUS bit3=0 (現金投入中)に設定
            
            // IRQ信号をLow(アクティブ)に設定してSOMAに通知
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);
            
            g_coindet_event.pressed = 0;
        }

        // if (g_connector_event.pressed) {
        //     // コネクタ検知時の処理
        //     if (g_connector_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // コネクタ検知処理
        //         g_axon_state.left_amount = 5;  // デバッグ用
        //         g_axon_state.right_amount = 5; // デバッグ用
        //     }
        //     g_connector_event.pressed            = 0;
        //     g_connector_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_soldout_event.pressed) {
        //     // 売り切れ検知時の処理
        //     if (g_soldout_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // 売り切れ検知処理
        //         g_axon_state.left_amount = 7;  // デバッグ用
        //         g_axon_state.right_amount = 7; // デバッグ用
        //     }
        //     g_soldout_event.pressed            = 0;
        //     g_soldout_event.last_press_time_ms = get_systick_count_ms();
        // }

        // if (g_door_event.pressed) {
        //     // ドア開閉検知時の処理
        //     if (g_door_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // ドア開閉検知処理
        //         g_axon_state.left_amount = 9;  // デバッグ用
        //         g_axon_state.right_amount = 9; // デバッグ用
        //     }
        //     g_door_event.pressed            = 0;
        //     g_door_event.last_press_time_ms = get_systick_count_ms();
        // }

        // for debug CN3 ROT_DET(11pin) TEST
        // if (g_rotary_event.pressed ){
        //     // ダイヤル回転検知
        //     if (g_rotary_event.last_press_time_ms + 10 < get_systick_count_ms()) {
        //         // ダイヤル回転検知
        //         g_axon_state.left_amount = 2;
        //         g_axon_state.right_amount = 2;
        //     }
        //     g_rotary_event.pressed = 0;
        //     g_rotary_event.last_press_time_ms = get_systick_count_ms();
        // }


        // 7セグLED更新: グローバル変数とローカル変数を同期
        // SETAXONで更新された値を優先的に反映
        if (g_left_amount != g_axon_state.left_amount) {
            g_axon_state.left_amount = g_left_amount;
        }
        if (g_right_amount != g_axon_state.right_amount) {
            g_axon_state.right_amount = g_right_amount;
        }

        // 7seg LED 更新
        _set_segment_leds(g_axon_state.left_amount, g_axon_state.right_amount);

        // ==========================================
        // SOMA UARTテスト: フレームチェック
        // ==========================================
        // 注意: soma_check_frame()はSOMA側の36バイトAES暗号化通信用
        // AXON_BOARDでは使用しない
        // soma_check_frame();

        // ==========================================
        // 36バイトフレーム受信処理（CHKIRQ/SETAXON/NOP等）
        // ==========================================
        // ISRで受信した36バイトフレームを処理
        extern volatile uint8_t rx_complete_ready;
        extern uint8_t rx_complete_frame[36];
        
        // デバッグ変数の外部宣言（soma_uart_test.h で定義済み）
        extern volatile uint32_t debug_rx_count;
        extern volatile uint32_t debug_complete_count;
        extern volatile uint32_t debug_sync_reset_count;
        extern volatile uint8_t debug_last_byte;
        
        // デバッグ出力 - テスト中は無効化（リアルタイム性優先）
        #if 0  // デバッグ出力を完全無効化
        extern volatile uint8_t debug_byte1;
        extern volatile uint32_t debug_byte1_ng_count;
        extern volatile uint32_t debug_isr_call_count;  // ISR呼び出し回数
        extern volatile uint32_t debug_iidx_value;      // 最後のiidx値
        extern volatile uint32_t debug_fifo_empty_count; // FIFO空判定回数
        extern volatile uint32_t debug_uart_stat_value;  // UART STAT値
        extern volatile uint32_t debug_rxdata_raw_value; // RXDATA生値
        static systick_t last_debug_time = 0;
        static uint32_t debug_output_counter = 0;
        systick_t current_time = get_systick_count_ms();
        if ((current_time - last_debug_time) >= 200) {  // 200msごとに変更（負荷軽減）
            debug_output_counter++;
            extern volatile uint32_t debug_overrun_count;
            extern volatile uint32_t debug_framing_error_count;
            printf("[DBG] isr=%lu, iidx=0x%lX, rx=%lu, cmp=%lu, rdy=%d, last=0x%02X, b1=0x%02X, fifo_e=%lu\n",
                   (unsigned long)debug_isr_call_count,
                   (unsigned long)debug_iidx_value,
                   (unsigned long)debug_rx_count,
                   (unsigned long)debug_complete_count,
                   rx_complete_ready,
                   debug_last_byte,
                   debug_byte1,
                   (unsigned long)debug_fifo_empty_count);
            printf("[DBG] STAT=0x%08lX, RXDATA=0x%08lX, sync_rst=%lu, OVERRUN=%lu, FRM_ERR=%lu\n",
                   (unsigned long)debug_uart_stat_value,
                   (unsigned long)debug_rxdata_raw_value,
                   (unsigned long)debug_sync_reset_count,
                   (unsigned long)debug_overrun_count,
                   (unsigned long)debug_framing_error_count);
            NVIC_EnableIRQ(UART0_INT_IRQn);  // printf()完了後、割り込み再有効化
            
            // 5秒ごとにUART STATレジスタを表示（より詳細な診断）
            if (debug_output_counter % 50 == 0) {
                NVIC_DisableIRQ(UART0_INT_IRQn);  // printf()前に割り込み無効化
                uint32_t uart_stat = S2A_UART_INST->STAT;
                printf("[UART_STAT] 0x%08lX (RXOE=%d, RXFE=%d, BUSY=%d, IDLE=%d)\n",
                       (unsigned long)uart_stat,
                       (uart_stat & (1 << 11)) ? 1 : 0,  // bit11: RX Overrun Error
                       (uart_stat & (1 << 10)) ? 1 : 0,  // bit10: RX Framing Error
                       (uart_stat & (1 << 7)) ? 1 : 0,   // bit7: UART Busy
                       (uart_stat & (1 << 1)) ? 1 : 0);  // bit1: UART Idle
                NVIC_EnableIRQ(UART0_INT_IRQn);  // printf()完了後、割り込み再有効化
            }
            last_debug_time = current_time;
        }
        #endif  // デバッグ出力無効化終了
        
        if (rx_complete_ready) {
            // フレームを処理（protocol/src/s2a_packet.cの各ハンドラを呼び出し）
            uint8_t header = rx_complete_frame[0];
            uint8_t length = rx_complete_frame[1];
            
            // Headerでコマンド種別を判定
            bool handled = false;
            
            if (header == 0x14 && length == 0x20) {
                // 36バイト標準フレーム（CHKIRQ/SETAXON/NOP/AFWUP）
                uint8_t cmd_id = rx_complete_frame[2];  // Data部の最初のバイトがコマンドID
                
                switch (cmd_id) {
                    case 0x49:  // CHKIRQ - ポート確認シーケンス
                        // デバッグ出力無効化（リアルタイム性優先）
                        #if 0
                        NVIC_DisableIRQ(UART0_INT_IRQn);
                        printf("[CHKIRQ] Received! Header=0x%02X, Len=0x%02X, CmdID=0x%02X\n",
                               header, length, cmd_id);
                        NVIC_EnableIRQ(UART0_INT_IRQn);
                        #endif
                        handled = axon_handle_chkirq(rx_complete_frame);
                        #if 0
                        if (handled) {
                            NVIC_DisableIRQ(UART0_INT_IRQn);
                            printf("[CHKIRQ] Handler returned success\n");
                            NVIC_EnableIRQ(UART0_INT_IRQn);
                        } else {
                            NVIC_DisableIRQ(UART0_INT_IRQn);
                            printf("[CHKIRQ] Handler returned failure\n");
                            NVIC_EnableIRQ(UART0_INT_IRQn);
                        }
                        #endif
                        break;
                        
                    case 0x4A:  // SETAXON - AXON設定書き込み
                        handled = axon_handle_setaxon(rx_complete_frame);
                        if (handled) {
                            // 設定反映成功 - 状態更新
                            _change_status(&g_axon_state, STATE_NORMAL);
                            changed = true;
                        }
                        break;
                        
                    case 0x50:  // NOP - 無操作コマンド
                        handled = axon_handle_nop(rx_complete_frame);
                        break;
                        
                    case 0x4B:  // AFWUP - FW更新要求（仕様書Table 4-1）
                        handled = axon_handle_afwup(rx_complete_frame);
                        break;
                        
                    case 0x7F:  // AXONRBT - 再起動要求（仕様書Table 4-1）
                        handled = axon_handle_axonrbt(rx_complete_frame);
                        break;
                        
                    default:
                        // 未知のコマンド - 無視
                        break;
                }
            }
            
            // 処理完了後、フラグをクリア
            rx_complete_ready = 0;
        }
        
        // ==========================================
        // 可変長フレーム受信処理（SETOKEY/CODEPKT/ERRCHK）
        // ==========================================
        extern volatile uint8_t rx_variable_ready;
        extern volatile uint8_t rx_variable_length;
        extern uint8_t rx_variable_frame[40];
        
        if (rx_variable_ready) {
            uint8_t header = rx_variable_frame[0];
            uint8_t length = rx_variable_frame[1];
            bool handled = false;
            
            if (header == 0x15 && length == 0x10 && rx_variable_length == 18) {
                // 18バイトフレーム - SETOKEY（運用鍵設定、仕様書Table 4-20準拠）
                handled = axon_handle_setokey(rx_variable_frame);
                if (handled) {
                    // 運用鍵設定成功
                }
            } else if (header == 0xA5 && length == 0x24 && rx_variable_length == 40) {
                // 40バイトフレーム - CODEPKT（FWコードパケット）
                handled = axon_handle_codepkt(rx_variable_frame);
                if (handled) {
                    // FWコードパケット受信成功
                }
            } else if (header == 0xC4 && length == 0x02 && rx_variable_length == 6) {
                // 6バイトフレーム - ERRCHK（FWエラーチェック）
                handled = axon_handle_errchk(rx_variable_frame);
                if (handled) {
                    // エラーチェック完了
                }
            }
            
            // 処理完了後、フラグをクリア
            rx_variable_ready = 0;
            rx_variable_length = 0;
        }



        // ==========================================
        // 旧POC用UART受信処理（3バイトパケット）
        // 注意: SOMA_BOARD専用（AXON_BOARDでは使用しない）
        // ==========================================
#ifdef SOMA_BOARD
        uart_packet_status_t status = receive_uart_s2a_packet(&s2a_packet);
        
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
                        g_axon_state.sol_state,       // ソレノイド状態
                        g_axon_state.dial_detect      // ハンドル回転検出
                    );

                    // 応答を送信
                    status = send_uart_s2a_packet(&s2a_packet);
                    if (status == UART_PACKET_STATUS_SUCCESS) {
                        _change_status(&g_axon_state, STATE_NORMAL);
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
                        &g_axon_state.amount,
                        &g_axon_state.led_state,
                        &g_axon_state.sol_state,
                        &g_axon_state.dial_state
                    );

                    // ACK応答パケットを構築
                    s2a_packet.header  = UART_S2A_HEADER;
                    s2a_packet.command = UART_CMD_SET_STATUS_RESP;
                    s2a_packet.data    = _generate_uart_terminal_status_data(
                        g_axon_state.amount,
                        g_axon_state.led_state,
                        g_axon_state.sol_state,
                        g_axon_state.dial_state
                    );

                    // ACK応答を送信
                    status = send_uart_s2a_packet(&s2a_packet);
                    if (status == UART_PACKET_STATUS_SUCCESS) {
                        // ハードウェア制御を更新
                        _set_amount(g_axon_state.amount);
                        _set_led_pins(g_axon_state.led_state);
                        _set_solenoid_pins(g_axon_state.sol_state);

                        // 状態遷移の処理
                        if (g_axon_state.sol_state) {
                            // ソレノイドON時: ハンドル回転検出待ちへ
                            _change_status(&g_axon_state, STATE_SOL_ON);
                            g_axon_state.dial_detect = 0;
                        } else if (g_axon_state.status != STATE_NOTIFY) {
                            // ソレノイドOFF時: 通常状態へ
                            _change_status(&g_axon_state, STATE_NORMAL);
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
            _change_status(&g_axon_state, STATE_NOTIFY);
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, UART_IRQ_OUT_PIN);
        }
#endif  // SOMA_BOARD

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
