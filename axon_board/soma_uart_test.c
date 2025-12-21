#include "soma_uart_test.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>      // for memset
#include "ti_msp_dl_config.h"
#include "driver_config.h"
#include "peripheral/msp_peripheral_config.h"  // UART0ピン定義用

#define AXON_FRAME_SIZE 36
#define AXON_MAX_FRAME_SIZE 40  // CODEPKT用の最大サイズ

uint8_t rx_frame[AXON_FRAME_SIZE];
uint8_t decrypted_data[AES_DATA_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t frame_received = 0;

// 可変長フレーム受信バッファ（SETOKEY/CODEPKT/ERRCHK用）
uint8_t rx_variable_frame[AXON_MAX_FRAME_SIZE];
volatile uint8_t rx_variable_ready = 0;
volatile uint8_t rx_variable_length = 0;

// デバッグ用: 受信フレーム履歴（最新5フレーム）
#ifdef AXON_BOARD
#define DEBUG_FRAME_HISTORY_SIZE 5
typedef struct {
    uint8_t frame[AXON_FRAME_SIZE];
    uint32_t timestamp;
    uint8_t valid;
} debug_frame_t;

volatile debug_frame_t debug_frames[DEBUG_FRAME_HISTORY_SIZE] = {0};
volatile uint8_t debug_frame_index = 0;
#endif

// デバッグ変数（グローバルスコープ：isr.cとsoma_uart_test.cで共有）
volatile uint32_t debug_rx_count = 0;
volatile uint32_t debug_frame_count = 0;
volatile uint32_t debug_sync_reset_count = 0;
volatile uint32_t debug_complete_count = 0;
volatile uint8_t debug_last_byte = 0;
volatile uint8_t debug_rx_index = 0;
volatile uint8_t debug_byte1 = 0;
volatile uint32_t debug_byte1_ng_count = 0;
volatile uint32_t debug_isr_call_count = 0;    // ISR呼び出し回数
volatile uint32_t debug_iidx_value = 0;        // 最後のiidx値
volatile uint32_t debug_fifo_empty_count = 0;  // FIFO空判定回数
volatile uint32_t debug_uart_stat_value = 0;   // 最後のUART STAT値
volatile uint32_t debug_rxdata_raw_value = 0;  // 最後のRXDATA生値
volatile uint32_t debug_overrun_count = 0;     // RXオーバーラン検出回数
volatile uint32_t debug_framing_error_count = 0; // フレーミングエラー検出回数

// AES-128キー（16バイト固定）
// 注意: 本番環境では安全な方法でキーを管理してください
static const uint8_t aes_key[AES_KEY_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE
};

// AES-128 IV（初期化ベクトル、16バイト固定）
// 注意: 本番環境では各セッションで異なるIVを使用してください
static const uint8_t aes_iv[AES_IV_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

// CRC16-CCITT (ISO/IEC 13239) LSB-first implementation
// 多項式: X^16 + X^12 + X^5 + 1
// 通常形式(MSB): 0x1021
// 反転形式(LSB): 0x8408
uint16_t crc16_tep(const uint8_t* data, int len)
{
    uint16_t crc = 0xFFFF;  // 初期値
    
    for (int i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];  // データバイトをXOR
        
        // 8ビット分処理（LSB-first）
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x8408;  // LSB=1の場合、多項式でXOR
            } else {
                crc >>= 1;  // LSB=0の場合、単純に右シフト
            }
        }
    }
    
    // ★修正: 最終XOR処理を追加（s2a_packet.cと統一）
    crc ^= 0xFFFF;
    
    // ★修正: バイトスワップ（リトルエンディアン対応）
    return ((crc & 0xFF) << 8) | ((crc >> 8) & 0xFF);
}

