#ifndef __MSPM0_UART_PACKET_H__
#define __MSPM0_UART_PACKET_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/systick.h"

#define UART_HEADER_SIZE         1
#define UART_COMMAND_SIZE        1
#define UART_DATA_SIZE           2
#define UART_T2S_HEADER          0xFF
#define UART_S2A_HEADER          0xFF
#define UART_CMD_GET_STATUS_REQ  0x00
#define UART_CMD_GET_STATUS_RESP 0x01
#define UART_CMD_SET_STATUS_REQ  0x02
#define UART_CMD_SET_STATUS_RESP 0x03

typedef enum {
    UART_PACKET_STATUS_SUCCESS          = 0U,
    UART_PACKET_STATUS_RECV_START       = 1U,
    UART_PACKET_STATUS_NO_DATA          = 2U,
    UART_PACKET_STATUS_NOT_INITIALIZED  = 3U,
    UART_PACKET_STATUS_INVALID_ARGUMENT = 4U,
    UART_PACKET_STATUS_IO_ERROR         = 5U,
    UART_PACKET_STATUS_INVALID_FORMAT   = 6U,
    UART_PACKET_STATUS_TIMEOUT          = 7U
} uart_packet_status_t;

/// @brief POC用 TG-SOMA間通信パケット
/// @note header:0xFF, command:0x00~0x03, data:2byte
typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t data[2];
} uart_t2s_packet_t;

/// @brief POC用 SOMA-AXON間通信パケット
/// @note header:0xFF, command:0x00~0x03, data:1byte
typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t data;
} uart_s2a_packet_t;

/// @brief POC用 カプセルトイ状態
/// amount:0bXXXX0000
/// led_status:0b0000XX00
/// sol_status:0b000000X0
/// rotate_detect:0b0000000X
// typedef struct {
//     uint8_t amount : 4;         // 1~9 (+1)
//     uint8_t led_status : 2;     // 0b00:OFF 0b01:PWM(SLOW) 0b10:PWM(FAST) 0b11:ON
//     uint8_t sol_status : 1;     // 0:OFF 1:ON
//     uint8_t lotate_detect : 1;  // 0:DISABLE 1:ENABLE
// } capsuletoy_status_t;

#define CAPSULETOY_AMOUNT_MASK           0xF0U
#define CAPSULETOY_AMOUNT_SHIFT          4U
#define CAPSULETOY_AMOUNT_VALUE_MASK     0x0FU
#define CAPSULETOY_LED_STATUS_MASK       0x0CU
#define CAPSULETOY_LED_STATUS_SHIFT      2U
#define CAPSULETOY_LED_STATUS_VALUE_MASK 0x03U
#define CAPSULETOY_SOL_STATUS_MASK       0x02U
#define CAPSULETOY_ROTATE_DETECT_MASK    0x01U
#define CAPSULETOY_AMOUNT_MIN            1U
#define CAPSULETOY_AMOUNT_MAX            9U
#define CAPSULETOY_LED_OFF               0U
#define CAPSULETOY_LED_PWM_SLOW          1U
#define CAPSULETOY_LED_PWM_FAST          2U
#define CAPSULETOY_LED_ON                3U
#define CAPSULETOY_SOL_OFF               0U
#define CAPSULETOY_SOL_ON                1U
#define CAPSULETOY_ROTATE_DISABLE        0U
#define CAPSULETOY_ROTATE_ENABLE         1U

/// @brief カプセルトイ状態をエンコードする
/// @param amount 7セグLEDで表示する数値(1~9)
/// @param led LEDの状態(0:OFF, 1:PWM(SLOW), 2:PWM(FAST), 3:ON)
/// @param sol ソレノイドの状態(0:OFF, 1:ON)
/// @param rotate 回転検出の状態(0:DISABLE, 1:ENABLE)
/// @return エンコードされたカプセルトイ状態
static inline uint8_t capsuletoy_status_encode(uint8_t amount, uint8_t led, bool sol, bool rotate) {
    if (amount < CAPSULETOY_AMOUNT_MIN) {
        amount = CAPSULETOY_AMOUNT_MIN;
    } else if (amount > CAPSULETOY_AMOUNT_MAX) {
        amount = CAPSULETOY_AMOUNT_MAX;
    }

    if (led > CAPSULETOY_LED_ON) {
        led = CAPSULETOY_LED_ON;
    }

    return (uint8_t)(((amount & CAPSULETOY_AMOUNT_VALUE_MASK) << CAPSULETOY_AMOUNT_SHIFT) |
                     ((led & CAPSULETOY_LED_STATUS_VALUE_MASK) << CAPSULETOY_LED_STATUS_SHIFT) |
                     (sol ? CAPSULETOY_SOL_STATUS_MASK : 0U) | (rotate ? CAPSULETOY_ROTATE_DETECT_MASK : 0U));
}

