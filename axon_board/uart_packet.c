#include "uart_packet.h"

#include <string.h>
#include <stdbool.h>

#include "driver/UART.h"
#include "driver_config.h"
#include "driver/utils/RingBuf.h"
#include "peripheral/msp_peripheral_config.h"
#include "isr.h"
#include "ti_msp_dl_config.h"
// for debug
#include "ti/driverlib/dl_uart.h"

#define UART_PACKET_WRITE_TIMEOUT_MS (100U)  // 100ms
#define UART_BUFFER_SIZE             (CONFIG_UART_BUFFER_SIZE)

typedef struct uart_isr_context {
    volatile uint32_t timestamp;
    volatile uint32_t received_bytes;
    volatile uint32_t rx_head;
    volatile uint32_t rx_tail;
    volatile uint32_t uart_errors;
    volatile uint8_t  rx_buffer[UART_BUFFER_SIZE];
    volatile bool     rx_in_progress;
    volatile bool     tx_in_progress;
} uart_context_t;

static const uint32_t rx_buffer_size = UART_BUFFER_SIZE;

static UART_Handle _uart_t2s_handle = NULL;
static UART_Handle _uart_s2a_handle = NULL;

static uart_context_t _t2s_uart_context = {0};
static uart_context_t _s2a_uart_context = {0};

static uart_packet_status_t _init_uart_t2s(void);
static uart_packet_status_t _init_uart_s2a(void);
static bool                 _is_valid_uart_command(uint8_t command);
static void read_callback_fxn(UART_Handle handle, void* buffer, size_t count, void* userArg, int_fast16_t status);
static void write_callback_fxn(UART_Handle handle, void* buffer, size_t count, void* userArg, int_fast16_t status);

uart_packet_status_t init_uart_ports(void) {
    uart_packet_status_t status = _init_uart_t2s();
    if (status != UART_PACKET_STATUS_SUCCESS) {
        return status;
    }

    status = _init_uart_s2a();
    if (status != UART_PACKET_STATUS_SUCCESS) {
        if (_uart_t2s_handle != NULL) {
            UART_close(_uart_t2s_handle);
            _uart_t2s_handle = NULL;
        }
    }

    return status;
}

