#ifndef __MSP_FRAM_UTILS_H__
#define __MSP_FRAM_UTILS_H__

#ifdef SOMA_BOARD

#include "msp_peripheral_config.h"
#include "peripheral_typedef.h"
#include "peripheral/fram_memory_map.h"

// EXTERNマクロ
#ifdef DEFINE_GLOBALS
    #define EXTERN
#else
    #define EXTERN extern
#endif

#define WREN 0x06  // Write Enable
#define WRDI 0x04  // Write Disable
#define RDSR 0x05  // Read Status Register
#define WRSR 0x01  // Write Status Register
#define READ 0x03  // Read Data
#define WRITE 0x02 // Write Data
#define FRAM_SIZE 0x4000 // 16KByte

EXTERN volatile uint8_t gRxData;                         // SPI受信データ
EXTERN volatile uint8_t gTxData;                         // SPI送信データ
EXTERN volatile bool gSpiRxCompFlg;                      // SPI受信完了割り込み判定フラグ

// タイムアウト値
EXTERN volatile uint32_t fram_timeout;         // SPI通信タイムアウト値（約60～150μs）


// ==============================================================================
// fram_memory_map.c関数のラッピング FRAMメモリマップAPI
// ==============================================================================

/**
 * @brief FRAMメモリマップAPI初期化関数
 */
fram_error_t fram_memory_init(void);

/**
 * @brief AXON基板シリアル番号操作関数
 */
fram_error_t fram_write_axon_serial(uint8_t board_num, const uint8_t* serial_data);
fram_error_t fram_read_axon_serial(uint8_t board_num, uint8_t* serial_data);

/**
 * @brief 現金カウンタ操作関数
 */
fram_error_t fram_write_cash_counter(uint8_t board_num, uint16_t counter_value);
fram_error_t fram_read_cash_counter(uint8_t board_num, uint16_t* counter_value);

/**
 * @brief プライズカウンタ操作関数
 */
fram_error_t fram_write_prize_counter(uint8_t board_num, uint16_t counter_value);
fram_error_t fram_read_prize_counter(uint8_t board_num, uint16_t* counter_value);

/**
 * @brief ユーザーメモリ操作関数
 */
fram_error_t fram_write_user_memory(const uint8_t* user_data);
fram_error_t fram_read_user_memory(uint8_t* user_data);

/**
 * @brief AXON基板データ一括操作関数
 */
fram_error_t fram_read_axon_board_data(uint8_t board_num, fram_axon_data_t* axon_data);
fram_error_t fram_write_axon_board_data(uint8_t board_num, const fram_axon_data_t* axon_data);

/**
 * @brief カウンタインクリメント関数
 */
fram_error_t fram_increment_counter(uint8_t board_num, bool is_cash_counter);

/**
 * @brief ユーティリティ関数
 */
const char* fram_error_to_string(fram_error_t error);


// ==============================================================================
// FRAMアクセス関数（SPI割り込みベース）
// ==============================================================================
/**
 * @brief FRAMのチップセレクトピンをLOWに設定
 */
static inline void set_fram_cs_Low(void) {
    DL_GPIO_clearPins(FRAM_SPI_PORT, FRAM_SPI_CS_PIN);
}

/**
 * @brief FRAMのチップセレクトピンをHIGHに設定
 */
static inline void set_fram_cs_High(void) {
    DL_GPIO_setPins(FRAM_SPI_PORT, FRAM_SPI_CS_PIN);
}

/**
 * @brief FRAMのステータスレジスタを読み取る
 * @return ステータスレジスタの値（エラー時は0xFF）
 * @note 割り込みを使用せず、ポーリングで受信データを取得
 */
static inline uint8_t fram_read_status_register(void) {
    uint8_t status = 0xFF;
    
    set_fram_cs_Low();
    
    // 1) RDSRコマンド送信（割り込みで受信）
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;    // フラグクリア
    DL_SPI_transmitData8(FRAM_SPI_INST, RDSR);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    
    while (!gSpiRxCompFlg && --fram_timeout > 0) {
         // 割り込み完了待ち 
    }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;  // タイムアウトエラー
    }
    
    // 一時的にSPI RX割り込みを無効化（送信開始の原子性保証）
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    
    gRxData = 0xFF;           // 受信データ初期化
    gSpiRxCompFlg = false;    // フラグクリア
    fram_timeout = 1000;           // タイムアウト値リセット（1000ループ ≈ 40～80μs）
    
    // 送信開始してから割り込み再有効化
    DL_SPI_transmitData8(FRAM_SPI_INST, 0x00);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    
    // 割り込みハンドラでフラグが立つまで待つ（経過時間測定）
    while (!gSpiRxCompFlg && --fram_timeout > 0) {
         // 割り込み完了待ち 
    }
    
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;  // タイムアウトエラー
    }
    
    status = gRxData;  // 割り込みで格納されたステータスを取得
    set_fram_cs_High();
    return status;
}