void soma_uart_init(void)
{
    // 受信バッファ初期化
    rx_index = 0;
    frame_received = 0;
    memset(rx_frame, 0, AXON_FRAME_SIZE);
    
    // デバッグ: SOMA UARTテスト用にLEDをGPIOモードに変更
#ifdef AXON_BOARD
    // PWM機能を停止してGPIOモードに変更
    DL_TimerA_disableClock(TIMG0);  // LED_RG_TIM_INST
    DL_TimerG_disableClock(TIMG7);  // LED_B_TIM_INST
    
    // LED ピンを通常のGPIO出力として再設定（回路図準拠: PB0=R, PB1=G, PB2=B）
    DL_GPIO_initDigitalOutput(IOMUX_PINCM12);  // LED_R: PB0 (47pin)
    DL_GPIO_initDigitalOutput(IOMUX_PINCM13);  // LED_G: PB1 (48pin)
    DL_GPIO_initDigitalOutput(IOMUX_PINCM15);  // LED_B: PB2 (50pin)
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // PB0, PB1, PB2
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // 初期状態: LOW=消灯
    
    // デバッグ: 初期化開始を示す（全LED点灯）
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // RGB全点灯 (HIGH=点灯)
    for (volatile int i = 0; i < 1000000; i++);  // 約100ms待機（目視確認用）
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // RGB全消灯 (LOW=消灯)
#endif
 
    // ========== UART0初期化 ==========
    // 注意: UART0の全設定(NVIC含む)はSYSCFG_DL_init()内の_msp_peripheral_uart_init()で実施済み
    // ここでは追加の設定は不要
    
    // AES初期化
    DL_AES_reset(AES);
    DL_AES_setKey(AES, aes_key, DL_AES_KEY_LENGTH_128);

}

bool aes_decrypt_cbc(const uint8_t* encrypted_data, uint8_t* decrypted_data)
{
    if (encrypted_data == NULL || decrypted_data == NULL) {
        return false;
    }
    
    uint8_t iv_copy[16];
    uint8_t prev_cipher[16];
    uint8_t temp[16];
    
    // IVをコピー
    memcpy(iv_copy, aes_iv, 16);
    
    // AESモジュールをリセット
    DL_AES_reset(AES);
    
    // 復号キーを設定（uint8_t*で渡す）
    DL_AES_setKey(AES, aes_key, DL_AES_KEY_LENGTH_128);
    
    // ブロック1（16バイト）の復号
    // 暗号文を保存（次のブロックのIVに使用）
    memcpy(prev_cipher, &encrypted_data[0], 16);
    
    // ECBモードで復号（CBCは手動実装）
    // データをロード（uint8_t*ポインタを直接渡す）
    DL_AES_loadDataIn(AES, &encrypted_data[0]);
    
    // 復号完了待ち
    while (DL_AES_isBusy(AES));
    
    // 復号データ取得
    DL_AES_getDataOut(AES, temp);
    
    // CBC: 復号結果とIVをXOR
    for (int i = 0; i < 16; i++) {
        decrypted_data[i] = temp[i] ^ iv_copy[i];
    }
    
    // ブロック2（16バイト）の復号
    DL_AES_loadDataIn(AES, &encrypted_data[16]);
    
    // 復号完了待ち
    while (DL_AES_isBusy(AES));
    
    // 復号データ取得
    DL_AES_getDataOut(AES, temp);
    
    // CBC: 復号結果と前の暗号文をXOR
    for (int i = 0; i < 16; i++) {
        decrypted_data[16 + i] = temp[i] ^ prev_cipher[i];
    }
    
    return true;
}

/**
 * @brief 任意のパケットをUART送信（高速版 - ACK/ATIRQ用）
 * @param data 送信データバッファ
 * @param len 送信データ長
 * @return true: 成功, false: 失敗
 * @note 仕様書の10ms遅延なし（応答コマンド用）
 */
bool uart_send_packet_fast(const uint8_t* data, size_t len)
{
    if (data == NULL || len == 0) {
        return false;
    }
    
    // UART送信（確実に1バイトずつ送信）
    for (size_t i = 0; i < len; i++) {
        // TXFIFOが空になるまで待機
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST)) {
            __NOP();
        }
        
        // データ送信
        DL_UART_transmitData(S2A_UART_INST, data[i]);
        
        // 送信完了待機
        while (DL_UART_isBusy(S2A_UART_INST)) {
            __NOP();
        }
    }
    
    return true;
}

/**
 * @brief 任意のパケットをUART送信（汎用関数）
 * @param data 送信データバッファ
 * @param len 送信データ長
 * @return true: 成功, false: 失敗
 */
