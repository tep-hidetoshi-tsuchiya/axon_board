#ifndef __SOMA_UART_TEST_H__
#define __SOMA_UART_TEST_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // size_t定義用

#define AXON_FRAME_SIZE 36
#define AXON_MAX_FRAME_SIZE 40  // CODEPKT用の最大サイズ
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16
#define AES_DATA_SIZE 32
#define ACK_NACK_FRAME_SIZE 6

// ACK/NACKコマンド定義（仕様書準拠）
#define CMD_RESPONSE_TYPE 0x01
#define CMD_ACK 0x01
#define CMD_NACK 0x02
#define FRAME_HEADER 0x14
#define ACK_NACK_LENGTH 0x04

/// @brief SOMAからの36バイトフレーム受信バッファ
extern uint8_t rx_frame[AXON_FRAME_SIZE];

/// @brief AES復号後のデータバッファ（32バイト）
extern uint8_t decrypted_data[AES_DATA_SIZE];

/// @brief 受信インデックス
extern volatile uint8_t rx_index;

/// @brief フレーム受信完了フラグ
extern volatile uint8_t frame_received;

// デバッグ用変数（Expressionsウィンドウで確認可能）
#ifdef AXON_BOARD
extern volatile uint32_t debug_rx_count;         // 受信バイト総数
extern volatile uint32_t debug_frame_count;      // 受信フレーム数
extern volatile uint32_t debug_sync_reset_count; // ヘッダー同期リセット回数
extern volatile uint32_t debug_complete_count;   // 36バイト完了回数
extern volatile uint8_t debug_last_byte;         // 最後に受信したバイト
extern volatile uint8_t debug_rx_index;          // 現在のrx_index
extern volatile uint8_t debug_byte1;             // 2バイト目の値（0x20のはず）
extern volatile uint32_t debug_byte1_ng_count;   // 2バイト目が0x20でない回数

// ISR→メイン受け渡しバッファ（ダブルバッファリング用）
extern uint8_t rx_complete_frame[AXON_FRAME_SIZE];
extern volatile uint8_t rx_complete_ready;

// 可変長フレーム受信バッファ（SETOKEY/CODEPKT/ERRCHK用）
extern uint8_t rx_variable_frame[AXON_MAX_FRAME_SIZE];
extern volatile uint8_t rx_variable_ready;
extern volatile uint8_t rx_variable_length;  // 受信したフレームの実際の長さ
#endif

/// @brief CRC16-CCITT (ISO/IEC 13239) LSB-first計算関数
/// @details 
///   多項式: X^16 + X^12 + X^5 + 1
///   通常形式(MSB): 0x1021
///   反転形式(LSB): 0x8408
///   初期値: 0xFFFF
///   最終XOR: なし
///   入出力: LSB-first (reflected)
/// @param data データバッファ
/// @param len データ長
/// @return CRC16値（リトルエンディアン）
uint16_t crc16_tep(const uint8_t* data, int len);

/// @brief SOMA UART初期化（36バイトフレーム/AES暗号化通信用）
/// @warning この関数は通常のSOMA-AXON間通信（3バイトパケット、0xFF header）とは
///          互換性がありません。デバッグ/テスト専用です。
/// @note 本番動作では init_uart_ports() で初期化されたUARTドライバを使用してください。
void soma_uart_init(void);

/// @brief AES-128-CBC復号処理
/// @param encrypted_data 暗号化されたデータ（32バイト）
/// @param decrypted_data 復号後のデータ格納先（32バイト）
/// @return true: 成功, false: 失敗
bool aes_decrypt_cbc(const uint8_t* encrypted_data, uint8_t* decrypted_data);

/// @brief 任意のパケットをUART送信（汎用関数）
/// @param data 送信データバッファ
/// @param len 送信データ長
/// @return true: 成功, false: 失敗
bool uart_send_packet(const uint8_t* data, size_t len);

/// @brief ACKフレーム送信
/// @return true: 成功, false: 失敗
bool send_ack_frame(void);

/// @brief NACKフレーム送信
/// @return true: 成功, false: 失敗
bool send_nack_frame(void);

/// @brief フレームのチェックと処理（AES復号対応）
void soma_check_frame(void);

/// @brief AXONボード内UARTループバックテスト
/// @details PA10(TX)とPA11(RX)を物理的に接続して実行
///          36バイトのテストパターンを送信し、受信データと比較
/// @return true: テスト成功, false: テスト失敗
bool axon_uart_loopback_test(void);

/// @brief 36バイトフレーム完全検証テスト（CRC・フレーム同期・ISR処理含む）
/// @details ヘッダー(0x14 0x20) + データ32バイト + CRC16の完全なフレームテスト
///          ISRのダブルバッファリング、フレーム同期、CRC検証を全て確認
/// @return true: テスト成功, false: テスト失敗
bool axon_36byte_frame_test(void);

#endif  // __SOMA_UART_TEST_H__