uart_packet_status_t send_uart_t2s_packet(const uart_t2s_packet_t* packet) {
    if (_uart_t2s_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }

    if (packet == NULL) {
        return UART_PACKET_STATUS_INVALID_ARGUMENT;
    }

    // if ((packet->header != UART_T2S_HEADER) || (!_is_valid_uart_command(packet->command))) {
    //     return UART_PACKET_STATUS_INVALID_FORMAT;
    // }

    _t2s_uart_context.tx_in_progress = true;
    int_fast16_t result              = UART_write(_uart_t2s_handle, packet, sizeof(uart_t2s_packet_t), NULL);
    if (result != UART_STATUS_SUCCESS) {
        _t2s_uart_context.tx_in_progress = false;
        return UART_PACKET_STATUS_IO_ERROR;
    }

    while (_t2s_uart_context.tx_in_progress) {
        // wait for transmission complete
        __WFI();
    }

    flush_uart_t2s_tx_buffer();

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t receive_uart_t2s_packet(uart_t2s_packet_t* packet) {
    if (_uart_t2s_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }

    if (packet == NULL) {
        return UART_PACKET_STATUS_INVALID_ARGUMENT;
    }

    int_fast16_t result = UART_read(_uart_t2s_handle, _t2s_uart_context.rx_buffer, UART_BUFFER_SIZE, NULL);

    if ((result != UART_STATUS_SUCCESS && result != UART_STATUS_EINUSE)) {
        return UART_PACKET_STATUS_IO_ERROR;
    }

    if (result == UART_STATUS_SUCCESS) {
        _t2s_uart_context.rx_in_progress = true;
        _t2s_uart_context.received_bytes = 0;

        return UART_PACKET_STATUS_RECV_START;
    }

    // if read is in progress, cancel it to process received data
    UARTMSP_dmaStopRx(_uart_t2s_handle);

    UARTMSP_dmaRx(_uart_t2s_handle, true);

    UART_Object*         object     = UART_Obj_Ptr(_uart_t2s_handle);
    UART_Buffers_Object* buffer_obj = UART_buffersObject(object);

    size_t available = RingBuf_getCount(&buffer_obj->rxBuf);
    // 削除: size_t received = 0; (未使用のため削除 2025/11/13)
    if (available < sizeof(uart_t2s_packet_t)) {
        return UART_PACKET_STATUS_NO_DATA;
    }

    while (1) {
        available = RingBuf_getCount(&buffer_obj->rxBuf);
        if (available < sizeof(uart_t2s_packet_t)) {
            return UART_PACKET_STATUS_NO_DATA;
        }

        uint8_t rx_data = 0x00;
        RingBuf_get(&buffer_obj->rxBuf, &rx_data);
        if (rx_data == UART_T2S_HEADER) {
            packet->header = rx_data;
            break;
        }
    }

    for (size_t i = 1; i < sizeof(uart_t2s_packet_t); i++) {
        uint8_t rx_data = 0x00;
        RingBuf_get(&buffer_obj->rxBuf, &rx_data);
        ((uint8_t*)packet)[i] = rx_data;
    }

#ifdef SOMA_BOARD
    bool valid_header  = (packet->header == UART_T2S_HEADER);
    bool valid_command = _is_valid_uart_command(packet->command);
    if (!valid_header) {
        DL_GPIO_writePinsVal(LED_PORT, LED_PORT_3_PIN, LED_PORT_3_PIN);
    }
    if (!valid_command) {
        DL_GPIO_writePinsVal(LED_PORT, LED_PORT_4_PIN, LED_PORT_4_PIN);
    }
#endif  // SOMA_BOARD

    if ((packet->header != UART_T2S_HEADER) || (!_is_valid_uart_command(packet->command))) {
        _t2s_uart_context.received_bytes = 0;

        return UART_PACKET_STATUS_INVALID_FORMAT;
    }

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t wait_for_uart_t2s_packet(uart_t2s_packet_t* packet, systick_t timeout) {
    systick_t start_time = get_systick_count_ms();
    systick_t elapsed    = 0;

    while (elapsed < timeout) {
        uart_packet_status_t status = receive_uart_t2s_packet(packet);
        if (status == UART_PACKET_STATUS_SUCCESS) {
            return UART_PACKET_STATUS_SUCCESS;
        } else if (status == UART_PACKET_STATUS_TIMEOUT || status == UART_PACKET_STATUS_IO_ERROR) {
            // continue waiting
        } else {
            return status;  // invalid format
        }

        // wait a bit before retrying
        delay_cycles(CPUCLK_FREQ / 1000U);  // 1ms

        elapsed = get_systick_count_ms() - start_time;
    }

    UART_readCancel(_uart_t2s_handle);
    // flush_uart_t2s_rx_buffer();

    _t2s_uart_context.rx_in_progress = false;

    return UART_PACKET_STATUS_TIMEOUT;
}

uart_packet_status_t flush_uart_t2s_tx_buffer(void) {
    if (_uart_t2s_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }
    UART_Object*         object        = UART_Obj_Ptr(_uart_t2s_handle);
    UART_Buffers_Object* buffersObject = UART_buffersObject(object);

    RingBuf_flush(&buffersObject->txBuf);

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t flush_uart_t2s_rx_buffer(void) {
    if (_uart_t2s_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }
    UART_Object*         object        = UART_Obj_Ptr(_uart_t2s_handle);
    UART_Buffers_Object* buffersObject = UART_buffersObject(object);

    RingBuf_flush(&buffersObject->rxBuf);

    _t2s_uart_context.received_bytes = 0;
    memset((void*)_t2s_uart_context.rx_buffer, 0, sizeof(_t2s_uart_context.rx_buffer));

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t send_uart_s2a_packet(const uart_s2a_packet_t* packet) {
    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }

    if (packet == NULL) {
        return UART_PACKET_STATUS_INVALID_ARGUMENT;
    }

    if ((packet->header != UART_S2A_HEADER) || (!_is_valid_uart_command(packet->command))) {
        return UART_PACKET_STATUS_INVALID_FORMAT;
    }

    _s2a_uart_context.tx_in_progress = true;
    int_fast16_t result              = UART_write(_uart_s2a_handle, packet, sizeof(uart_s2a_packet_t), NULL);
    if (result != UART_STATUS_SUCCESS) {
        _s2a_uart_context.tx_in_progress = false;
        return UART_PACKET_STATUS_IO_ERROR;
    }

    while (_s2a_uart_context.tx_in_progress) {
        // wait for transmission complete
        __WFI();
    }

    flush_uart_s2a_tx_buffer();

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t receive_uart_s2a_packet(uart_s2a_packet_t* packet) {
    // ==========================================
    // 1. 入力パラメータの検証
    // ==========================================
    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }

    if (packet == NULL) {
        return UART_PACKET_STATUS_INVALID_ARGUMENT;
    }

    // ==========================================
    // 2. UART読み取りの開始
    // ==========================================
    int_fast16_t result = UART_read(_uart_s2a_handle, _s2a_uart_context.rx_buffer, UART_BUFFER_SIZE, NULL);

    // エラーチェック: SUCCESSまたはEINUSE以外はエラー
    if ((result != UART_STATUS_SUCCESS && result != UART_STATUS_EINUSE)) {
        return UART_PACKET_STATUS_IO_ERROR;
    }

    // 読み取りが正常に開始された場合
    if (result == UART_STATUS_SUCCESS) {
        _s2a_uart_context.rx_in_progress = true;  // 修正: _t2s_ → _s2a_ (2025/11/13)
        _s2a_uart_context.received_bytes = 0;      // 修正: _t2s_ → _s2a_ (2025/11/13)
        return UART_PACKET_STATUS_RECV_START;
    }

    // ==========================================
    // 3. DMA受信の制御
    // ==========================================
    // 進行中の読み取りをキャンセルし、受信データを処理
    UARTMSP_dmaStopRx(_uart_s2a_handle);
    UARTMSP_dmaRx(_uart_s2a_handle, true);

    // ==========================================
    // 4. 受信バッファの確認
    // ==========================================
    UART_Object*         object     = UART_Obj_Ptr(_uart_s2a_handle);
    UART_Buffers_Object* buffer_obj = UART_buffersObject(object);
    size_t               available  = RingBuf_getCount(&buffer_obj->rxBuf);

    // パケットサイズ分のデータが揃っていない場合
    if (available < sizeof(uart_s2a_packet_t)) {
        return UART_PACKET_STATUS_NO_DATA;
    }

    // ==========================================
    // 5. ヘッダーの検索
    // ==========================================
    // バッファ内からヘッダー(0xFF)を探す
    while (1) {
        available = RingBuf_getCount(&buffer_obj->rxBuf);
        if (available < sizeof(uart_s2a_packet_t)) {
            return UART_PACKET_STATUS_NO_DATA;
        }

        uint8_t rx_data = 0x00;
        RingBuf_get(&buffer_obj->rxBuf, &rx_data);
        
        // ヘッダーを発見したらループを抜ける
        if (rx_data == UART_S2A_HEADER) {
            packet->header = rx_data;
            break;
        }
    }

    // ==========================================
    // 6. 残りのパケットデータを取得
    // ==========================================
    // ヘッダー以降のデータ(commandとdata)を読み取る
    for (size_t i = 1; i < sizeof(uart_s2a_packet_t); i++) {
        uint8_t rx_data = 0x00;
        RingBuf_get(&buffer_obj->rxBuf, &rx_data);
        ((uint8_t*)packet)[i] = rx_data;
    }

    // ==========================================
    // 7. パケットの妥当性検証
    // ==========================================
    bool valid_header  = (packet->header == UART_S2A_HEADER);
    bool valid_command = _is_valid_uart_command(packet->command);

    if (!valid_header || !valid_command) {
        _s2a_uart_context.received_bytes = 0;
        UART_readCancel(_uart_s2a_handle);
        return UART_PACKET_STATUS_INVALID_FORMAT;
    }

    // ==========================================
    // 8. 正常終了 - 次の受信のために状態をリセット
    // ==========================================
    // 追加: 次の受信を可能にするため状態をクリア (2025/11/13)
    _s2a_uart_context.received_bytes = 0;
    _s2a_uart_context.rx_in_progress = false;
    UART_readCancel(_uart_s2a_handle);

    return UART_PACKET_STATUS_SUCCESS;
}

// uart_packet_status_t _receive_uart_core(UART_Handle handle, uart_context_t* context) {
//     if (handle == NULL) {
//         return UART_PACKET_STATUS_NOT_INITIALIZED;
//     }

//     if (context == NULL) {
//         return UART_PACKET_STATUS_INVALID_ARGUMENT;
//     }

//     int_fast16_t result = UART_read(handle, context->rx_buffer, , NULL);
//     if (result != UART_STATUS_SUCCESS) {
//         return UART_PACKET_STATUS_IO_ERROR;
//     }

//     return UART_PACKET_STATUS_SUCCESS;
// }

uart_packet_status_t start_receive_uart_s2a_packet(void) {
    uart_packet_status_t status;

    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }

    while (1) {
        status = UART_read(_uart_s2a_handle, _s2a_uart_context.rx_buffer, UART_BUFFER_SIZE, NULL);
        if (status == UART_STATUS_SUCCESS || status == UART_STATUS_EINUSE) {
            _s2a_uart_context.rx_in_progress = true;
            return UART_PACKET_STATUS_RECV_START;
        } else {
            UART_readCancel(_uart_s2a_handle);
        }
    }
}

uart_packet_status_t wait_for_uart_s2a_packet(uart_s2a_packet_t* packet, systick_t timeout) {
    systick_t            start_time = get_systick_count_ms();
    systick_t            elapsed    = 0;
    uart_packet_status_t status;

    // 修正: _t2s_ → _s2a_ (2025/11/13)
    if (!_s2a_uart_context.rx_in_progress) {
        return UART_PACKET_STATUS_INVALID_ARGUMENT;  // TODO: change error code
    }

    while (elapsed < timeout) {
        status = receive_uart_s2a_packet(packet);

        elapsed = get_systick_count_ms() - start_time;

        if (status == UART_PACKET_STATUS_SUCCESS) {
            UART_readCancel(_uart_s2a_handle);
            // flush_uart_s2a_rx_buffer();

            _s2a_uart_context.rx_in_progress = false;

            return UART_PACKET_STATUS_SUCCESS;
        } else if (status == UART_PACKET_STATUS_TIMEOUT || status == UART_PACKET_STATUS_IO_ERROR ||
                   status == UART_PACKET_STATUS_NO_DATA) {
            // continue waiting
        } else {
            _s2a_uart_context.rx_in_progress = false;
            return status;  // invalid format
        }

        // wait a bit before retrying
        delay_cycles(CPUCLK_FREQ / 1000U);  // 1ms
    }

    // UART_readCancel(_uart_s2a_handle);
    // flush_uart_s2a_rx_buffer();

    _s2a_uart_context.rx_in_progress = false;

    return UART_PACKET_STATUS_TIMEOUT;
}

uart_packet_status_t flush_uart_s2a_tx_buffer(void) {
    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }
    UART_Object*         object        = UART_Obj_Ptr(_uart_s2a_handle);
    UART_Buffers_Object* buffersObject = UART_buffersObject(object);

    RingBuf_flush(&buffersObject->txBuf);

    return UART_PACKET_STATUS_SUCCESS;
}

uart_packet_status_t flush_uart_s2a_rx_buffer(void) {
    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_NOT_INITIALIZED;
    }
    UART_Object*         object        = UART_Obj_Ptr(_uart_s2a_handle);
    UART_Buffers_Object* buffersObject = UART_buffersObject(object);

    RingBuf_flush(&buffersObject->rxBuf);

    _s2a_uart_context.received_bytes = 0;
    memset((void*)_s2a_uart_context.rx_buffer, 0, sizeof(_s2a_uart_context.rx_buffer));

    return UART_PACKET_STATUS_SUCCESS;
}