/**
 * @brief FRAMの書き込みを有効にする
 * @return 0（成功）
 */
static inline uint8_t fram_write_enable(void) {
    fram_timeout = 1000;
    set_fram_cs_Low();
    
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;    // フラグクリア
    DL_SPI_transmitData8(FRAM_SPI_INST, WREN);    // WRENコマンド送信(0x06)
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    
    while (!gSpiRxCompFlg && --fram_timeout > 0) {
         // 割り込み完了待ち 
    }
    set_fram_cs_High();
    return 0;
}

static inline uint8_t fram_write_disable(void) {
    fram_timeout = 1000;
    set_fram_cs_Low();
    
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;    // フラグクリア
    DL_SPI_transmitData8(FRAM_SPI_INST, WRDI);    // WRDIコマンド送信(0x04)
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    
    while (!gSpiRxCompFlg && --fram_timeout > 0) {
         // 割り込み完了待ち
    }
    set_fram_cs_High();
    return 0;
}

/**
 * @brief FRAMにデータを書き込む（割り込みベース）
 * @param address 書き込み先アドレス（0～0x3FFF）
 * @param data 書き込むデータのポインタ
 * @param length 書き込むデータ長
 * @return 0（成功）、0xFF（エラー）
 * @note SPI割り込みを使用してデータ送信完了を待機
 */
static inline uint8_t fram_write(uint16_t address, uint8_t* data, uint16_t length) {
    if (address + length > FRAM_SIZE) {
        return 0xFF;  // アドレス範囲外エラー
    }
    
    // 書き込み有効化
    fram_write_enable();
    
    // WRITEコマンド（0x02）+ 16bitアドレス + データ
    set_fram_cs_Low();
    
    // WRITEコマンド送信
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, WRITE);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }
    
    // アドレス送信（上位バイト）
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, (address >> 8) & 0xFF);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }

    // アドレス送信（下位バイト）
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, address & 0xFF);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }

    // データ送信（割り込み待機）
    for (uint16_t i = 0; i < length; i++) {
        fram_timeout = 1000;
        DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
        gSpiRxCompFlg = false;
        DL_SPI_transmitData8(FRAM_SPI_INST, data[i]);
        DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
        while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
        if (fram_timeout <= 0) {
            set_fram_cs_High();
            return 0xFF;
        }
    }
    
    set_fram_cs_High();
    
    // 書き込み無効化
    fram_write_disable();
    
    return 0;
}

/**
 * @brief FRAMからデータを読み込む（割り込みベース）
 * @param address 読み込み元アドレス（0～0x3FFF）
 * @param data 読み込んだデータを格納するバッファのポインタ
 * @param length 読み込むデータ長
 * @return 0（成功）、0xFF（エラー）
 * @note SPI割り込みを使用してデータ受信完了を待機
 */
static inline uint8_t fram_read(uint16_t address, uint8_t* data, uint16_t length) {
    if (address + length > FRAM_SIZE) {
        return 0xFF;  // アドレス範囲外エラー
    }
    
    // READコマンド（0x03）+ 16bitアドレス + データ受信
    set_fram_cs_Low();
    
    // READコマンド送信
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, READ);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }
    
    // アドレス送信（上位バイト）
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, (address >> 8) & 0xFF);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }
    
    // アドレス送信（下位バイト）
    fram_timeout = 1000;
    DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    gSpiRxCompFlg = false;
    DL_SPI_transmitData8(FRAM_SPI_INST, address & 0xFF);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
    while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }
    if (fram_timeout <= 0) {
        set_fram_cs_High();
        return 0xFF;
    }
    
    // データ受信（割り込み待機 + gRxDataから取得）
    for (uint16_t i = 0; i < length; i++) {
        fram_timeout = 1000;
        DL_SPI_disableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
        gSpiRxCompFlg = false;
        gRxData = 0xFF;  // 受信データ初期化
        DL_SPI_transmitData8(FRAM_SPI_INST, 0x00);  // ダミー送信
        DL_SPI_enableInterrupt(FRAM_SPI_INST, DL_SPI_INTERRUPT_RX);
        
        while (!gSpiRxCompFlg && --fram_timeout > 0) { /* 割り込み完了待ち */ }

        if (fram_timeout <= 0) {
            set_fram_cs_High();
            return 0xFF;
        }
        
        // 割り込みハンドラで gRxData に格納されたデータを取得
        data[i] = gRxData;
    }
    
    set_fram_cs_High();
    
    return 0;
}








