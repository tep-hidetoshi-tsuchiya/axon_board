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
#include "debug_log.h"  // ノンブロッキングログ

// デバッグログバッファ（グローバル変数）
debug_log_t g_debug_log = {0};

// 重複検出後の自動リトライフラグ (s2a_packet.cで定義)
extern volatile uint8_t g_retry_irq_request;

#define POLLING_INTERVAL_MS (500U)
#define SWITCH_DEBOUNCE_US  (50U)
#define DEBOUNCE_CYCLES_1US (CPUCLK_FREQ / 1000000U)
#define BUTTON_INTERVAL_MS  (250U)
#define BUTTON_LONG_PRESS_MS (2000U)  // 長押し検出時間: 3秒（テストしやすい時間、仕様は5秒）
#define BUTTON_BOTH_LONG_PRESS_MS (2000U)  // 両ボタン同時長押し: 2秒（※仕様書には記載なし、誤操作防止用）
#define LED_BLINK_INTERVAL_MS (500U)  // LED点滅周期: 500ms
#define AUTO_RETRY_INTERVAL_MS (50U)  // 自動リトライ間隔: 50ms（重複検出後の最速再試行）

// ボタン処理用の構造体
typedef struct {
    systick_t *press_start;           // 押下開始時刻へのポインタ
    button_event_t *event;             // ボタンイベントへのポインタ
    volatile uint8_t *pending_amount;  // pending値へのポインタ
    volatile uint8_t *pending_updated; // pending更新フラグへのポインタ
    uint8_t *current_amount;           // 現在値へのポインタ
    systick_t *release_candidate_time; // リリース候補時刻
    uint8_t *release_candidate_flag;   // リリース候補フラグ
    const char *name;                  // デバッグ用ボタン名
} button_context_t;

typedef enum {
    STATE_NONE,
    STATE_NORMAL,
    STATE_SOL_ON,
    STATE_DIAL_DETECT,
    STATE_NOTIFY,
} terminal_status_t;

typedef struct {
    terminal_status_t status;
    uint8_t           sol_state;           // ソレノイド状態（SOMA_BOARD用）
    uint8_t           dial_detect;         // ダイヤル検出（SOMA_BOARD用）
    uint8_t           left_amount;         // 0-9 (7セグLED左側)
    uint8_t           right_amount;        // 0-9 (7セグLED右側)
    uint8_t           led_blink_mode;      // 0: 点灯, 1: 点滅(変更モード中)
    uint8_t           led_blink_state;     // 点滅時の現在状態(0: 消灯, 1: 点灯)
    systick_t         button1_press_start; // ボタン1長押し開始時刻
    systick_t         button2_press_start; // ボタン2長押し開始時刻
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

// SOMA-AXON共有状態（protocol/src/s2a_packet.cから参照）
volatile axon_status_shared_t g_axon_status_shared = {0};

// 7セグLED表示値（protocol/src/s2a_packet.cから参照）
// ★CRITICAL: volatile必須（ISR/メインループ/UARTハンドラー間で共有）
volatile uint8_t g_left_amount = 0;
volatile uint8_t g_right_amount = 0;

// ATIRQ送信用の一時変数（ボタン押下時の+1値を保持、SETAXON受信まで表示は更新しない）
// ★CRITICAL: volatile必須（メインループでセット、UARTハンドラーでクリア）
volatile uint8_t g_pending_left_amount = 0;
volatile uint8_t g_pending_right_amount = 0;
volatile uint8_t g_pending_left_updated = 0;   // 1=pending値が更新済み（ATIRQ送信すべき）
volatile uint8_t g_pending_right_updated = 0;  // 1=pending値が更新済み（ATIRQ送信すべき）
volatile uint8_t g_retry_pending = 0;          // 1=重複検出リトライ中（新規ボタン押下を抑制）

// ダイヤル回転数カウンタ（ATIRQ STATUS bit[15:8]、0x00→0xFF循環）
volatile uint8_t g_dial_rotation_count = 0;

// IRQ_N信号制御フラグ（メインループで50msパルス処理）
static volatile uint8_t g_irq_pulse_pending = 0;  // 1=IRQ_N Lowセット済み、50ms待機→High必要
static volatile systick_t g_irq_pulse_start_time = 0;  // IRQ_N Low開始時刻

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

#endif  // SOMA_BOARD

#ifdef AXON_BOARD

static inline void _set_solenoid_pins(const uint8_t state) {
    // DL_GPIO_writePinsVal(COIN_SOL_PORT, COIN_SOL_PIN, state ? COIN_SOL_PIN : 0);
    DL_GPIO_writePinsVal(BLOCK_SOL_PORT, BLOCK_SOL_PIN, state ? BLOCK_SOL_PIN : 0);
}

static inline void _set_segment_led(GPIO_Regs* gpio, uint32_t segment_mask, const uint32_t bit_offset,
                                    const uint8_t amount) {
    // bit lines GFEDCBA
    // 7 ... 0 bits
    uint32_t seg_bits = 0;
    
    // 0xFF は消灯用の特殊値
    if (amount == 0xFF) {
        seg_bits = 0b00000000;  // 全セグメント消灯
    } else {
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
                // 範囲外の値（10以上）は0を表示（0-9の範囲で循環）
                seg_bits = 0b00111111;  // 0の表示パターン
                break;
        }
    }

    DL_GPIO_writePinsVal(gpio, segment_mask, (seg_bits << bit_offset) & segment_mask);
}