static uart_packet_status_t _init_uart_t2s(void) {
#ifdef SOMA_BOARD
    if (_uart_t2s_handle != NULL) {
        return UART_PACKET_STATUS_SUCCESS;
    }

    _t2s_uart_context.received_bytes = 0;
    _t2s_uart_context.rx_head        = 0;
    _t2s_uart_context.rx_tail        = UART_BUFFER_SIZE - 1;

    UART_Params params;

    UART_Params_init(&params);
    params.baudRate       = UART_BAUD_RATE;
    params.readMode       = UART_Mode_CALLBACK;
    params.writeMode      = UART_Mode_CALLBACK;
    params.readReturnMode = UART_ReadReturnMode_PARTIAL;
    params.readCallback   = read_callback_fxn;
    params.writeCallback  = write_callback_fxn;
    params.userArg        = &_t2s_uart_context;

    _uart_t2s_handle = UART_open(CONFIG_UART_1, &params);
    if (_uart_t2s_handle == NULL) {
        return UART_PACKET_STATUS_IO_ERROR;
    }

    const UARTMSP_HWAttrs* attrs = UART_config[CONFIG_UART_1].hwAttrs;

    // configure RX timeout interrupt
    // DL_UART_setRXInterruptTimeout(attrs->regs, 1);  // 0-15
    // DL_UART_enableInterrupt(attrs->regs, DL_UART_DMA_INTERRUPT_RX_TIMEOUT);
    // DL_UART_enableDMAReceiveEvent(attrs->regs, DL_UART_DMA_INTERRUPT_RX_TIMEOUT);
    // DL_UART_enableInterrupt(attrs->regs, DL_UART_INTERRUPT_EOT_DONE);

    return UART_PACKET_STATUS_SUCCESS;
#else
    return UART_PACKET_STATUS_SUCCESS;
#endif  // SOMA_BOARD
}

