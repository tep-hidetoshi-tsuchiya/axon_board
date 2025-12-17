#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdint.h>

// 簡易デバッグログ（ノンブロッキング）
// リングバッファに1文字コードを記録し、アイドル時に出力

#define DEBUG_LOG_SIZE 32

typedef struct {
    uint8_t buffer[DEBUG_LOG_SIZE];
    volatile uint8_t write_idx;
    volatile uint8_t read_idx;
} debug_log_t;

extern debug_log_t g_debug_log;

// ログコード定義
#define LOG_DUPLICATE    'D'  // 重複検出
#define LOG_ACCEPTED     'A'  // 正常受理
#define LOG_RETRY_IRQ    'R'  // リトライIRQ送信
#define LOG_BTN_PRESS    'B'  // ボタン押下
#define LOG_BTN_RELEASE  'b'  // ボタンリリース

// ノンブロッキングでログを記録（数μs）
static inline void log_event(uint8_t code) {
    uint8_t next_idx = (g_debug_log.write_idx + 1) % DEBUG_LOG_SIZE;
    if (next_idx != g_debug_log.read_idx) {  // バッファフルチェック
        g_debug_log.buffer[g_debug_log.write_idx] = code;
        g_debug_log.write_idx = next_idx;
    }
}

// アイドル時にログを出力（ブロッキング可）
static inline void flush_debug_log(void) {
    while (g_debug_log.read_idx != g_debug_log.write_idx) {
        extern int printf(const char *fmt, ...);
        printf("%c", g_debug_log.buffer[g_debug_log.read_idx]);
        g_debug_log.read_idx = (g_debug_log.read_idx + 1) % DEBUG_LOG_SIZE;
    }
}

#endif // DEBUG_LOG_H