bool uart_send_packet(const uint8_t* data, size_t len)
{
    if (data == NULL || len == 0) {
        return false;
    }
    
    // ★仕様書準拠: コマンド送信前に10ms遅延（SOMA-AXON仕様書 4.2章）
    // CPUCLK_FREQ = 32MHz, 10ms = 32MHz × 0.01秒 = 320,000サイクル
    extern void delay_cycles(uint32_t cycles);
    delay_cycles(CPUCLK_FREQ / 100);  // 10ms待機
    
    return uart_send_packet_fast(data, len);
}

bool send_ack_frame(void)
{
    uint8_t tx_frame[ACK_NACK_FRAME_SIZE] = {
        FRAME_HEADER,      // Byte 0: ヘッダー
        ACK_NACK_LENGTH,   // Byte 1: フレーム長
        CMD_RESPONSE_TYPE, // Byte 2: コマンド種別（応答）
        CMD_ACK,           // Byte 3: ACK
        0x00, 0x00         // Byte 4-5: CRC（後で計算）
    };
    
    // CRC計算（Byte 2-3に対して）
    uint16_t crc = crc16_tep(&tx_frame[2], 2);
    tx_frame[4] = crc & 0xFF;
    tx_frame[5] = (crc >> 8) & 0xFF;
    
    // デバッグ: ACK送信開始（赤LED点灯、他はOFF）
#ifdef AXON_BOARD
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // G,B消灯
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);  // LED_R_PIN ON (PB0, HIGH=点灯)
#endif
    
    // 送信前に待機（UARTバッファとレシーバー準備）
    for (volatile int i = 0; i < 5000; i++);
    
    // UART送信（確実に1バイトずつ送信）
    for (int i = 0; i < ACK_NACK_FRAME_SIZE; i++) {
        // TXFIFOが空になるまで待機
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST)) {
            __NOP();
        }
        
        // データ送信
        DL_UART_transmitData(S2A_UART_INST, tx_frame[i]);
        
        // 送信完了待機
        while (DL_UART_isBusy(S2A_UART_INST)) {
            __NOP();
        }
        
        // バイト間に短い遅延
        for (volatile int j = 0; j < 100; j++);
    }
    
    // デバッグ: ACK送信完了（赤LED消灯）
#ifdef AXON_BOARD
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0);  // LED_R_PIN OFF (PB0, LOW=消灯)
#endif
    
    return true;
}

bool send_nack_frame(void)
{
    uint8_t tx_frame[ACK_NACK_FRAME_SIZE] = {
        FRAME_HEADER,      // Byte 0: ヘッダー
        ACK_NACK_LENGTH,   // Byte 1: フレーム長
        CMD_RESPONSE_TYPE, // Byte 2: コマンド種別（応答）
        CMD_NACK,          // Byte 3: NACK
        0x00, 0x00         // Byte 4-5: CRC（後で計算）
    };
    
    // CRC計算（Byte 2-3に対して）
    uint16_t crc = crc16_tep(&tx_frame[2], 2);
    tx_frame[4] = crc & 0xFF;
    tx_frame[5] = (crc >> 8) & 0xFF;
    
    // 送信前に待機（UARTバッファとレシーバー準備）
    for (volatile int i = 0; i < 5000; i++);
    
    // UART送信（確実に1バイトずつ送信）
    for (int i = 0; i < ACK_NACK_FRAME_SIZE; i++) {
        // TXFIFOが空になるまで待機
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST)) {
            __NOP();
        }
        
        // データ送信
        DL_UART_transmitData(S2A_UART_INST, tx_frame[i]);
        
        // 送信完了待機
        while (DL_UART_isBusy(S2A_UART_INST)) {
            __NOP();
        }
        
        // バイト間に短い遅延
        for (volatile int j = 0; j < 100; j++);
    }
    
    return true;
}

