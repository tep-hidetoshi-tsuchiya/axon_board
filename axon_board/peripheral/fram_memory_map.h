#ifndef __FRAM_MEMORY_MAP_H__
#define __FRAM_MEMORY_MAP_H__

#ifdef SOMA_BOARD
#include <stdint.h>
#include <stdbool.h>

// EXTERNマクロ
#ifdef DEFINE_GLOBALS
    #define EXTERN
#else
    #define EXTERN extern
#endif

// FRAMメモリマップ定数定義
#define FRAM_AXON_SERIAL_SIZE       10  // AXON基板シリアル番号サイズ
#define FRAM_CASH_COUNTER_SIZE      2   // 現金カウンタサイズ
#define FRAM_PRIZE_COUNTER_SIZE     2   // プライズカウンタサイズ
#define FRAM_USER_MEMORY_SIZE       10  // ユーザーメモリサイズ
#define FRAM_MAX_AXON_BOARDS        9   // 最大AXON基板数

// 各AXON基板のデータ構造体
typedef struct {
    uint8_t serial[FRAM_AXON_SERIAL_SIZE];      // AXON基板シリアル番号 (10バイト)
    uint16_t cash_counter;                      // 現金カウンタ値 (2バイト)
    uint16_t prize_counter;                     // プライズカウンタ値 (2バイト)
} __attribute__((packed)) fram_axon_data_t;

// FRAMメモリマップ全体構造体
typedef struct {
    fram_axon_data_t axon_boards[FRAM_MAX_AXON_BOARDS];  // AXON基板データ配列 (9基板分)
    uint8_t user_memory[FRAM_USER_MEMORY_SIZE];          // ユーザーメモリ (10バイト)
} __attribute__((packed)) fram_memory_map_t;

// FRAMメモリマップのアドレス定義
#define FRAM_BASE_ADDRESS           0x00000000
#define FRAM_AXON_BOARD_SIZE        (sizeof(fram_axon_data_t))  // 14バイト
#define FRAM_USER_MEMORY_ADDRESS    0x0000007E

// 各AXON基板の開始アドレス計算マクロ
#define FRAM_AXON_BOARD_ADDRESS(board_num) \
    (FRAM_BASE_ADDRESS + ((board_num) * FRAM_AXON_BOARD_SIZE))

// 各フィールドのアドレス計算マクロ
#define FRAM_AXON_SERIAL_ADDRESS(board_num) \
    (FRAM_AXON_BOARD_ADDRESS(board_num) + offsetof(fram_axon_data_t, serial))

#define FRAM_CASH_COUNTER_ADDRESS(board_num) \
    (FRAM_AXON_BOARD_ADDRESS(board_num) + offsetof(fram_axon_data_t, cash_counter))

#define FRAM_PRIZE_COUNTER_ADDRESS(board_num) \
    (FRAM_AXON_BOARD_ADDRESS(board_num) + offsetof(fram_axon_data_t, prize_counter))

// アドレス検証マクロ
#define IS_VALID_BOARD_NUM(board_num) \
    ((board_num) >= 0 && (board_num) < FRAM_MAX_AXON_BOARDS)

#define IS_VALID_FRAM_ADDRESS(address) \
    ((address) >= FRAM_BASE_ADDRESS && (address) < (FRAM_BASE_ADDRESS + sizeof(fram_memory_map_t)))

// エラーコード定義
typedef enum {
    FRAM_ERROR_SUCCESS = 0,         // 成功
    FRAM_ERROR_INVALID_BOARD_NUM,   // 無効な基板番号
    FRAM_ERROR_INVALID_ADDRESS,     // 無効なアドレス
    FRAM_ERROR_SPI_TIMEOUT,         // SPI通信タイムアウト
    FRAM_ERROR_DATA_SIZE_MISMATCH,  // データサイズ不一致
    FRAM_ERROR_NULL_POINTER         // NULLポインタエラー
} fram_error_t;

// FRAMメモリマップのグローバルインスタンス（RAMキャッシュ用）
EXTERN fram_memory_map_t g_fram_memory_cache;

// RAMキャッシュアクセス関数プロトタイプ（fram_utils.hに移動済み）

// 静的アサート（コンパイル時チェック）
_Static_assert(sizeof(fram_axon_data_t) == 14, "fram_axon_data_t size must be 14 bytes");
_Static_assert(sizeof(fram_memory_map_t) == (14 * 9 + 10), "fram_memory_map_t size must be 136 bytes");

#endif // SOMA_BOARD
#endif /* __FRAM_MEMORY_MAP_H__ */