static inline void _set_segment_leds(const uint8_t amount1, const uint8_t amount2) {
    _set_segment_led(SEG1_PORT, SEG1_PINS_MASK, SEG1_BIT_OFFSET, amount1);
    _set_segment_led(SEG2_PORT, SEG2_PINS_MASK, SEG2_BIT_OFFSET, amount2);
}

// 外部から呼び出せる7セグLED更新関数（s2a_packet.cから使用）
// 注: 点滅モード中は現在値のみ更新し、表示はメインループの点滅処理に任せる
void update_segment_leds(const uint8_t amount1, const uint8_t amount2) {
    // グローバル変数を更新（s2a_packet.cで参照される）
    g_left_amount = amount1;
    g_right_amount = amount2;
    
    // 内部状態も更新
    g_axon_state.left_amount = amount1;
    g_axon_state.right_amount = amount2;
    
    // 通常モード（非点滅）の場合のみ即座に表示更新
    if (!g_axon_state.led_blink_mode) {
        _set_segment_leds(amount1, amount2);
    }
    // 点滅モード中は表示更新しない（メインループの点滅処理が制御）
}

// 外部から呼び出せる点滅モードクリア関数（s2a_packet.cから使用）
void axon_clear_blink_mode(void) {
    g_axon_state.led_blink_mode = 0;   // 点滅モード終了
    g_axon_state.led_blink_state = 1;  // 常時点灯状態
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

// ==========================================
// ボタン処理ヘルパー関数
// ==========================================

/**
 * @brief ボタン押下時の処理（長押し開始時刻記録）
 * @param ctx ボタンコンテキスト
 * @param both_buttons_pressed 両ボタン同時押しフラグ
 * @param other_button_pressed 他方のボタン押下状態
 */
static inline void handle_button_press(button_context_t *ctx, uint8_t both_buttons_pressed, uint8_t other_button_pressed) {
    *ctx->release_candidate_flag = 0;  // 押下中はリリース候補をキャンセル
    systick_t current_time = get_systick_count_ms();
    
    // リリース待ち状態
    if (*ctx->press_start == 0xFFFFFFFF) {
        return;
    }
    
    // 長押し開始時刻を記録（両ボタン同時押下中でない時）
    if (*ctx->press_start == 0 && !both_buttons_pressed) {
        *ctx->press_start = current_time;
        printf("[%s] Press started at %lu ms, waiting for long press (%u ms)\n", 
               ctx->name, (unsigned long)current_time, BUTTON_LONG_PRESS_MS);
        return;
    }
    
    // 長押し待機中の処理（両ボタン同時押し時はスキップ）
    // 注: both_buttons_pressedフラグはメインループで管理されているため、ここでは個別の長押し検出のみを行う
    if (!both_buttons_pressed && *ctx->press_start != 0) {
        static systick_t last_debug_time[2] = {0, 0};
        int idx = (ctx->name[6] == '2') ? 1 : 0;  // "BUTTON1" or "BUTTON2"
        
        uint32_t elapsed = current_time - *ctx->press_start;
        
        #ifdef DEBUG_BUTTON_VERBOSE
        // デバッグ出力（1秒ごと）
        if (elapsed > 0 && (elapsed - (elapsed % 1000)) > last_debug_time[idx]) {
            printf("[%s] Long press progress: %lu ms / %u ms (blink_mode=%d, other_btn=%d)\n", 
                   ctx->name, (unsigned long)elapsed, BUTTON_LONG_PRESS_MS, 
                   g_axon_state.led_blink_mode, other_button_pressed);
            last_debug_time[idx] = elapsed - (elapsed % 1000);
        }
        #endif
        
        // 長押し判定（3秒）→ 点滅モードトグル（SOMA-TG仕様書 7.3章準拠）
        if (elapsed >= BUTTON_LONG_PRESS_MS) {
            if (!g_axon_state.led_blink_mode) {
                // 通常モード → 点滅モード開始
                g_axon_state.led_blink_mode = 1;
                printf("[BUTTON] Long press detected on %s, LED blink mode started\n", ctx->name);
            } else {
                // 点滅モード → 通常モード（確定・完了）
                g_axon_state.led_blink_mode = 0;
                g_axon_state.led_blink_state = 1;  // LED点灯に戻す
                printf("[BUTTON] Long press detected on %s, LED blink mode stopped (confirmed)\n", ctx->name);
            }
            *ctx->press_start = 0xFFFFFFFF;  // リリース待ち状態
            ctx->event->last_press_time_ms = current_time;
            #ifdef DEBUG_BUTTON_VERBOSE
            last_debug_time[idx] = 0;
            #endif
        }
    }
}

/**
 * @brief ボタンリリース時の処理（短押し/長押し判定）
 * @param ctx ボタンコンテキスト
 */
static inline void handle_button_release(button_context_t *ctx) {
    systick_t current_time = get_systick_count_ms();
    
    // 通常リリース処理（press_start記録中）
    if (*ctx->press_start != 0 && *ctx->press_start != 0xFFFFFFFF) {
        if (!*ctx->release_candidate_flag) {
            *ctx->release_candidate_time = current_time;
            *ctx->release_candidate_flag = 1;
        } else if ((current_time - *ctx->release_candidate_time) >= 50) {
            uint32_t elapsed = *ctx->release_candidate_time - *ctx->press_start;
            
            // チャタリング判定
            if (elapsed < 100) {
                printf("[%s] Ignored chattering: %lu ms\n", ctx->name, (unsigned long)elapsed);
                *ctx->release_candidate_flag = 0;
                return;
            }
            
            // 短押し判定（3秒未満でリリース）
            if (elapsed < BUTTON_LONG_PRESS_MS) {
                // 点滅モード中のみ値を変更可能（SOMA-TG仕様書 7.3章準拠）
                if (g_axon_state.led_blink_mode && !g_retry_pending) {
                    if ((ctx->event->last_press_time_ms + BUTTON_INTERVAL_MS) < current_time) {
                        // pending値は常にcurrent_amountを基準に+1（0-9の範囲で循環）
                        *ctx->pending_amount = (*ctx->current_amount + 1) % 10;
                        *ctx->pending_updated = 1;
                        
                        // IRQ送信
                        DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);
                        
                        g_irq_pulse_start_time = get_systick_count_ms();
                        g_irq_pulse_pending = 1;
                        ctx->event->last_press_time_ms = current_time;
                        
                        printf("[%s] Value changed: %d -> %d (blink mode)\n", 
                               ctx->name, *ctx->current_amount, *ctx->pending_amount);
                    }
                }
            } else {
                // 長押し完了後のリリース
                printf("[%s] Released after %lu ms (long press threshold: %u ms)\n", 
                       ctx->name, (unsigned long)elapsed, BUTTON_LONG_PRESS_MS);
            }
            
            *ctx->press_start = 0;
            *ctx->release_candidate_flag = 0;
        }
    }
    // 長押し完了後のリリース処理
    else if (*ctx->press_start == 0xFFFFFFFF) {
        if (!*ctx->release_candidate_flag) {
            *ctx->release_candidate_time = current_time;
            *ctx->release_candidate_flag = 1;
        } else if ((current_time - *ctx->release_candidate_time) >= 50) {
            printf("[%s] Released (after long press completion)\n", ctx->name);
            *ctx->press_start = 0;
            *ctx->release_candidate_flag = 0;
        }
    }
    // リセット
    else {
        *ctx->release_candidate_flag = 0;
    }
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
    
    // ========== ソレノイドテスト（300msec毎にON/OFF） ==========
    #if 0  // テストを有効化する場合は #if 1、無効化する場合は #if 0
    {
        systick_t last_toggle_time = 0;
        uint8_t solenoid_state = 0;
        
        printf("\r\n");
        printf("========================================\r\n");
        printf("Solenoid Test Mode\r\n");
        printf("PA7  (BLOCK_SOL)  : Toggle every 300ms\r\n");
        printf("PA17 (COIN_SOL)   : Toggle every 300ms\r\n");
        printf("Press Reset to exit\r\n");
        printf("========================================\r\n");
        
        last_toggle_time = get_systick_count_ms();
        
        while (1) {
            systick_t current_time = get_systick_count_ms();
            
            // 300msec経過チェック
            if ((current_time - last_toggle_time) >= 300) {
                solenoid_state = !solenoid_state;
                
                if (solenoid_state) {
                    DL_GPIO_setPins(BLOCK_SOL_PORT, BLOCK_SOL_PIN);
                    DL_GPIO_setPins(COIN_SOL_PORT, COIN_SOL_PIN);
                    printf("[%lu ms] Solenoids ON  (PA7=1, PA17=1)\r\n", (unsigned long)current_time);
                } else {
                    DL_GPIO_clearPins(BLOCK_SOL_PORT, BLOCK_SOL_PIN);
                    DL_GPIO_clearPins(COIN_SOL_PORT, COIN_SOL_PIN);
                    printf("[%lu ms] Solenoids OFF (PA7=0, PA17=0)\r\n", (unsigned long)current_time);
                }
                
                last_toggle_time = current_time;
            }
            
            // CPUリソース削減のため短時間待機
            delay_cycles(CPUCLK_FREQ / 1000);  // 1ms待機
        }
    }
    #endif
    // ========== ソレノイドテスト終了 ==========

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

    // ATIRQ送信用の一時変数を現在値で初期化（起動時）
    g_pending_left_amount = g_left_amount;
    g_pending_right_amount = g_right_amount;
    
    // 内部状態も初期化
    g_axon_state.left_amount = g_left_amount;
    g_axon_state.right_amount = g_right_amount;
    g_axon_state.led_blink_mode = 0;   // 通常モード（点灯）
    g_axon_state.led_blink_state = 1;  // 点灯状態
    
    // ★追加: リセットフラグを設定（起動時のATIRQでSOMAに通知）
    g_axon_status_shared.reset_flag = 1;

    _set_segment_leds(g_axon_state.left_amount, g_axon_state.right_amount);

    // ==========================================
    // 各スイッチの初期状態を読み取り（起動時の状態同期）
    // ==========================================
    // 注: メインループ開始前に現在の物理的な状態を読み取り、
    //     prev_*_state変数を初期化することで、起動後の最初の状態変化を
    //     正しく検出できるようにする
    uint8_t initial_connector_state = DL_GPIO_readPins(CONNECTOR_DET_PORT, CONNECTOR_DET_PIN) ? 0U : 1U;
    uint8_t initial_soldout_state = DL_GPIO_readPins(SOLDOUT_SW_PORT, SOLDOUT_SW_PIN) ? 0U : 1U;
    uint8_t initial_door_state = DL_GPIO_readPins(DOOR_OC_DET_PORT, DOOR_OC_DET_PIN) ? 0U : 1U;
    
    printf("[INIT] Switch initial states: CONNECTOR=%d, SOLDOUT=%d, DOOR=%d\n",
           initial_connector_state, initial_soldout_state, initial_door_state);

    while (1) {
        // check dial rotation
        if (g_axon_state.status == STATE_SOL_ON) {
            bool dial_rotated = _check_debounce_complete(&g_rotary_event, SWITCH_DEBOUNCE_US);
            if (dial_rotated) {
                g_axon_state.sol_state   = 0;
                g_axon_state.dial_detect = 1;
                _change_status(&g_axon_state, STATE_DIAL_DETECT);
                _set_solenoid_pins(g_axon_state.sol_state);
                changed = true;
            }
        } else {
            // 0.1ms wait
            delay_cycles(CPUCLK_FREQ / 10000);
        }

        // ==========================================
        // ボタン押下処理（SOMA-TG仕様書 7.3, 7.4, 7.5準拠）
        // ==========================================
        
        // 両ボタン同時長押し検出（設定確定: 変更モード終了）
        static uint8_t both_buttons_pressed = 0;
        static systick_t both_press_start = 0;
        static uint8_t both_release_lockout = 0;  // 両ボタンリリース直後の個別処理抑制フラグ
        
        // Button1/2用のチャタリング対策変数
        static systick_t release_candidate_time1 = 0;
        static uint8_t release_candidate1 = 0;
        static systick_t release_candidate_time2 = 0;
        static uint8_t release_candidate2 = 0;
        
        // ボタンコンテキスト初期化
        button_context_t btn1_ctx = {
            .press_start = &g_axon_state.button1_press_start,
            .event = &g_button_1_event,
            .pending_amount = &g_pending_left_amount,
            .pending_updated = &g_pending_left_updated,
            .current_amount = &g_left_amount,  // ★修正: g_axon_state.left_amount -> g_left_amount
            .release_candidate_time = &release_candidate_time1,
            .release_candidate_flag = &release_candidate1,
            .name = "BUTTON1"
        };
        
        button_context_t btn2_ctx = {
            .press_start = &g_axon_state.button2_press_start,
            .event = &g_button_2_event,
            .pending_amount = &g_pending_right_amount,
            .pending_updated = &g_pending_right_updated,
            .current_amount = &g_right_amount,  // ★修正: g_axon_state.right_amount -> g_right_amount
            .release_candidate_time = &release_candidate_time2,
            .release_candidate_flag = &release_candidate2,
            .name = "BUTTON2"
        };
        
        // 両ボタン同時押下検出（誤操作防止＋点滅モード終了）
        // 注: 仕様書7.3章では各ボタン長押しでトグルだが、両ボタン長押しで強制終了も実装
        if (g_button_1_event.pressed && g_button_2_event.pressed) {
            if (!both_buttons_pressed) {
                both_buttons_pressed = 1;
                both_press_start = get_systick_count_ms();
                printf("[BUTTON] Both buttons pressed, individual button actions disabled\n");
                // 両ボタン同時押し開始時に個別のpress_startをリセット（長押し検出を無効化）
                g_axon_state.button1_press_start = 0;
                g_axon_state.button2_press_start = 0;
            }
            
            // リリースロックアウトをクリア（両ボタン押下中は通常状態）
            both_release_lockout = 0;
            
            // 2秒経過チェック（点滅モード中のみ終了処理）
            if (g_axon_state.led_blink_mode && 
                (get_systick_count_ms() - both_press_start) >= BUTTON_BOTH_LONG_PRESS_MS) {
                g_axon_state.led_blink_mode = 0;
                g_axon_state.led_blink_state = 1;
                printf("[BUTTON] Both buttons long press detected, LED blink mode stopped\n");
                // リリース待ち状態に設定（連続実行防止）
                g_axon_state.button1_press_start = 0xFFFFFFFF;
                g_axon_state.button2_press_start = 0xFFFFFFFF;
            }
        } else {
            if (both_buttons_pressed) {
                printf("[BUTTON] At least one button released (both_buttons_pressed cleared)\n");
                both_buttons_pressed = 0;
                // 両ボタン解除直後は個別のpress_startを強制リセット（汚染防止）
                g_axon_state.button1_press_start = 0;
                g_axon_state.button2_press_start = 0;
                // ★両ボタンリリース直後は個別処理を抑制（リリースエッジでのインクリメント防止）
                both_release_lockout = 1;
            }
            
            // ★両ボタンが両方リリースされたらロックアウト解除
            if (both_release_lockout && !g_button_1_event.pressed && !g_button_2_event.pressed) {
                both_release_lockout = 0;
                printf("[BUTTON] Both buttons fully released, individual processing re-enabled\n");
            }
        }
        
        // Button1処理（両ボタン同時押し中または両ボタンリリース直後は個別処理をスキップ）
        if (g_button_1_event.pressed && !both_buttons_pressed && !both_release_lockout) {
            handle_button_press(&btn1_ctx, both_buttons_pressed, g_button_2_event.pressed);
        } else if (!both_buttons_pressed && !both_release_lockout) {
            handle_button_release(&btn1_ctx);
        }
        
        // Button2処理（両ボタン同時押し中または両ボタンリリース直後は個別処理をスキップ）
        if (g_button_2_event.pressed && !both_buttons_pressed && !both_release_lockout) {
            handle_button_press(&btn2_ctx, both_buttons_pressed, g_button_1_event.pressed);
        } else if (!both_buttons_pressed && !both_release_lockout) {
            handle_button_release(&btn2_ctx);
        }
        
        // ESCROW SW (エスクロ/返却ボタン検知)
        if (g_escrow_event.pressed) {
            // エスクロ検知時の処理: STATUS bit4をLatch、IRQ信号をSOMAに送信
            axon_status_latch_escrow();  // STATUS bit4=1 (返却ボタン押下)に設定
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (Escrow detected)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000));
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            g_escrow_event.pressed            = 0;
            g_escrow_event.last_press_time_ms = log_time;
        }

        if (g_coindet_event.pressed) {
            // 現金検知時の処理: STATUS bit3をLatch、IRQ信号をSOMAに送信
            axon_status_latch_coin();  // STATUS bit3=0 (現金投入中)に設定
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (Coin detected)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000));
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            g_coindet_event.pressed = 0;
        }

        // INSERT_DET（コネクタ検知/AXON接続検知）
        // 両エッジ検出: pressed=1でAXON接続、pressed=0でAXON切断
        static uint8_t prev_connector_state = 0xFF;
        static uint8_t connector_initialized = 0;
        
        // 初回のみ現在の状態で初期化
        if (!connector_initialized) {
            prev_connector_state = initial_connector_state;
            g_connector_event.pressed = initial_connector_state;  // ISRの変数も同期
            g_axon_status_shared.connector_det = initial_connector_state;
            connector_initialized = 1;
        }
        
        if (g_connector_event.pressed != prev_connector_state) {
            uint8_t new_state = g_connector_event.pressed;
            g_axon_status_shared.connector_det = new_state;  // STATUS bit設定
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (INSERT_DET %s)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000),
                   new_state ? "Connected" : "Disconnected");
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            prev_connector_state = new_state;
            g_connector_event.last_press_time_ms = log_time;
        }

        // 売り切れ検知SW
        // 両エッジ検出: pressed=1で売り切れ、pressed=0で売り切れ解除
        static uint8_t prev_soldout_state = 0xFF;
        static uint8_t soldout_initialized = 0;
        
        // 初回のみ現在の状態で初期化
        if (!soldout_initialized) {
            prev_soldout_state = initial_soldout_state;
            g_soldout_event.pressed = initial_soldout_state;  // ISRの変数も同期
            g_axon_status_shared.sold_out = initial_soldout_state;
            soldout_initialized = 1;
        }
        
        if (g_soldout_event.pressed != prev_soldout_state) {
            uint8_t new_state = g_soldout_event.pressed;
            g_axon_status_shared.sold_out = new_state;  // STATUS bit1設定
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (SOLDOUT %s)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000),
                   new_state ? "ON" : "OFF");
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            prev_soldout_state = new_state;
            g_soldout_event.last_press_time_ms = log_time;
        }

        // ドア開閉検知SW
        // 両エッジ検出: pressed=1でドア開、pressed=0でドア閉
        static uint8_t prev_door_state = 0xFF;
        static uint8_t door_initialized = 0;
        
        // 初回のみ現在の状態で初期化
        if (!door_initialized) {
            prev_door_state = initial_door_state;
            g_door_event.pressed = initial_door_state;  // ISRの変数も同期
            g_axon_status_shared.door_open = initial_door_state;
            door_initialized = 1;
        }
        
        if (g_door_event.pressed != prev_door_state) {
            uint8_t new_state = g_door_event.pressed;
            g_axon_status_shared.door_open = new_state;  // STATUS bit6設定
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (DOOR %s)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000),
                   new_state ? "Opened" : "Closed");
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            prev_door_state = new_state;
            g_door_event.last_press_time_ms = log_time;
        }

        // ダイヤル回転検知（CN3 ROT_DET 11pin）
        if (g_rotary_event.pressed) {
            // ダイヤル回転検知時の処理: Latch、IRQ信号をSOMAに送信
            axon_status_latch_dial();  // 内部状態にダイヤル回転をラッチ
            g_dial_rotation_count++;   // カウンタインクリメント（0xFF→0x00へ自動ラップアラウンド）
            g_axon_status_shared.dial_rotation_count = g_dial_rotation_count;
            
            systick_t log_time = get_systick_count_ms();
            uint32_t log_sec = log_time / 1000;
            printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] High -> Low (Dial rotated)\n",
                   (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, (unsigned long)(log_time%1000));
            DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);  // IRQ_N = Low (Active)
            
            // IRQ_Nパルス制御をメインループに移管
            g_irq_pulse_start_time = log_time;
            g_irq_pulse_pending = 1;
            
            g_rotary_event.pressed = 0;
            g_rotary_event.last_press_time_ms = log_time;
        }


        // ==========================================
        // 7セグLED更新処理（点滅制御含む）
        // ==========================================
        
        // グローバル変数とローカル変数の同期は update_segment_leds() で行われるため、ここでは不要
        // （update_segment_leds() が g_left_amount, g_right_amount, g_axon_state を同時に更新）

        // LED点滅制御（変更モード中は500ms周期で点滅）
        static systick_t last_blink_toggle = 0;
        static uint8_t prev_blink_mode = 0;  // 前回の点滅モード状態
        systick_t current_time_blink = get_systick_count_ms();
        
        // 点滅モード開始時の初期化
        if (g_axon_state.led_blink_mode && !prev_blink_mode) {
            last_blink_toggle = current_time_blink;
            g_axon_state.led_blink_state = 1;  // 点灯状態から開始
            printf("[LED] Blink mode started\n");
        }
        prev_blink_mode = g_axon_state.led_blink_mode;
        
        if (g_axon_state.led_blink_mode) {
            // 点滅モード（変更中）
            if ((current_time_blink - last_blink_toggle) >= LED_BLINK_INTERVAL_MS) {
                g_axon_state.led_blink_state = !g_axon_state.led_blink_state;
                last_blink_toggle = current_time_blink;
            }
            
            // 点滅状態に応じて表示/消灯
            if (g_axon_state.led_blink_state) {
                _set_segment_leds(g_axon_state.left_amount, g_axon_state.right_amount);
            } else {
                _set_segment_leds(0xFF, 0xFF);  // 消灯（全セグメントOFF）
            }
        } else {
            // 通常モード（常時点灯）
            g_axon_state.led_blink_state = 1;
            _set_segment_leds(g_axon_state.left_amount, g_axon_state.right_amount);
        }

        // ==========================================
        // IRQ_N 50msパルス制御（メインループ処理）
        // ==========================================
        // ボタン押下やイベント発生時にIRQ_N Lowにセットし、50ms後にHighに戻す
        // delay_cycles()を使わず、メインループで時間経過をチェック
        // 注: UART処理の直前に配置し、IRQ_N信号を速やかにHighに戻す
        if (g_irq_pulse_pending) {
            systick_t current_time = get_systick_count_ms();
            if ((current_time - g_irq_pulse_start_time) >= 50) {
                // 50ms経過 → IRQ_N Highに戻す
                DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, UART_IRQ_OUT_PIN);
                g_irq_pulse_pending = 0;
                
                uint32_t log_sec = current_time / 1000;
                printf("[%02u:%02u:%02u.%03u][IRQ_SIGNAL] Low -> High (50ms pulse completed)\n",
                       (unsigned int)((log_sec/3600)%24), (unsigned int)((log_sec/60)%60),
                       (unsigned int)(log_sec%60), (unsigned int)(current_time%1000));
            }
        }

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
                        // デバッグ出力無効化（UART通信安定化のため）
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
                        // デバッグ出力無効化（UART通信安定化のため）
                        #if 0
                        NVIC_DisableIRQ(UART0_INT_IRQn);
                        printf("[SETAXON] Received! Header=0x%02X, Len=0x%02X, CmdID=0x%02X\n",
                               header, length, cmd_id);
                        NVIC_EnableIRQ(UART0_INT_IRQn);
                        #endif
                        handled = axon_handle_setaxon(rx_complete_frame);
                        #if 0
                        if (handled) {
                            NVIC_DisableIRQ(UART0_INT_IRQn);
                            printf("[SETAXON] Handler returned success (ACK sent)\n");
                            NVIC_EnableIRQ(UART0_INT_IRQn);
                            // 設定反映成功 - 状態更新
                            _change_status(&g_axon_state, STATE_NORMAL);
                            changed = true;
                        } else {
                            NVIC_DisableIRQ(UART0_INT_IRQn);
                            printf("[SETAXON] Handler returned failure\n");
                            NVIC_EnableIRQ(UART0_INT_IRQn);
                        }
                        #endif
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
        // 面番号重複検出時の自動リトライ処理
        // ==========================================
        // SOMA-TG仕様書 7.4 ①準拠:
        // 「Note over AXON: ❸のGPIO割り込みに戻ってL+1番の指定をリクエスト」
        // → 重複検出時は自動的に次の番号(L+1)をATIRQで送信
        
        static systick_t retry_last_send_time = 0;
        systick_t retry_current_time = get_systick_count_ms();
        
        if (g_retry_pending && !g_irq_pulse_pending) {
            // リトライ間隔チェック（100ms以上経過）
            if ((retry_current_time - retry_last_send_time) >= AUTO_RETRY_INTERVAL_MS) {
                // デバッグ出力: 自動リトライ実行
                uint32_t log_sec = retry_current_time / 1000;
                printf("[%02lu:%02lu:%02lu.%03lu][AUTO_RETRY] Face duplicate detected, retrying with FACE=%d\n",
                       (log_sec/3600)%24, (log_sec/60)%60, log_sec%60, 
                       (unsigned long)(retry_current_time%1000),
                       g_pending_right_updated ? g_pending_right_amount : g_pending_left_amount);
                
                // IRQ_N Low送信（自動リトライ）
                DL_GPIO_writePinsVal(UART_PORT, UART_IRQ_OUT_PIN, 0);
                
                g_irq_pulse_start_time = retry_current_time;
                g_irq_pulse_pending = 1;
                retry_last_send_time = retry_current_time;
                
                // 次の番号をATIRQで送信（s2a_packet.cで既にpending値がL+1に更新済み）
            }
        }
        
        // デバッグログをアイドル時に出力（UART処理完了後）
        // flush_debug_log();  // 必要に応じてコメント解除

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
        // RGB LED制御（SETAXON経由で制御、ここでは上書きしない）
        // ==========================================
        // 注: rgb_led_fade_update()は呼ばない（SETAXONのSET_LEDで制御）
        /*
        if ((period - last_led_update) >= 5) {      // 5msec周期
            // Full Color LED 点灯処理
            last_led_update = period;
            rgb_led_fade_update(selected_color_index, is_fast_blink, period);
        }
        */
    }
#endif  // AXON_BOARD
}