/// @brief カプセルトイ状態をデコードする
/// @param status エンコードされたカプセルトイ状態
/// @param amount 7セグLEDで表示する数値(1~9)のポインタ(NULL可)
/// @param led LEDの状態(0:OFF, 1:PWM(SLOW), 2:PWM(FAST), 3:ON)のポインタ(NULL可)
/// @param sol ソレノイドの状態(0:OFF, 1:ON)のポインタ(NULL可)
/// @param rotate 回転検出の状態(0:DISABLE, 1:ENABLE)のポインタ(NULL可)
/// @note NULLを指定した場合、その値は設定されない
static inline void capsuletoy_status_decode(uint8_t status, uint8_t* amount, uint8_t* led, bool* sol, bool* rotate) {
    if (amount) {
        uint8_t decoded_amount = (status & CAPSULETOY_AMOUNT_MASK) >> CAPSULETOY_AMOUNT_SHIFT;
        if (decoded_amount < CAPSULETOY_AMOUNT_MIN) {
            decoded_amount = CAPSULETOY_AMOUNT_MIN;
        } else if (decoded_amount > CAPSULETOY_AMOUNT_MAX) {
            decoded_amount = CAPSULETOY_AMOUNT_MAX;
        }
        *amount = decoded_amount;
    }
    if (led) {
        uint8_t decoded_led = (status & CAPSULETOY_LED_STATUS_MASK) >> CAPSULETOY_LED_STATUS_SHIFT;
        if (decoded_led > CAPSULETOY_LED_ON) {
            decoded_led = CAPSULETOY_LED_ON;
        }
        *led = decoded_led;
    }
    if (sol) {
        *sol = (status & CAPSULETOY_SOL_STATUS_MASK) ? true : false;
    }
    if (rotate) {
        *rotate = (status & CAPSULETOY_ROTATE_DETECT_MASK) ? true : false;
    }
}

/// @brief UARTポートを初期化する
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t init_uart_ports(void);

/// @brief TG-SOMA向けパケットを送信する
/// @param packet 送信するパケット
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t send_uart_t2s_packet(const uart_t2s_packet_t* packet);

/// @brief TG-SOMA向けパケットを受信する
/// @param packet 受信結果を書き込むパケット
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t receive_uart_t2s_packet(uart_t2s_packet_t* packet);

/// @brief TG-SOMA向けパケットの受信を待機する
/// @param packet 受信結果を書き込むパケット
/// @param timeout タイムアウト時間
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t wait_for_uart_t2s_packet(uart_t2s_packet_t* packet, systick_t timeout);

/// @brief TG-SOMA向けパケットの送信バッファをフラッシュする
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t flush_uart_t2s_tx_buffer(void);

/// @brief TG-SOMA向けパケットの受信バッファをフラッシュする
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t flush_uart_t2s_rx_buffer(void);

/// @brief SOMA-AXON向けパケットを送信する
/// @param packet 送信するパケット
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t send_uart_s2a_packet(const uart_s2a_packet_t* packet);

/// @brief SOMA-AXON向けパケットを受信する
/// @param packet 受信結果を書き込むパケット
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t receive_uart_s2a_packet(uart_s2a_packet_t* packet);

uart_packet_status_t start_receive_uart_s2a_packet(void);

/// @brief SOMA-AXON向けパケットの受信を待機する
/// @param packet 受信結果を書き込むパケット
/// @param timeout タイムアウト時間
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t wait_for_uart_s2a_packet(uart_s2a_packet_t* packet, systick_t timeout);

/// @brief SOMA-AXON向けパケットの送信バッファをフラッシュする
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t flush_uart_s2a_tx_buffer(void);

/// @brief SOMA-AXON向けパケットの受信バッファをフラッシュする
/// @return UART_PACKET_STATUS_* のステータスコード
uart_packet_status_t flush_uart_s2a_rx_buffer(void);

#endif  // __MSPM0_UART_PACKET_H__