void soma_check_frame(void)
{
#ifdef AXON_BOARD
    // デバッグ: 関数が呼ばれたことを確認（赤LEDトグル）
    DL_GPIO_togglePins(GPIOA, DL_GPIO_PIN_0);  // LED_R: PA0
#endif
    
    if (!frame_received)
        return;

    frame_received = 0;
    
    // ★重要: ISRが退避した完成フレームをコピー（競合回避）
    // extern宣言はsoma_uart_test.hに移動済み
    
    if (!rx_complete_ready) {
        return;  // データ準備できていない
    }
    
    memcpy(rx_frame, rx_complete_frame, AXON_FRAME_SIZE);
    rx_complete_ready = 0;  // クリア
    
#ifdef AXON_BOARD
    // デバッグ: 受信フレームを履歴に保存
    debug_frame_count++;
    memcpy((void*)debug_frames[debug_frame_index].frame, rx_frame, AXON_FRAME_SIZE);
    debug_frames[debug_frame_index].timestamp = get_systick_count_ms();
    debug_frames[debug_frame_index].valid = 1;
    debug_frame_index = (debug_frame_index + 1) % DEBUG_FRAME_HISTORY_SIZE;
    
    // デバッグ: フレーム処理開始（青LED点灯、他はOFF）
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1);  // R,G消灯
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_2);  // LED_B: PB2 (HIGH=点灯)
#endif

    // ===== Header/LEN確認（CRCの前に確認） =====
    if (rx_frame[0] != 0x14) {
        // ヘッダー不正
        send_nack_frame();
        return;
    }
    if (rx_frame[1] != 0x20) {
        // 長さ不正
        send_nack_frame();
        return;
    }

    // ===== CRCチェック（暗号化されたデータ部に対して） =====
    // 受信CRC（リトルエンディアン形式）
    uint16_t crc_recv = rx_frame[34] | (rx_frame[35] << 8);
    
    // ★仕様書準拠: Data部のみ（32バイト）に対してCRC計算
    uint16_t crc_calc = crc16_tep(&rx_frame[2], 32);  // Byte[2-33]

    if (crc_recv != crc_calc) {
        // CRC error → NACKを送信
        send_nack_frame();
        return;
    }

    // ===== AES-128-CBC復号処理 =====
    // データ部（rx_frame[2]〜rx_frame[33]の32バイト）を復号
    if (!aes_decrypt_cbc(&rx_frame[2], decrypted_data)) {
        // 復号失敗 → NACKを送信
        send_nack_frame();
        return;
    }

    // ===== 復号後のデータ処理 =====
    // decrypted_data[0〜31]に平文データが格納されている
    // ここで実際のデータ処理を行う
    // 例: プロトコル解析、コマンド処理など

    // ===== テスト成功 =====
    // ACKを送信
    send_ack_frame();
    
    // LED点滅やGPIOトグルで成功を通知
#ifdef AXON_BOARD
    // 全LED消灯（処理完了）
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);  // RGB全消灯
#endif
}

// ============================================================
// AXONボード内ループバックテスト
// ============================================================
/**
 * @brief UART0ループバックテスト（AXON内部テスト用）
 * @details PA10(TX)とPA11(RX)を物理的に接続して実行
 *          36バイトのテストパターンを送信し、受信データと比較
 * @return true: テスト成功, false: テスト失敗
 * 
 * 使用方法:
 *   1. PA10(TX)とPA11(RX)をジャンパーで接続
 *   2. axon_routine.cから呼び出し
 *   3. UARTコンソールでテスト結果を確認
 */
