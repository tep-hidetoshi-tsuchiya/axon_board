#include "soma_uart_test.h"
#include <stdint.h>
#include <string.h>
#include "ti_msp_dl_config.h"
#include "driver_config.h"

#define AXON_FRAME_SIZE 36

uint8_t rx_frame[AXON_FRAME_SIZE];
uint8_t decrypted_data[AES_DATA_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t frame_received = 0;

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
volatile uint32_t debug_rx_count = 0;
volatile uint32_t debug_frame_count = 0;
volatile uint32_t debug_sync_reset_count = 0;
volatile uint32_t debug_complete_count = 0;
volatile uint8_t debug_last_byte = 0;
volatile uint8_t debug_rx_index = 0;
volatile uint8_t debug_byte1 = 0;
volatile uint32_t debug_byte1_ng_count = 0;
#endif

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
    
    return crc;  // 最終XORなし
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
 
    // UART0のRXFIFOをクリア（リセットは行わない）
    // 起動直後に途中のデータが残っている可能性があるため、十分にクリア
    for (volatile int i = 0; i < 100; i++) {
        while (!DL_UART_isRXFIFOEmpty(S2A_UART_INST)) {
            DL_UART_receiveData(S2A_UART_INST);
        }
        for (volatile int j = 0; j < 1000; j++);  // 少し待機
    }
    
    // UART0の受信割り込みを明示的に有効化
    // FIFOトリガーレベル: 1バイト受信で割り込み発生
    DL_UART_setRXFIFOThreshold(S2A_UART_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    
    // 全ての割り込みをクリアしてからRX割り込みのみ有効化
    DL_UART_clearInterruptStatus(S2A_UART_INST, 0xFFFFFFFF);
    DL_UART_enableInterrupt(S2A_UART_INST, DL_UART_INTERRUPT_RX);
    
    // NVIC設定: UART0割り込みを最高優先度に設定
    NVIC_DisableIRQ(S2A_UART_IRQ);
    NVIC_ClearPendingIRQ(S2A_UART_IRQ);
    NVIC_SetPriority(S2A_UART_IRQ, 0);  // 最高優先度
    NVIC_EnableIRQ(S2A_UART_IRQ);
    
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
    
    // 暗号化されたByte[0〜33]に対してCRC計算
    uint16_t crc_calc = crc16_tep(rx_frame, 34);

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
