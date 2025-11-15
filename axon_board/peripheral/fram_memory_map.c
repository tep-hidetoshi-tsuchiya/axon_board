#ifdef SOMA_BOARD
#define DEFINE_GLOBALS
#include "fram_memory_map.h"
#include "fram_utils.h"
#include <string.h>
#include <stddef.h>

/**
 * @brief FRAMメモリマップを初期化
 * @return fram_error_t エラーコード
 */
fram_error_t fram_memory_init(void) {
    // RAMキャッシュをゼロクリア
    memset(&g_fram_memory_cache, 0, sizeof(fram_memory_map_t));
    
    // FRAM全体をFRAMから読み込んでキャッシュに格納
    if (fram_read(FRAM_BASE_ADDRESS, (uint8_t*)&g_fram_memory_cache, sizeof(fram_memory_map_t)) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief AXON基板シリアル番号を書き込み
 * @param board_num 基板番号 (0-8)
 * @param serial_data シリアル番号データ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_write_axon_serial(uint8_t board_num, const uint8_t* serial_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (serial_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_AXON_SERIAL_ADDRESS(board_num);
    
    // FRAMに書き込み
    if (fram_write(address, (uint8_t*)serial_data, FRAM_AXON_SERIAL_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(g_fram_memory_cache.axon_boards[board_num].serial, serial_data, FRAM_AXON_SERIAL_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief AXON基板シリアル番号を読み込み
 * @param board_num 基板番号 (0-8)
 * @param serial_data 読み込み先バッファ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_read_axon_serial(uint8_t board_num, uint8_t* serial_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (serial_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_AXON_SERIAL_ADDRESS(board_num);
    
    // FRAMから読み込み
    if (fram_read(address, serial_data, FRAM_AXON_SERIAL_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(g_fram_memory_cache.axon_boards[board_num].serial, serial_data, FRAM_AXON_SERIAL_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief 現金カウンタ値を書き込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value カウンタ値
 * @return fram_error_t エラーコード
 */
fram_error_t fram_write_cash_counter(uint8_t board_num, uint16_t counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    uint16_t address = FRAM_CASH_COUNTER_ADDRESS(board_num);
    
    // リトルエンディアンでFRAMに書き込み
    uint8_t data[2] = {
        (uint8_t)(counter_value & 0xFF),
        (uint8_t)((counter_value >> 8) & 0xFF)
    };
    
    if (fram_write(address, data, FRAM_CASH_COUNTER_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    g_fram_memory_cache.axon_boards[board_num].cash_counter = counter_value;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief 現金カウンタ値を読み込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value 読み込み先ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_read_cash_counter(uint8_t board_num, uint16_t* counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (counter_value == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_CASH_COUNTER_ADDRESS(board_num);
    uint8_t data[2];
    
    // FRAMから読み込み
    if (fram_read(address, data, FRAM_CASH_COUNTER_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // リトルエンディアンから変換
    *counter_value = data[0] | (data[1] << 8);
    
    // RAMキャッシュも更新
    g_fram_memory_cache.axon_boards[board_num].cash_counter = *counter_value;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief プライズカウンタ値を書き込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value カウンタ値
 * @return fram_error_t エラーコード
 */
fram_error_t fram_write_prize_counter(uint8_t board_num, uint16_t counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    uint16_t address = FRAM_PRIZE_COUNTER_ADDRESS(board_num);
    
    // リトルエンディアンでFRAMに書き込み
    uint8_t data[2] = {
        (uint8_t)(counter_value & 0xFF),
        (uint8_t)((counter_value >> 8) & 0xFF)
    };
    
    if (fram_write(address, data, FRAM_PRIZE_COUNTER_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    g_fram_memory_cache.axon_boards[board_num].prize_counter = counter_value;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief プライズカウンタ値を読み込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value 読み込み先ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_read_prize_counter(uint8_t board_num, uint16_t* counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (counter_value == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_PRIZE_COUNTER_ADDRESS(board_num);
    uint8_t data[2];
    
    // FRAMから読み込み
    if (fram_read(address, data, FRAM_PRIZE_COUNTER_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // リトルエンディアンから変換
    *counter_value = data[0] | (data[1] << 8);
    
    // RAMキャッシュも更新
    g_fram_memory_cache.axon_boards[board_num].prize_counter = *counter_value;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief ユーザーメモリを書き込み
 * @param user_data ユーザーデータ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_write_user_memory(const uint8_t* user_data) {
    if (user_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // FRAMに書き込み
    if (fram_write(FRAM_USER_MEMORY_ADDRESS, (uint8_t*)user_data, FRAM_USER_MEMORY_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(g_fram_memory_cache.user_memory, user_data, FRAM_USER_MEMORY_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief ユーザーメモリを読み込み
 * @param user_data 読み込み先バッファ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_read_user_memory(uint8_t* user_data) {
    if (user_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // FRAMから読み込み
    if (fram_read(FRAM_USER_MEMORY_ADDRESS, user_data, FRAM_USER_MEMORY_SIZE) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(g_fram_memory_cache.user_memory, user_data, FRAM_USER_MEMORY_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief 指定基板の全データを読み込み
 * @param board_num 基板番号 (0-8)
 * @param axon_data 読み込み先構造体ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_read_axon_board_data(uint8_t board_num, fram_axon_data_t* axon_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (axon_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_AXON_BOARD_ADDRESS(board_num);
    
    // FRAM基板データ全体を一括読み込み
    if (fram_read(address, (uint8_t*)axon_data, sizeof(fram_axon_data_t)) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(&g_fram_memory_cache.axon_boards[board_num], axon_data, sizeof(fram_axon_data_t));
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief 指定基板の全データを書き込み
 * @param board_num 基板番号 (0-8)
 * @param axon_data 書き込みデータ構造体ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_write_axon_board_data(uint8_t board_num, const fram_axon_data_t* axon_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (axon_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    uint16_t address = FRAM_AXON_BOARD_ADDRESS(board_num);
    
    // FRAM基板データ全体を一括書き込み
    if (fram_write(address, (uint8_t*)axon_data, sizeof(fram_axon_data_t)) != 0) {
        return FRAM_ERROR_SPI_TIMEOUT;
    }
    
    // RAMキャッシュも更新
    memcpy(&g_fram_memory_cache.axon_boards[board_num], axon_data, sizeof(fram_axon_data_t));
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief カウンタ値をインクリメント
 * @param board_num 基板番号 (0-8)
 * @param is_cash_counter true: 現金カウンタ, false: プライズカウンタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_increment_counter(uint8_t board_num, bool is_cash_counter) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    uint16_t current_value;
    fram_error_t result;
    
    // 現在値を読み込み
    if (is_cash_counter) {
        result = fram_read_cash_counter(board_num, &current_value);
    } else {
        result = fram_read_prize_counter(board_num, &current_value);
    }
    
    if (result != FRAM_ERROR_SUCCESS) {
        return result;
    }
    
    // インクリメント（オーバーフロー防止）
    if (current_value < 0xFFFF) {
        current_value++;
    }
    
    // 新しい値を書き込み
    if (is_cash_counter) {
        result = fram_write_cash_counter(board_num, current_value);
    } else {
        result = fram_write_prize_counter(board_num, current_value);
    }
    
    return result;
}

/**
 * @brief RAMキャッシュからAXON基板シリアル番号を高速読み込み
 * @param board_num 基板番号 (0-8)
 * @param serial_data 読み込み先バッファ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_cache_read_axon_serial(uint8_t board_num, uint8_t* serial_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (serial_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // RAMキャッシュから直接読み込み（SPI通信なし）
    memcpy(serial_data, g_fram_memory_cache.axon_boards[board_num].serial, FRAM_AXON_SERIAL_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief RAMキャッシュから現金カウンタ値を高速読み込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value 読み込み先ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_cache_read_cash_counter(uint8_t board_num, uint16_t* counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (counter_value == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // RAMキャッシュから直接読み込み（SPI通信なし）
    *counter_value = g_fram_memory_cache.axon_boards[board_num].cash_counter;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief RAMキャッシュからプライズカウンタ値を高速読み込み
 * @param board_num 基板番号 (0-8)
 * @param counter_value 読み込み先ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_cache_read_prize_counter(uint8_t board_num, uint16_t* counter_value) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (counter_value == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // RAMキャッシュから直接読み込み（SPI通信なし）
    *counter_value = g_fram_memory_cache.axon_boards[board_num].prize_counter;
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief RAMキャッシュから指定基板の全データを高速読み込み
 * @param board_num 基板番号 (0-8)
 * @param axon_data 読み込み先構造体ポインタ
 * @return fram_error_t エラーコード
 */
fram_error_t fram_cache_read_axon_board_data(uint8_t board_num, fram_axon_data_t* axon_data) {
    if (!IS_VALID_BOARD_NUM(board_num)) {
        return FRAM_ERROR_INVALID_BOARD_NUM;
    }
    
    if (axon_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // RAMキャッシュから直接読み込み（SPI通信なし）
    memcpy(axon_data, &g_fram_memory_cache.axon_boards[board_num], sizeof(fram_axon_data_t));
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief RAMキャッシュからユーザーメモリを高速読み込み
 * @param user_data 読み込み先バッファ (10バイト)
 * @return fram_error_t エラーコード
 */
fram_error_t fram_cache_read_user_memory(uint8_t* user_data) {
    if (user_data == NULL) {
        return FRAM_ERROR_NULL_POINTER;
    }
    
    // RAMキャッシュから直接読み込み（SPI通信なし）
    memcpy(user_data, g_fram_memory_cache.user_memory, FRAM_USER_MEMORY_SIZE);
    
    return FRAM_ERROR_SUCCESS;
}

/**
 * @brief エラーコードを文字列に変換（デバッグ用）
 * @param error エラーコード
 * @return エラー説明文字列
 */
const char* fram_error_to_string(fram_error_t error) {
    switch (error) {
        case FRAM_ERROR_SUCCESS:             return "Success";
        case FRAM_ERROR_INVALID_BOARD_NUM:   return "Invalid board number";
        case FRAM_ERROR_INVALID_ADDRESS:     return "Invalid address";
        case FRAM_ERROR_SPI_TIMEOUT:         return "SPI timeout";
        case FRAM_ERROR_DATA_SIZE_MISMATCH:  return "Data size mismatch";
        case FRAM_ERROR_NULL_POINTER:        return "Null pointer";
        default:                             return "Unknown error";
    }
}
#endif // SOMA_BOARD