bool axon_uart_loopback_test(void)
{
    printf("\n========================================\n");
    printf("AXON UART0 LOOPBACK TEST\n");
    printf("========================================\n");
    
    // マクロ展開確認
    printf("\n[PRE-TEST] Macro Check:\n");
    #ifdef AXON_BOARD
    printf("  AXON_BOARD is defined\n");
    #else
    printf("  AXON_BOARD is NOT defined\n");
    #endif
    printf("  S2A_UART_INST macro address = 0x%08lX\n", (unsigned long)S2A_UART_INST);
    printf("  UART0 address = 0x%08lX\n", (unsigned long)UART0);
    printf("  UART2 address = 0x%08lX\n", (unsigned long)UART2);
    
    // UARTレジスタ状態確認
    printf("\n[PRE-TEST] UART Register Check:\n");
    printf("  S2A_UART_INST addr = 0x%08lX\n", (unsigned long)S2A_UART_INST);
    printf("  CTL0  = 0x%08lX (bit0=EN, bit8=TXE, bit9=RXE)\n", (unsigned long)S2A_UART_INST->CTL0);
    printf("  STAT  = 0x%08lX (bit6=BUSY, bit4=TXFE, bit3=RXFF)\n", (unsigned long)S2A_UART_INST->STAT);
    printf("  IBRD  = 0x%08lX\n", (unsigned long)S2A_UART_INST->IBRD);
    printf("  FBRD  = 0x%08lX\n", (unsigned long)S2A_UART_INST->FBRD);
    printf("  IFLS  = 0x%08lX (RX FIFO level)\n", (unsigned long)S2A_UART_INST->IFLS);
    printf("  CPU_INT = 0x%08lX\n", (unsigned long)S2A_UART_INST->CPU_INT.IMASK);
    
    // ピン設定確認（IOMUX PINCM21=PA10, PINCM22=PA11）
    printf("\n[PRE-TEST] Pin Configuration Check:\n");
    volatile uint32_t *pincm21 = (volatile uint32_t *)0x40428054;  // IOMUX PINCM21 (PA10)
    volatile uint32_t *pincm22 = (volatile uint32_t *)0x40428058;  // IOMUX PINCM22 (PA11)
    printf("  IOMUX_PINCM21 address = 0x%08lX\n", (unsigned long)IOMUX_PINCM21);
    printf("  IOMUX_PINCM21_PF_UART0_TX value = 0x%08lX\n", (unsigned long)IOMUX_PINCM21_PF_UART0_TX);
    printf("  IOMUX_PINCM22_PF_UART0_RX value = 0x%08lX\n", (unsigned long)IOMUX_PINCM22_PF_UART0_RX);
    printf("  PINCM21 (PA10/TX) = 0x%08lX\n", (unsigned long)*pincm21);
    printf("  PINCM22 (PA11/RX) = 0x%08lX\n", (unsigned long)*pincm22);
    printf("  Expected: bit[3:0]=PF (should be 0x2 for UART0)\n");
    printf("  Expected: bit[8]=INENA (should be 1 for RX)\n");
    
    // NVIC状態確認
    printf("\n[PRE-TEST] NVIC Check:\n");
    printf("  UART0_IRQn = %d\n", S2A_UART_IRQ);
    printf("  NVIC Enabled = %d\n", NVIC_GetEnableIRQ(S2A_UART_IRQ));
    printf("  NVIC Priority = %lu\n", (unsigned long)NVIC_GetPriority(S2A_UART_IRQ));
    printf("  NVIC Pending = %d\n", NVIC_GetPendingIRQ(S2A_UART_IRQ));
    
    // デバッグカウンタリセット
    debug_rx_count = 0;
    debug_frame_count = 0;
    debug_sync_reset_count = 0;
    debug_complete_count = 0;
    debug_last_byte = 0;
    debug_rx_index = 0;
    
    // テストパターン準備（36バイトフレーム）
    uint8_t test_tx[AXON_FRAME_SIZE];
    uint8_t test_rx[AXON_FRAME_SIZE];
    
    // テストデータ生成: Header(0x14) + Length(0x20) + Data(32) + CRC(2)
    test_tx[0] = 0x14;  // Header
    test_tx[1] = 0x20;  // Length
    for (int i = 2; i < 34; i++) {
        test_tx[i] = (uint8_t)(i - 2);  // 0x00~0x1F
    }
    // ★仕様書準拠: Data部のみ（32バイト）をCRC計算
    uint16_t crc = crc16_tep(&test_tx[2], 32);  // Byte[2-33]
    test_tx[34] = crc & 0xFF;        // CRC LSB
    test_tx[35] = (crc >> 8) & 0xFF; // CRC MSB
    
    // 受信バッファクリア
    memset(test_rx, 0, AXON_FRAME_SIZE);
    rx_index = 0;
    frame_received = 0;
    memset(rx_frame, 0, AXON_FRAME_SIZE);
    
    printf("Test Pattern (36 bytes):\n");
    for (int i = 0; i < AXON_FRAME_SIZE; i++) {
        printf("%02X ", test_tx[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (AXON_FRAME_SIZE % 16 != 0) printf("\n");
    
    printf("\nSending data...\n");
    
    // データ送信（1バイトずつ確実に）
    uint16_t timeout = 50;
    for (int i = 0; i < AXON_FRAME_SIZE; i++) {
        // TX FIFO空き待ち
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST)) {
            __NOP();
            timeout--;
            if (timeout <= 0)   break;
        }
        
        DL_UART_transmitData(S2A_UART_INST, test_tx[i]);
        
        // 送信完了待ち
        timeout = 10000;
        while (DL_UART_isBusy(S2A_UART_INST)) {
            __NOP();
            timeout--;
            if (timeout <= 0)   break;
        }
        
        // バイト間ディレイ
        for (volatile int j = 0; j < 100; j++);
    }
    
    printf("Transmission completed.\n");
    printf("Waiting for reception...\n");
    
    // 受信待機（ISRでframe_receivedフラグがセットされるまで待つ）
    uint32_t wait_count = 0;
    for (volatile int i = 0; i < 10000000; i++) {
        wait_count++;
        // ★修正: rx_indexではなくframe_receivedで判定（ISRが完了をセット）
        if (frame_received) {
            break;
        }
        if (wait_count % 1000000 == 0) {
            printf("  Waiting... rx_index=%d, debug_rx_count=%lu, frame_received=%d\n", 
                   rx_index, (unsigned long)debug_rx_count, frame_received);
        }
    }
    
    // デバッグ情報表示
    printf("\n[POST-RX] Debug Info:\n");
    printf("  debug_isr_call_count = %lu (ISR invocations)\n", (unsigned long)debug_isr_call_count);
    printf("  debug_rx_count = %lu (total bytes received in ISR)\n", (unsigned long)debug_rx_count);
    printf("  debug_frame_count = %lu\n", (unsigned long)debug_frame_count);
    printf("  debug_complete_count = %lu\n", (unsigned long)debug_complete_count);
    printf("  debug_iidx_value = 0x%08lX (last interrupt index)\n", (unsigned long)debug_iidx_value);
    printf("  debug_last_byte = 0x%02X\n", debug_last_byte);
    printf("  debug_rx_index = %d\n", debug_rx_index);
    printf("  rx_index = %d\n", rx_index);
    printf("  frame_received = %d\n", frame_received);
    printf("  rx_complete_ready = %d\n", rx_complete_ready);
    
    // RXFIFOの状態確認
    printf("\n[POST-RX] UART Status:\n");
    printf("  STAT = 0x%08lX\n", (unsigned long)S2A_UART_INST->STAT);
    printf("  RX FIFO Empty = %d\n", DL_UART_isRXFIFOEmpty(S2A_UART_INST));
    printf("  CPU_INT.RIS = 0x%08lX\n", (unsigned long)S2A_UART_INST->CPU_INT.RIS);
    printf("  CPU_INT.MIS = 0x%08lX\n", (unsigned long)S2A_UART_INST->CPU_INT.MIS);
    
    // ★重要: ISRが退避したフレームデータをコピー（rx_frameは次の受信で上書きされる可能性）
    if (rx_complete_ready) {
        memcpy(test_rx, rx_complete_frame, AXON_FRAME_SIZE);
        rx_complete_ready = 0;  // クリア
    } else {
        memcpy(test_rx, rx_frame, AXON_FRAME_SIZE);
    }
    int received_bytes = (debug_rx_count >= AXON_FRAME_SIZE) ? AXON_FRAME_SIZE : debug_rx_count;
    
    printf("\nReceived %d bytes:\n", received_bytes);
    for (int i = 0; i < received_bytes; i++) {
        printf("%02X ", test_rx[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (received_bytes % 16 != 0) printf("\n");
    
    // 結果検証
    bool test_passed = true;
    
    if (received_bytes != AXON_FRAME_SIZE) {
        printf("\n[FAIL] Byte count mismatch: expected %d, got %d\n",
               AXON_FRAME_SIZE, received_bytes);
        test_passed = false;
    } else {
        // データ比較
        int mismatch_count = 0;
        for (int i = 0; i < AXON_FRAME_SIZE; i++) {
            if (test_tx[i] != test_rx[i]) {
                if (mismatch_count == 0) {
                    printf("\n[FAIL] Data mismatch:\n");
                }
                printf("  Byte[%d]: TX=0x%02X, RX=0x%02X\n",
                       i, test_tx[i], test_rx[i]);
                mismatch_count++;
                test_passed = false;
            }
        }
        
        if (test_passed) {
            printf("\n[PASS] All %d bytes matched!\n", AXON_FRAME_SIZE);
        } else {
            printf("\n[FAIL] %d bytes mismatched\n", mismatch_count);
        }
    }
    
    // LED表示（緑=成功、赤=失敗）
    if (test_passed) {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_1);  // 緑LED
        printf("\n✓ LOOPBACK TEST PASSED\n");
    } else {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);  // 赤LED
        printf("\n✗ LOOPBACK TEST FAILED\n");
    }
    
    printf("========================================\n\n");
    
    return test_passed;
}

/// @brief 36バイトフレーム完全検証テスト（CRC・フレーム同期・ISR処理含む）
/// @return true=成功, false=失敗
bool axon_36byte_frame_test(void) {
    printf("\n========================================\n");
    printf("36-BYTE FRAME TEST (with CRC validation)\n");
    printf("========================================\n");
    
    // 外部宣言：ISRが退避したフレームデータ（soma_uart_test.hで宣言済み）
    
    // デバッグカウンタリセット
    debug_rx_count = 0;
    debug_frame_count = 0;
    debug_sync_reset_count = 0;
    debug_complete_count = 0;
    debug_last_byte = 0;
    debug_rx_index = 0;
    debug_byte1 = 0;
    debug_byte1_ng_count = 0;
    
    // 受信バッファクリア
    rx_index = 0;
    frame_received = 0;
    rx_complete_ready = 0;
    memset(rx_frame, 0, AXON_FRAME_SIZE);
    memset(rx_complete_frame, 0, AXON_FRAME_SIZE);
    
    // テストフレーム作成
    uint8_t test_frame[AXON_FRAME_SIZE];
    test_frame[0] = 0x14;  // Header
    test_frame[1] = 0x20;  // Length (32 bytes)
    
    // データ部：シンプルなパターン (0x00~0x1F)
    for (int i = 2; i < 34; i++) {
        test_frame[i] = (uint8_t)(i - 2);
    }
    
    // ★仕様書準拠: Data部のみ（32バイト）をCRC計算
    uint16_t crc_calc = crc16_tep(&test_frame[2], 32);  // Byte[2-33]
    test_frame[34] = crc_calc & 0xFF;         // CRC LSB
    test_frame[35] = (crc_calc >> 8) & 0xFF;  // CRC MSB
    
    printf("\n[TEST FRAME] 36 bytes:\n");
    printf("  Header: 0x%02X 0x%02X\n", test_frame[0], test_frame[1]);
    printf("  Data[0-31]: ");
    for (int i = 2; i < 34; i++) {
        printf("%02X ", test_frame[i]);
        if ((i - 2 + 1) % 16 == 0) printf("\n              ");
    }
    printf("\n  CRC16: 0x%02X%02X (calculated=0x%04X)\n",
           test_frame[35], test_frame[34], crc_calc);
    
    // LED消灯（テスト開始）
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);
    
    printf("\n[SENDING] 36 bytes via UART0 loopback...\n");
    
    // 送信（1バイトずつ確実に）
    for (int i = 0; i < AXON_FRAME_SIZE; i++) {
        // TX FIFO空き待ち
        uint16_t timeout = 10000;
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST)) {
            if (--timeout == 0) {
                printf("[ERROR] TX timeout at byte %d\n", i);
                return false;
            }
        }
        
        DL_UART_transmitData(S2A_UART_INST, test_frame[i]);
        
        // 送信完了待ち
        timeout = 10000;
        while (DL_UART_isBusy(S2A_UART_INST)) {
            if (--timeout == 0) {
                printf("[ERROR] BUSY timeout at byte %d\n", i);
                return false;
            }
        }
        
        // バイト間ディレイ（フレーム同期確認用）
        for (volatile int d = 0; d < 200; d++);
    }
    
    printf("[SENT] Transmission completed.\n");
    
    // 受信待機（ISRで処理される）
    printf("\n[WAITING] Receiving via interrupt...\n");
    uint32_t wait_loops = 0;
    const uint32_t MAX_WAIT = 5000000;
    
    while (wait_loops < MAX_WAIT) {
        wait_loops++;
        
        // 定期的に状態表示
        if (wait_loops % 1000000 == 0) {
            printf("  [%u] rx_index=%d, rx_count=%lu, complete=%lu, ready=%d\n",
                   (unsigned int)(wait_loops / 1000000),
                   rx_index,
                   (unsigned long)debug_rx_count,
                   (unsigned long)debug_complete_count,
                   rx_complete_ready);
        }
        
        // 受信完了検出
        if (frame_received && rx_complete_ready) {
            break;
        }
    }
    
    printf("[RECEIVED] Wait completed.\n");
    
    // デバッグ情報表示
    printf("\n[DEBUG INFO]\n");
    printf("  debug_rx_count        = %lu (total bytes in ISR)\n", (unsigned long)debug_rx_count);
    printf("  debug_complete_count  = %lu (36-byte frames completed)\n", (unsigned long)debug_complete_count);
    printf("  debug_sync_reset_count= %lu (header sync resets)\n", (unsigned long)debug_sync_reset_count);
    printf("  debug_byte1_ng_count  = %lu (2nd byte != 0x20)\n", (unsigned long)debug_byte1_ng_count);
    printf("  debug_last_byte       = 0x%02X\n", debug_last_byte);
    printf("  rx_index              = %d\n", rx_index);
    printf("  frame_received        = %d\n", frame_received);
    printf("  rx_complete_ready     = %d\n", rx_complete_ready);
    
    // 結果検証
    bool test_passed = true;
    
    if (!frame_received) {
        printf("\n[FAIL] Frame not received (frame_received=0)\n");
        test_passed = false;
    } else if (!rx_complete_ready) {
        printf("\n[FAIL] Complete frame not ready (rx_complete_ready=0)\n");
        test_passed = false;
    } else if (debug_rx_count != AXON_FRAME_SIZE) {
        printf("\n[FAIL] Byte count mismatch: expected %d, got %lu\n",
               AXON_FRAME_SIZE, (unsigned long)debug_rx_count);
        test_passed = false;
    } else {
        // ISRが退避したフレームをコピー
        uint8_t received_frame[AXON_FRAME_SIZE];
        memcpy(received_frame, rx_complete_frame, AXON_FRAME_SIZE);
        rx_complete_ready = 0;  // クリア
        frame_received = 0;
        
        printf("\n[RECEIVED DATA] 36 bytes:\n");
        for (int i = 0; i < AXON_FRAME_SIZE; i++) {
            printf("%02X ", received_frame[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (AXON_FRAME_SIZE % 16 != 0) printf("\n");
        
        // ヘッダー検証
        if (received_frame[0] != 0x14) {
            printf("\n[FAIL] Header byte[0]: expected 0x14, got 0x%02X\n", received_frame[0]);
            test_passed = false;
        }
        if (received_frame[1] != 0x20) {
            printf("\n[FAIL] Length byte[1]: expected 0x20, got 0x%02X\n", received_frame[1]);
            test_passed = false;
        }
        
        // データ部検証
        int data_errors = 0;
        for (int i = 2; i < 34; i++) {
            if (received_frame[i] != test_frame[i]) {
                if (data_errors == 0) {
                    printf("\n[FAIL] Data mismatch:\n");
                }
                printf("  Byte[%d]: expected 0x%02X, got 0x%02X\n",
                       i, test_frame[i], received_frame[i]);
                data_errors++;
            }
        }
        if (data_errors > 0) {
            test_passed = false;
        }
        
        // CRC検証
        uint16_t crc_recv = received_frame[34] | (received_frame[35] << 8);
        // ★仕様書準拠: Data部のみ（32バイト）をCRC計算
        uint16_t crc_calc_rx = crc16_tep(&received_frame[2], 32);  // Byte[2-33]
        
        printf("\n[CRC VALIDATION]\n");
        printf("  Received CRC : 0x%04X\n", crc_recv);
        printf("  Calculated   : 0x%04X\n", crc_calc_rx);
        
        if (crc_recv != crc_calc_rx) {
            printf("  Result: FAILED (mismatch)\n");
            test_passed = false;
        } else {
            printf("  Result: PASSED (match)\n");
        }
        
        // 全体結果
        if (test_passed) {
            printf("\n[SUCCESS] All validations passed!\n");
            printf("  ✓ Header: 0x14 0x20\n");
            printf("  ✓ Data: 32 bytes correct\n");
            printf("  ✓ CRC16: valid\n");
            printf("  ✓ Frame sync: working\n");
            printf("  ✓ ISR buffering: working\n");
        }
    }
    
    // LED表示
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);
    if (test_passed) {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_1);  // 緑LED
        printf("\n✓ 36-BYTE FRAME TEST PASSED\n");
    } else {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);  // 赤LED
        printf("\n✗ 36-BYTE FRAME TEST FAILED\n");
    }
    
    printf("========================================\n\n");
    
    return test_passed;
}