#ifdef _TEST_CODE_  // for debug

/**
 * @brief FRAMの基本機能テスト（書き込み・読み込み・ステータス確認）
 * @return true（成功）、false（失敗）
 * @note 14バイトのテストデータを使用してFRAMの動作を確認
 */

 // FRAM test variables
uint8_t read_data[14] = {0};
uint8_t write_data[14] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
bool fram_test_result = false;

static inline bool fram_basic_test(uint8_t* write_data, uint8_t* read_data) {
    bool fram_test_result = false;
    
    // read_data clear
    for (int i = 0; i < 14; i++) {
        read_data[i] = 0;
    }
    
    // FRAMにデータ書き込み（アドレス0から14バイト）
    if (fram_write(0, write_data, 14) == 0) {
        // 書き込み成功
        delay_cycles(CPUCLK_FREQ / 1000);  // 1ms wait
        
        // FRAMからデータ読み込み（アドレス0から14バイト）
        if (fram_read(0, read_data, 14) == 0) {
            // 読み込み成功 - データ比較
            fram_test_result = true;
            for (int i = 0; i < 14; i++) {
                if (write_data[i] != read_data[i]) {
                    fram_test_result = false;
                    break;
                }
            }
        }
    }
    
    // テスト結果をLEDで表示
    if (fram_test_result) {
        // 成功: LED7を点灯
        DL_GPIO_setPins(LED_PORT, LED_PORT_7_PIN);
    } else {
        // 失敗: LED7を消灯
        DL_GPIO_clearPins(LED_PORT, LED_PORT_7_PIN);
    }
    
    // Write Enable
    fram_write_enable();

    // Read Status Register
    uint8_t status = fram_read_status_register();
    if ((status & 0x02) == 0x02) {          // 2bit目をマスクして確認
        // 正常
        DL_GPIO_togglePins(LED_PORT, LED_PORT_1_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_2_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_3_PIN);
    } else {
        // エラー処理
        DL_GPIO_clearPins(LED_PORT, LED_PORT_1_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_2_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_3_PIN);
    }

    // Write Disable
    fram_write_disable();
    
    // Read Status Register
    fram_read_status_register();
    
    return fram_test_result;
}

/**
 * @brief FRAM全アドレステスト（0x55パターン書き込み・読み込み）
 * @return true（成功）、false（失敗）
 * @note 全16KBのアドレス範囲に0x55を書き込み、正常に読み出せるかテスト
 */
static inline bool fram_full_address_test(void) {
    uint8_t write_value = 0x55;
    uint8_t read_value = 0;
    bool test_passed = true;

    // 全アドレスに0x55を書き込む
    for (uint16_t addr = 0; addr < FRAM_SIZE; addr++) {
        if (fram_write(addr, &write_value, 1) != 0) {
            test_passed = false;
            break;
        }
    }

    // 書き込み後、全アドレスを読み込んで確認
    if (test_passed) {
        for (uint16_t addr = 0; addr < FRAM_SIZE; addr++) {
            if (fram_read(addr, &read_value, 1) != 0 || read_value != write_value) {
                test_passed = false;
                break;
            }
        }
    }

    // テスト結果をLEDで表示
    if (test_passed) {
        // 成功: LED1-3を点灯
        DL_GPIO_togglePins(LED_PORT, LED_PORT_1_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_2_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_3_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_4_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_5_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_6_PIN);
    } else {
        // 失敗: LED4-6を点灯
        DL_GPIO_clearPins(LED_PORT, LED_PORT_1_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_2_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_PORT_3_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_4_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_5_PIN);
        DL_GPIO_togglePins(LED_PORT, LED_PORT_6_PIN);
    }
    
    return test_passed;
}