static uart_packet_status_t _init_uart_s2a(void) {
    if (_uart_s2a_handle != NULL) {
        return UART_PACKET_STATUS_SUCCESS;
    }

    _s2a_uart_context.received_bytes = 0;
    _s2a_uart_context.rx_head        = 0;
    _s2a_uart_context.rx_tail        = UART_BUFFER_SIZE - 1;

    UART_Params params;

    UART_Params_init(&params);
    params.baudRate       = UART_BAUD_RATE;
    params.readMode       = UART_Mode_CALLBACK;
    params.writeMode      = UART_Mode_CALLBACK;
    params.readReturnMode = UART_ReadReturnMode_PARTIAL;
    params.readCallback   = read_callback_fxn;
    params.writeCallback  = write_callback_fxn;
    params.userArg        = &_s2a_uart_context;

    _uart_s2a_handle = UART_open(CONFIG_UART_0, &params);
    if (_uart_s2a_handle == NULL) {
        return UART_PACKET_STATUS_IO_ERROR;
    }

    const UARTMSP_HWAttrs* attrs = UART_config[CONFIG_UART_0].hwAttrs;

    // configure RX timeout interrupt
    // DL_UART_setRXInterruptTimeout(attrs->regs, 1);  // 0-15
    // DL_UART_enableInterrupt(attrs->regs, DL_UART_DMA_INTERRUPT_RX_TIMEOUT);
    // DL_UART_enableDMAReceiveEvent(attrs->regs, DL_UART_DMA_INTERRUPT_RX_TIMEOUT);
    // DL_UART_enableInterrupt(attrs->regs, DL_UART_INTERRUPT_EOT_DONE);

    return UART_PACKET_STATUS_SUCCESS;
}

