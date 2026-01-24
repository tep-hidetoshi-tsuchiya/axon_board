/**
 * @file aes256.h
 * @brief AES-256 ECBモード（ソフト実装）ヘッダー
 * @details TinyAES-cライブラリをベースにしたAES-256/ECBのシンプルなAPI
 *          MSPM0G3507用：ハードAES不使用、ソフト実装のみ
 */

#ifndef __AES256_H__
#define __AES256_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief AES-256ステータス
 */
typedef enum {
    AES256_SUCCESS = 0,
    AES256_ERROR_NULL_POINTER = 1,
    AES256_ERROR_INVALID_SIZE = 2,
    AES256_ERROR_FAILED = 3
} aes256_status_t;

/**
 * @brief AES-256コンテキスト
 */
typedef struct {
    uint8_t round_key[240];
    int key_length;
} aes256_ctx_t;

/**
 * @brief AES-256初期化（キー展開）
 * @param ctx コンテキストポインタ
 * @param key 256ビット鍵（32バイト）
 * @return AES256_SUCCESS: 成功、その他：エラー
 */
aes256_status_t aes256_init(aes256_ctx_t* ctx, const uint8_t* key);

/**
 * @brief AES-256/ECB暗号化（ブロック単位）
 * @param ctx 初期化済みコンテキスト
 * @param plaintext 平文（16バイト単位）
 * @param ciphertext 暗号文出力（同じサイズ）
 * @param length バイト数（16の倍数）
 * @return AES256_SUCCESS: 成功、その他：エラー
 */
aes256_status_t aes256_encrypt_ecb(
    aes256_ctx_t* ctx,
    const uint8_t* plaintext,
    uint8_t* ciphertext,
    size_t length
);

/**
 * @brief AES-256/ECB復号（ブロック単位）
 * @param ctx 初期化済みコンテキスト
 * @param ciphertext 暗号文
 * @param plaintext 平文出力（同じサイズ）
 * @param length バイト数（16の倍数）
 * @return AES256_SUCCESS: 成功、その他：エラー
 */
aes256_status_t aes256_decrypt_ecb(
    aes256_ctx_t* ctx,
    const uint8_t* ciphertext,
    uint8_t* plaintext,
    size_t length
);

#ifdef __cplusplus
}
#endif

#endif // __AES256_H__