/**
 * @brief FRAMステータスレジスタ取得テスト（低レベルSPI使用）
 * @return 取得したステータスレジスタの値（0x00、0x02、その他）
 * @note 低レベルSPI関数を使用してWREN設定後にステータスレジスタを読み取り
 */
static inline uint8_t fram_status_register_test(void) {
    uint8_t sr = 0x00;
    
    // WRENを設定（WRENセット）
    {
        uint8_t tx_wren = WREN;  // 0x06
        set_fram_cs_Low();
        DL_SPI_fillTXFIFO8(FRAM_SPI_INST, &tx_wren, 1);
        delay_cycles(CPUCLK_FREQ / 10000);  // 100us wait
        while (DL_SPI_isBusy(FRAM_SPI_INST)) { /* wait */ }
        set_fram_cs_High();
    }

    // --- RDSR 読出し（0x05→ダミー0x00 で1バイト受信）---
    set_fram_cs_Low();

    // 1) オペコード送信
    {
        uint8_t op = RDSR; // 0x05
        DL_SPI_fillTXFIFO8(FRAM_SPI_INST, &op, 1);
        delay_cycles(CPUCLK_FREQ / 10000);  // 100us wait
        while (DL_SPI_isBusy(FRAM_SPI_INST)) { /* wait */ }

        // オペコード送信後のRX FIFOクリア
        if (!DL_SPI_isRXFIFOEmpty(FRAM_SPI_INST)) {
            (void)DL_SPI_receiveDataBlocking8(FRAM_SPI_INST);
        }
    }

    // ダミー送信しながらSR受信
    {
        uint8_t dummy = 0x00;
        DL_SPI_fillTXFIFO8(FRAM_SPI_INST, &dummy, 1);
        delay_cycles(CPUCLK_FREQ / 10000);  // 100us wait
        while (DL_SPI_isBusy(FRAM_SPI_INST)) {}

        while (DL_SPI_isRXFIFOEmpty(FRAM_SPI_INST)) {}
        sr = DL_SPI_receiveDataBlocking8(FRAM_SPI_INST);

        // ステータスレジスタの値に応じたLED表示
        if (sr == 0x02)
        {
            // WREN=1 (Write Enable成功)
            DL_GPIO_togglePins(LED_PORT, LED_PORT_1_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_2_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_3_PIN);
            DL_GPIO_clearPins(LED_PORT, LED_PORT_4_PIN);
        }
        else if (sr == 0x00)
        {
            // WREN=0 (Write Disable状態)
            DL_GPIO_togglePins(LED_PORT, LED_PORT_1_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_2_PIN);
            DL_GPIO_clearPins(LED_PORT, LED_PORT_3_PIN);
            DL_GPIO_clearPins(LED_PORT, LED_PORT_4_PIN);
        }
        else {
            // 予期しない値
            DL_GPIO_togglePins(LED_PORT, LED_PORT_5_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_6_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_7_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_8_PIN);
            DL_GPIO_togglePins(LED_PORT, LED_PORT_9_PIN);
        }
    }
    set_fram_cs_High();
    
    return sr;
}
#endif // _TEST_CODE_


// ==============================================================================
// 使用例 - fram_memory_map.cの高レベルAPIラッピング
// ==============================================================================
/*
// 初期化
fram_error_t result = fram_memory_init();
if (result != FRAM_ERROR_SUCCESS) {
    // エラー処理
}

// AXON基板のシリアル番号読み書き
uint8_t serial[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
fram_write_axon_serial(0, serial);  // 基板0にシリアル番号書き込み

uint8_t read_serial[10];
fram_read_axon_serial(0, read_serial);  // 基板0のシリアル番号読み込み

// カウンタ操作
fram_write_cash_counter(0, 100);        // 基板0の現金カウンタに100を設定
fram_increment_counter(0, true);        // 基板0の現金カウンタをインクリメント

uint16_t cash_counter;
fram_read_cash_counter(0, &cash_counter);  // 現在値は101

// 高速キャッシュアクセス（SPI通信なし）
fram_cache_read_cash_counter(0, &cash_counter);
*/

#endif /* __MSP_FRAM_UTILS_H__ */

#endif // SOMA_BOARD