static bool _is_valid_uart_command(uint8_t command) {
    return (command == UART_CMD_GET_STATUS_REQ) || (command == UART_CMD_GET_STATUS_RESP) ||
           (command == UART_CMD_SET_STATUS_REQ) || (command == UART_CMD_SET_STATUS_RESP);
}

static void read_callback_fxn(UART_Handle handle, void* buffer, size_t count, void* userArg, int_fast16_t status) {
    (void)buffer;

    if (status == UART_STATUS_SUCCESS) {
        // Successful read
        uart_context_t* context = (uart_context_t*)userArg;
        if (context != NULL) {
            context->received_bytes += count;
            context->timestamp = get_systick_count_ms();
        }
    } else {
        // Handle read error
        uart_context_t* context = (uart_context_t*)userArg;
        if (context != NULL) {
            context->received_bytes = 0;
            context->timestamp      = get_systick_count_ms();
            context->uart_errors++;
        }
    }
}

static void write_callback_fxn(UART_Handle handle, void* buffer, size_t count, void* userArg, int_fast16_t status) {
    (void)buffer;
    (void)count;

    if (status == UART_STATUS_SUCCESS) {
        // Successful write
        uart_context_t* context = (uart_context_t*)userArg;
        if (context != NULL) {
            context->tx_in_progress = false;
        }
    } else {
        // Handle write error
        uart_context_t* context = (uart_context_t*)userArg;
        if (context != NULL) {
            context->tx_in_progress = false;
            context->uart_errors++;
        }
    }
}