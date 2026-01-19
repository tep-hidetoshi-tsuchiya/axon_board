#include "s2a_packet.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(AXON_BOARD)
#include "ti_msp_dl_config.h"
#include "../../peripheral/msp_peripheral_config.h"  // GPIO pin definitions
#include "../../peripheral/dipsw_utils.h"
#include "../../peripheral/hw_ver_utils.h"
#include "../../driver/systick.h"  // systick timer - must be before led_utils.h
#include "../../peripheral/led_utils.h"  // LED PWM control functions
#include "../../event.h"
#include "../../axon_status.h"  // AXON status structure and functions
#include "../../debug_log.h"  // ノンブロッキングログ
// #include "../../driver/utils/tiny_aes.h"  // Software AES (MSPM0 DECRYPT bug workaround) - NOT NEEDED for AXON
#include "../../soma_axon_comm_test.h"  // Test mode functions
// External UART send function (implemented in soma_uart_test.c)
extern bool uart_send_packet(const uint8_t* data, size_t len);
extern bool uart_send_packet_fast(const uint8_t* data, size_t len);  // 高速版（ACK/ATIRQ用）

// グローバル変数: コマンド受信トグルビット
static volatile uint8_t g_cmd_recv_toggle = 0;

// 外部参照: axon_routine.cで定義されたグローバル変数
// ★CRITICAL: volatile必須（複数の実行コンテキスト間で共有）
extern volatile uint8_t g_left_amount;
extern volatile uint8_t g_right_amount;
extern volatile uint8_t g_pending_left_amount;   // ATIRQ送信用（ボタン押下時の+1値を保持）
extern volatile uint8_t g_pending_right_amount;  // ATIRQ送信用（ボタン押下時の+1値を保持）
extern volatile uint8_t g_pending_left_updated;  // pending値更新フラグ
extern volatile uint8_t g_pending_right_updated; // pending値更新フラグ
extern volatile uint8_t g_retry_pending;         // 重複検出リトライ中フラグ
extern volatile uint8_t g_irq_pulse_pending;     // IRQ_Nパルス状態フラグ（axon_routine.c）
extern volatile systick_t g_irq_pulse_start_time;  // IRQ_N Low開始時刻

// 重複検出時のIRQ再送要求フラグ（メインループで処理）
volatile uint8_t g_retry_irq_request = 0;
#endif

#ifdef SOMA_BOARD

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMEOUT_MS 1000

// Forward declarations for functions
// MUX制御関連
static void mux_select(uint8_t n);

// UART通信関連
static void uart_send(uint8_t* data, size_t len);
static bool uart_receive(uint8_t* data, size_t len);

// SOMA RAM関連
static void soma_ram_save(uint8_t board_num, uint8_t* serial_number, uint8_t fw_ver);

// FWアップデート関連
static void fw_update_sequence(uint8_t board_num);

// タイム関連
static uint32_t get_current_time_ms(void);

// ヘルパー関数の前方宣言
static bool receive_atirq_response(ATIRQ_PACKET* response);
static void save_to_soma_ram(uint8_t n, ATIRQ_PACKET* response);
static bool check_fw_version(uint8_t fw_version);

void soma_port_check_sequence() {
    for (uint8_t n = 1; n <= 9; n++) {
        // 1. MUX 切替
        mux_select(n);

        // 2. CHKIRQ 送信
        CHKIRQ_PACKET chkirq = {0x10, sizeof(CHKIRQ_PACKET), 0x49, 0, 0, {0}, 0, 0, 0, 0};
        uart_send((uint8_t*)&chkirq, sizeof(chkirq));
        printf("CHKIRQ command sent to AXON %d\n", n);

        // 3. ATIRQ 受信
        ATIRQ_PACKET atirq;
        if (!receive_atirq_response(&atirq)) {
            // タイムアウト処理
            printf("Timeout: No response from AXON %d\n", n);
            continue;
        }

        // 4. 情報を SOMA RAM に保存
        save_to_soma_ram(n, &atirq);
        printf("AXON %d info saved to SOMA RAM\n", n);

        // 5. FW Version 確認
        if (check_fw_version(atirq.afw_ver)) {
            // FW 更新シーケンスへ分岐
            fw_update_sequence(n);
        }
    }
}

// Helper functions (Stub implementations - to be completed later)

static void mux_select(uint8_t n) {
    // TODO: MUX を指定ポートに切り替える処理を実装
    printf("MUX switched to AXON %d\n", n);
}

static void uart_send(uint8_t* data, size_t len) {
    // TODO: UART送信処理を実装
    printf("UART send %zu bytes\n", len);
}

static bool uart_receive(uint8_t* data, size_t len) {
    // TODO: UART受信処理を実装
    (void)data;
    (void)len;
    return false; // 現在は常に失敗を返す
}

static void soma_ram_save(uint8_t board_num, uint8_t* serial_number, uint8_t fw_ver) {
    // TODO: SOMA RAMへの保存処理を実装
    printf("Save to SOMA RAM: board=%d, fw_ver=0x%02X\n", board_num, fw_ver);
    (void)serial_number;
}

static void fw_update_sequence(uint8_t board_num) {
    // TODO: FWアップデートシーケンスを実装
    printf("FW update sequence for board %d\n", board_num);
}

static uint32_t get_current_time_ms(void) {
    // FreeRTOSのティック数をミリ秒に変換
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static bool receive_atirq_response(ATIRQ_PACKET* response) {
    uint32_t start_time = get_current_time_ms();
    while (get_current_time_ms() - start_time < TIMEOUT_MS) {
        if (uart_receive((uint8_t*)response, sizeof(ATIRQ_PACKET))) {
            printf("ATIRQ response received\n");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms待機
    }
    return false; // タイムアウト
}

static void save_to_soma_ram(uint8_t n, ATIRQ_PACKET* response) {
    soma_ram_save(n, response->msn, response->afw_ver);
    printf("AXON %d info saved to SOMA RAM\n", n);
}

static bool check_fw_version(uint8_t fw_version) {
    const uint8_t current_fw_version = 0x10; // 現行バージョン
    return fw_version < current_fw_version;
}

#endif // defined(SOMA_BOARD)

// ============================================================
// AXON Board Implementation
// ============================================================
#if defined(AXON_BOARD)

// Forward declarations
static bool aes_decrypt_cbc(const uint8_t* encrypted_data, uint8_t* decrypted_data);
static bool aes_encrypt_cbc(const uint8_t* plaintext_data, uint8_t* encrypted_data);
static uint16_t crc16_tep(const uint8_t* data, size_t len);
static bool send_ack_frame(void);
static bool send_nack_frame(uint8_t err_code);
static void clear_irq_signal(void);  // IRQ信号クリア（High設定）

// AXON board information (should be stored in FRAM/Flash in production)
// シリアル番号フォーマット: 例）25L6200001 ⇒ 0x19_0C_3E_00_03E9
//   Byte[0]: 製造年（25 → 0x19）
//   Byte[1]: 製造月（L=12月 → 0x0C）
//   Byte[2]: 製品番号（62 → 0x3E）
//   Byte[3]: オプション（0 → 0x00）
//   Byte[4-5]: ロット番号（1001 → 0x03E9、ビッグエンディアン）
static const uint8_t axon_serial_number[6] = {0x19, 0x0C, 0x3E, 0x00, 0x03, 0xE9};  // 25L6201001
static const uint8_t axon_fw_version = 0x10;  // FW Version 1.0 (bit[7:4]=Major, bit[3:0]=Minor)
static const uint8_t axon_fw_min_version = 0x10;  // 最小要求FWバージョン

// FW更新用グローバル変数
static uint32_t g_fw_total_crc = 0;      // 受信したFWコード全体のCRC累積
static uint32_t g_fw_last_address = 0;   // 最後に受信したアドレス

/**
 * @brief DIC値を生成（シリアル番号ベース）
 * @return DIC値（16bit）
 * @details 固定値0x0123を使用（[28]=0x01, [29]=0x23の順で送信）
 *          複数AXON基板の識別に使用
 *          リトルエンディアンシステムで[28]=0x01, [29]=0x23となるよう0x2301を返す
 */
static inline uint16_t generate_dic_from_serial(void) {
    return 0x2301;  // リトルエンディアンで[28]=0x01, [29]=0x23と送信される
}

// デバッグ変数（外部から参照可能）
volatile uint32_t debug_chkirq_call_count = 0;
volatile uint32_t debug_header_ng_count = 0;
volatile uint32_t debug_len_ng_count = 0;
volatile uint32_t debug_crc_ng_count = 0;
volatile uint32_t debug_cmd_ng_count = 0;
volatile uint32_t debug_decrypt_ng_count = 0;
volatile uint32_t debug_encrypt_ng_count = 0;
volatile uint32_t debug_send_ok_count = 0;
volatile uint16_t debug_crc_recv = 0;
volatile uint16_t debug_crc_calc = 0;
volatile uint8_t debug_decrypted_id = 0;
volatile uint8_t debug_decrypted_axon_num = 0;

// デバッグ用: ATIRQ暗号化の確認
volatile uint8_t debug_atirq_plain[32] = {0};
volatile uint8_t debug_atirq_encrypted[32] = {0};

volatile uint32_t debug_led_event_count = 0; // LEDイベントをログ化するためのカウンタ

// AES-256-ECB encryption key (SOMA側と一致)
static const uint8_t aes_key[32] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01
};
// 注: ECBモードではIVは使用しないが、将来の拡張のため定義を残す
static const uint8_t aes_iv[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

/**
 * @brief AES初期化（AXON起動時に1回だけ呼ぶ）
 * @details MSPM0 AESモジュールを初期化し、KEYを設定。
 *          SYSCFG_DL_AES_init()はENCRYPT_ECBモードで初期化するため、
 *          ここでは何もしない。各関数内で必要なモードでinit+setKeyを実行する。
 *          （TI SDK aes_cbc_256_enc_decサンプルと同じ方式）
 */
void axon_aes_init(void)
{
    // TI SDKサンプルと同じく、暗号化/復号の前に必ずinit+setKeyを実行するため
    // ここでは何もしない（TinyAES-Cは各関数内で初期化）
}
/**
 * @brief FWバージョンチェック（最小要求バージョンとの比較）
 * @param recv_min_version 受信した最小要求FWバージョン
 * @return true: バージョンOK, false: バージョンNG（更新必要）
 */
static inline bool check_fw_version(uint8_t recv_min_version)
{
    // 現在のFWバージョンが最小要求バージョン以上かチェック
    // Major version check (bit[7:4])
    uint8_t current_major = (axon_fw_version >> 4) & 0x0F;
    uint8_t required_major = (recv_min_version >> 4) & 0x0F;
    
    if (current_major < required_major) {
        return false;  // Major version不足
    }
    if (current_major > required_major) {
        return true;   // Major versionが上回っている
    }
    
    // Major versionが同じ場合、Minor version check (bit[3:0])
    uint8_t current_minor = axon_fw_version & 0x0F;
    uint8_t required_minor = recv_min_version & 0x0F;
    
    return (current_minor >= required_minor);
}

/**
 * @brief CHKIRQ受信時の処理（AXON側）
 * @details SOMAから暗号化CHKIRQコマンドを受信し、暗号化ATIRQ応答を返す
 * @param encrypted_frame 受信した暗号化フレーム（36バイト: Header[1] + LEN[1] + Encrypted[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_chkirq(const uint8_t* encrypted_frame)
{
    debug_chkirq_call_count++;
    
    // デバッグログ簡略化（UART通信安定化）
    #if 0
    // 時刻取得
    systick_t current_ms = get_systick_count_ms();
    uint32_t total_sec = current_ms / 1000;
    uint32_t hours = (total_sec / 3600) % 24;
    uint32_t minutes = (total_sec / 60) % 60;
    uint32_t seconds = total_sec % 60;
    uint32_t ms = current_ms % 1000;
    
    NVIC_DisableIRQ(UART0_INT_IRQn);
    printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] Called (count=%lu)\n",
           (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds, (unsigned long)ms,
           (unsigned long)debug_chkirq_call_count);
    NVIC_EnableIRQ(UART0_INT_IRQn);
    #endif
    
    // テストモード: コマンド受信記録
    // axon_test_record_command(0x49);
    
    // 全LED消灯
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);
    
    if (encrypted_frame == NULL) {
        #if 0
        systick_t t = get_systick_count_ms();
        uint32_t s = t / 1000;
        NVIC_DisableIRQ(UART0_INT_IRQn);
        printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] ERROR: NULL frame\n",
               (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000));
        NVIC_EnableIRQ(UART0_INT_IRQn);
        #endif
        // NULL: indicate error via LEDs and return error (avoid infinite halt)
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_2);
        return false;
    }

    // 1. Header確認
    if (encrypted_frame[0] != 0x14) {
        systick_t t = get_systick_count_ms();
        uint32_t s = t / 1000;
        NVIC_DisableIRQ(UART0_INT_IRQn);
        printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] ERROR: Invalid header 0x%02X\n",
               (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000), encrypted_frame[0]);
        NVIC_EnableIRQ(UART0_INT_IRQn);
        debug_header_ng_count++;
        return false;
    }
    
    // 2. LEN確認
    if (encrypted_frame[1] != 0x20) {
        systick_t t = get_systick_count_ms();
        uint32_t s = t / 1000;
        NVIC_DisableIRQ(UART0_INT_IRQn);
        printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] ERROR: Invalid length 0x%02X\n",
               (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000), encrypted_frame[1]);
        NVIC_EnableIRQ(UART0_INT_IRQn);
        debug_len_ng_count++;
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（32バイト）を計算対象とする
    uint16_t crc_recv = encrypted_frame[34] | (encrypted_frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&encrypted_frame[2], 32);  // Data部のみ（Byte[2-33]）
    debug_crc_recv = crc_recv;
    debug_crc_calc = crc_calc;
    
    if (crc_recv != crc_calc) {
        systick_t t = get_systick_count_ms();
        uint32_t s = t / 1000;
        NVIC_DisableIRQ(UART0_INT_IRQn);
        printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] ERROR: CRC mismatch recv=0x%04X calc=0x%04X\n",
               (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000), crc_recv, crc_calc);
        NVIC_EnableIRQ(UART0_INT_IRQn);
        debug_crc_ng_count++;
        axon_test_record_crc_error();  // テストモード: CRCエラー記録
        return false;
    }

    // 4. 平文データ取得（暗号化なし）
    uint8_t* decrypted_data = (uint8_t*)&encrypted_frame[2];

    // 5. コマンドID確認（CHKIRQ平文32バイト構造体で解釈）
    CHKIRQ_PLAIN32* chkirq_plain = (CHKIRQ_PLAIN32*)decrypted_data;
    debug_decrypted_id = chkirq_plain->id;
    if (chkirq_plain->id != 0x49) {
        systick_t t = get_systick_count_ms();
        uint32_t s = t / 1000;
        NVIC_DisableIRQ(UART0_INT_IRQn);
        printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] ERROR: Invalid command ID 0x%02X\n",
               (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000), chkirq_plain->id);
        NVIC_EnableIRQ(UART0_INT_IRQn);
        // コマンドID不一致: ログカウンタを増やして終了（LED点滅はログ化）
        debug_cmd_ng_count++;
        debug_led_event_count++;
        return false;
    }
    
    systick_t t = get_systick_count_ms();
    uint32_t s = t / 1000;
    NVIC_DisableIRQ(UART0_INT_IRQn);
    printf("[%02lu:%02lu:%02lu.%03lu][CHKIRQ_HANDLER] Validation passed, preparing ATIRQ response\n",
           (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000));
    NVIC_EnableIRQ(UART0_INT_IRQn);

    // CMD ID検証OK: 黄色LED短め点滅
    // CMD ID検証OK: ログ化（黄色LED短め点滅をログで代替）
    debug_led_event_count++;

        // CHKIRQ受信をトリガにIRQ_NをHighへ戻す（SOMA側の受信を完了させるため）
        g_irq_pulse_pending = 0;
        g_irq_pulse_start_time = 0;
        clear_irq_signal();

    // 6. ATIRQ平文データ（32バイト）を仕様準拠structで構築
    ATIRQ_PLAIN32 atirq_plain;
    memset(&atirq_plain, 0, sizeof(atirq_plain));
    
    // ID: 0x6A (ATIRQ識別子)
    atirq_plain.id = 0x6A;
    
    // MD (モード通知): SOMA側仕様準拠 Table 4-16
    uint8_t md = 0x00;
    
    // bit7: AXON基板リセットフラグ (0=通常, 1=リセット状態)
    if (g_axon_status_shared.reset_flag) {
        md |= (1 << 7);
    }
    
    // bit6-5: RFU (0固定)
    
    // bit4: コマンド受信トグル（SETAXONコマンド受信毎にトグル）
    // 注: CHKIRQでは変更しない（SETAXONハンドラでトグル）
    extern volatile uint8_t g_cmd_recv_toggle;
    if (g_cmd_recv_toggle) {
        md |= (1 << 4);
    }
    
    // bit3: 面番号設定中 (7セグLED点滅中)
    // ボタン押下後、SETAXONで確定するまで設定中状態
    if (g_pending_right_updated) {
        md |= (1 << 3);
    }
    
    // bit2: 金額設定中 (7セグLED点滅中)
    // ボタン押下後、SETAXONで確定するまで設定中状態
    if (g_pending_left_updated) {
        md |= (1 << 2);
    }
    
    // bit1: LEFT（金額枚数）ボタン押下状態 (0=通常, 1=押下中)
    extern button_event_t g_button_1_event;
    if (g_button_1_event.pressed) {
        md |= (1 << 1);
    }
    
    // bit0: RIGHT（面）ボタン押下状態 (0=通常, 1=押下中)
    extern button_event_t g_button_2_event;
    if (g_button_2_event.pressed) {
        md |= (1 << 0);
    }
    
    atirq_plain.md = md;
    
    // FACE_N: 設定面番号（下位4bitのみ使用、上位4bitはRFU）
    // ★仕様書準拠: ボタン押下時はpending値を送信
    //   - g_pending_right_updated=1: g_pending_right_amount (ボタン押下時の+1値)
    //   - g_pending_right_updated=0: g_right_amount (現在の表示値)
    uint8_t face_to_send = g_pending_right_updated ? g_pending_right_amount : g_right_amount;
    atirq_plain.face_n = face_to_send & 0x0F;
    
    // デバッグ: ATIRQ送信時の状態をLEDで表示
    if (g_pending_right_updated) {
        // 黄色LED短時間点灯（pending値送信中）
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1);  // R+G = 黄色
        delay_cycles(CPUCLK_FREQ / 200);  // 5ms
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1);
    }
    
    // CASH_VLU: 設定金額（100円単位、リトルエンディアン）
    // ★仕様書準拠: ボタン押下時はpending値を送信
    //   - g_pending_left_updated=1: g_pending_left_amount (ボタン押下時の+1値)
    //   - g_pending_left_updated=0: g_left_amount (現在の表示値)
    uint16_t cash_to_send = g_pending_left_updated ? g_pending_left_amount : g_left_amount;
    atirq_plain.cash_vlu = cash_to_send;
    
    // STATUS: 16bitステータスフィールド（Table 4-16準拠）
    //   bit[15:8]: ダイヤル回転数カウント (0x00→0xFF循環)
    //   bit7: RFU
    //   bit6: ドア開閉 (0=CLOSE, 1=OPEN)
    //   bit5: 現金ブロック (0=通常, 1=ブロック)
    //   bit4: 返却ボタン押下 (0=通常, 1=押下)
    //   bit3: 光センサー/現金用 Latch式 (0=現金投入中, 1=現金なし)
    //   bit2: 電子マネーソレノイド (0=回転不可, 1=回転OK)
    //   bit1: 売り切れ検知 (0=販売可能, 1=売り切れ)
    //   bit0: FACE有効無効 (0=無効, 1=有効)
    atirq_plain.status = axon_status_compose_bits();
    
    // MSN: AXON基板シリアル番号（6バイト）
    memcpy(atirq_plain.msn, axon_serial_number, 6);     // static const uint8_t axon_serial_number[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    
    // AFW_VER: FWバージョン（bit7-4=Major, bit3-0=Minor）
    atirq_plain.afw_ver = axon_fw_version;  // 0x10 = Version 1.0
    
    // CHK_LED: LED状態（Table 4-13準拠）
    //   bit0: R (0=消灯, 1=点灯)
    //   bit1: G (0=消灯, 1=点灯)
    //   bit2: B (0=消灯, 1=点灯)
    //   bit3: RFU
    //   bit[6:4]: 動作モード (0=点灯, 1=点滅1秒, 2=点滅2秒, 3=点滅0.5秒, 4-7=RFU)
    //   bit7: RFU
    atirq_plain.chk_led = g_axon_status_shared.led_pattern;
    
    // CHK_TOUT: タイムアウト設定 (bit[7:4]=RFU, bit[3:0]=タイムアウト値)
    //   0x0=30秒(Default), 0x1=15秒, 0x2=20秒, 0x3=25秒, 0x4=30秒,
    //   0x5=35秒, 0x6=40秒, 0x7=45秒, 0x8=50秒, 0x9=55秒,
    //   0xA=60秒, 0xB=90秒, 0xC=120秒, 0xD=150秒, 0xE=無限秒
    // g_axon_status_shared.timeout_secondsから逆変換
    uint8_t tout_code = 0x00;  // デフォルト30秒
    switch (g_axon_status_shared.timeout_seconds) {
        case 15:  tout_code = 0x01; break;
        case 20:  tout_code = 0x02; break;
        case 25:  tout_code = 0x03; break;
        case 30:  tout_code = 0x00; break;  // または 0x04
        case 35:  tout_code = 0x05; break;
        case 40:  tout_code = 0x06; break;
        case 45:  tout_code = 0x07; break;
        case 50:  tout_code = 0x08; break;
        case 55:  tout_code = 0x09; break;
        case 60:  tout_code = 0x0A; break;
        case 90:  tout_code = 0x0B; break;
        case 120: tout_code = 0x0C; break;
        case 150: tout_code = 0x0D; break;
        case 255: tout_code = 0x0E; break;  // 無限秒
        default:  tout_code = 0x00; break;  // その他は30秒
    }
    atirq_plain.chk_tout = tout_code;
    
    // RFU: 10バイト ALL 0固定 (仕様: 19th～28th Byte)
    memset(atirq_plain.rfu, 0, sizeof(atirq_plain.rfu));
    
    // DIC/AuthCode/RND (29-34th Byte)
    atirq_plain.dic       = generate_dic_from_serial();  // シリアル番号から生成
    atirq_plain.auth_code = 0x0000;
    atirq_plain.rnd       = 0x0000;

    // ATIRQ平文構築完了: ログ化（シアンLED点滅をログで代替）
    debug_led_event_count++;

    // 7. 暗号化パケット構築
    uint8_t encrypted_packet[36];
    encrypted_packet[0] = 0x14;  // Header（Table 4-10準拠）
    encrypted_packet[1] = 0x20;

    memcpy((void*)debug_atirq_plain, &atirq_plain, 32);
    memset((void*)debug_atirq_encrypted, 0xFF, 32);

    // 暗号化前のインジケータはログ化（赤LED点滅をログで代替）
    debug_led_event_count++;

    // 平文データをそのままコピー（暗号化なし）
    memcpy(&encrypted_packet[2], &atirq_plain, 32);

    // ★仕様書準拠: Data部のみ（32バイト）をCRC計算
    uint16_t crc = crc16_tep(&encrypted_packet[2], 32);  // Data部のみ（Byte[2-33]）
    encrypted_packet[34] = (uint8_t)(crc & 0xFF);         // LSB
    encrypted_packet[35] = (uint8_t)((crc >> 8) & 0xFF);  // MSB

    // デバッグログ無効化（UART通信安定化）
    #if 0
    // デバッグ: ATIRQ送信直前ログ
    systick_t send_time = get_systick_count_ms();
    uint32_t send_sec = send_time / 1000;
    NVIC_DisableIRQ(UART0_INT_IRQn);
    printf("[%02lu:%02lu:%02lu.%03lu][ATIRQ_SEND] Calling uart_send_packet (36 bytes)\n",
           (send_sec/3600)%24, (send_sec/60)%60, send_sec%60, (unsigned long)(send_time%1000));
    printf("[ATIRQ_SEND] Data: %02X %02X %02X %02X %02X %02X...\n",
           encrypted_packet[0], encrypted_packet[1], encrypted_packet[2],
           encrypted_packet[3], encrypted_packet[4], encrypted_packet[5]);
    NVIC_EnableIRQ(UART0_INT_IRQn);
    #endif

    bool result = uart_send_packet_fast(encrypted_packet, 36);
    
    #if 0
    // デバッグ: ATIRQ送信完了ログ
    systick_t complete_time = get_systick_count_ms();
    uint32_t complete_sec = complete_time / 1000;
    NVIC_DisableIRQ(UART0_INT_IRQn);
    printf("[%02lu:%02lu:%02lu.%03lu][ATIRQ_SEND] uart_send_packet returned: %s (elapsed: %lu ms)\n",
           (complete_sec/3600)%24, (complete_sec/60)%60, complete_sec%60, 
           (unsigned long)(complete_time%1000),
           result ? "true" : "false",
           (unsigned long)(complete_time - send_time));
    NVIC_EnableIRQ(UART0_INT_IRQn);
    #endif
    
    if (result) {
        debug_send_ok_count++;
        axon_test_record_response(0x6A);  // テストモード: ATIRQ送信記録
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);
        
        // イベント系ラッチをクリア（次回検出に備える）
        axon_status_clear_event_latches();
    } else {
        DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1);
    }
    return result;
}

// ============================================================
// AES-128-CBC Encryption/Decryption Functions
// ============================================================

/**
 * @brief AES-128-CBC復号（ハードウェアAES使用）
 * @details TI SDK aes_cbc_256_enc_decサンプルの手順に基づく実装
 * @param encrypted_data 暗号化データ（32バイト）
 * @param decrypted_data 復号データ（32バイト）
 * @return true: 成功, false: 失敗
 */
static bool aes_decrypt_cbc(const uint8_t* encrypted_data, uint8_t* decrypted_data)
{
    if (!encrypted_data || !decrypted_data) return false;

    // AXON側ではAES復号は不要（平文通信）
    // SOMAから送信されるデータは既に平文化されている
    memcpy(decrypted_data, encrypted_data, 32);
    return true;
    
    // 以下、TinyAES-C実装（現在は不要のためコメントアウト）
    // uint8_t temp_buffer[32];
    // memcpy(temp_buffer, encrypted_data, 32);
    // struct AES_ctx ctx;
    // AES_init_ctx_iv(&ctx, aes_key, (uint8_t*)aes_iv);
    // AES_CBC_decrypt_buffer(&ctx, temp_buffer, 32);
    // memcpy(decrypted_data, temp_buffer, 32);
}

/**
 * @brief AES-256-ECB暗号化（ハードウェアAES使用）
 * @details ECBモード（Electronic Codebook Mode）：各ブロックを独立して暗号化
 * @param plaintext_data 平文データ（32バイト）
 * @param encrypted_data 暗号化データ（32バイト）
 * @return true: 成功, false: 失敗
 */
static bool aes_encrypt_cbc(const uint8_t* plaintext_data, uint8_t* encrypted_data)
{
    if (!plaintext_data || !encrypted_data) return false;

    uint8_t cipher[16];

    //
    // MSPM0 AES IMPORTANT:
    // ECBモードへ切り替え後は KEY が壊れる可能性あり
    // かならず毎回 setKey を実行する（TI Errata対応）
    //
    DL_AES_init(AES, DL_AES_MODE_ENCRYPT_ECB_MODE, DL_AES_KEY_LENGTH_256);
    DL_AES_setKey(AES, (uint8_t*)aes_key, DL_AES_KEY_LENGTH_256);

    // ---------- Block 0 (0-15バイト) ----------
    DL_AES_loadDataIn(AES, (uint8_t*)plaintext_data);
    while (DL_AES_isBusy(AES));
    DL_AES_getDataOut(AES, cipher);
    memcpy(encrypted_data, cipher, 16);

    // ---------- Block 1 (16-31バイト) ----------
    DL_AES_loadDataIn(AES, (uint8_t*)(plaintext_data + 16));
    while (DL_AES_isBusy(AES));
    DL_AES_getDataOut(AES, cipher);
    memcpy(encrypted_data + 16, cipher, 16);

    return true;
}


/**
 * @brief CRC16計算（ISO/IEC 13239準拠、SOMA側互換）
 * @details データ部をCRC16演算した結果。
 *          式：X^16+X^12+X^5+1
 *          初期値：0xFFFF
 *          演算方向：LSBファースト
 *          例）データ部が "0x000000" の3バイト時、演算結果は "0xC6CC" となる。
 * @param data データバッファ
 * @param len データ長
 * @return CRC16値（バイトスワップなし）
 */
static uint16_t crc16_tep(const uint8_t* data, size_t len)
{
    // CRC初期値を0xFFFFに設定
    uint16_t crc = 0xFFFF;

    // データの各バイトに対してCRC計算を実行
    for (size_t i = 0; i < len; i++) {
        // 現在のバイトとCRCをXOR
        crc ^= (uint16_t)data[i];
        
        // 1バイト分（8ビット）のビット単位処理
        for (int j = 0; j < 8; j++) {
            // LSB（最下位ビット）をチェック
            if (crc & 0x0001) {
                // LSBが1の場合：右シフトして多項式0x8408とXOR
                // 0x8408は標準多項式0x1021（X^16+X^12+X^5+1）のビット反転形式
                crc = (crc >> 1) ^ 0x8408;
            } else {
                // LSBが0の場合：単純に右シフト
                crc >>= 1;
            }
        }
    }

    // ISO/IEC 13239の要件:
    // 1. 最終XOR処理
    crc ^= 0xFFFF;
    
    // 2. バイトスワップなし（計算結果をそのまま返す）
    //    例: 0x9fb6 → リトルエンディアンで [34]=0x9f, [35]=0xb6 の順で送信
    return crc;
}

/**
 * @brief ACK応答送信（36バイトフレーム）
 * 
 * フォーマット: Header(0x10) + LEN(0x20) + データ32バイト + CRC16(2バイト)
 * データ内容: ID(0x00) + RFU(25バイト) + DIC(2) + AuthCode(2) + RND(2)
 * 
 * @return true:送信成功, false:送信失敗
 */
static bool send_ack_frame(void)
{
    S2A_PACKET packet;
    memset(&packet, 0, sizeof(packet));
    
    // ACKパケット構築
    packet.ack.header    = 0x10;
    packet.ack.len       = 0x20;
    packet.ack.id        = 0x00;
    packet.ack.dic       = generate_dic_from_serial();  // シリアル番号から生成
    packet.ack.auth_code = 0x0000;
    packet.ack.rnd       = 0x0000;
    
    // ★仕様書準拠: Data部のみ（32バイト）をCRC計算
    // ACK構造体のData部開始 = Header(1) + LEN(1) の次 = offset 2
    packet.ack.crc16 = crc16_tep((uint8_t*)&packet.ack + 2, 32);
    
    // テストモード: ACK送信記録
    axon_test_record_response(0x00);
    
    // UART送信（高速版 - 応答は即座に返す）
    return uart_send_packet_fast((uint8_t*)&packet.ack, sizeof(ACK_PACKET));
}

/**
 * @brief IRQ信号をクリア（High = 非アクティブ）
 * @details SOMA-AXON通信仕様に基づき、IRQ信号をHighに設定
 *          - ATIRQ/ACK/NACK送信完了後に呼び出す
 *          - エラー発生時にも呼び出してIRQ信号をクリア
 */
static void clear_irq_signal(void)
{
    systick_t t = get_systick_count_ms();
    uint32_t s = t / 1000;
    NVIC_DisableIRQ(UART0_INT_IRQn);
    printf("[%02lu:%02lu:%02lu.%03lu][IRQ_SIGNAL] Low -> High (cleared)\n",
           (s/3600)%24, (s/60)%60, s%60, (unsigned long)(t%1000));
    NVIC_EnableIRQ(UART0_INT_IRQn);
    
    DL_GPIO_setPins(UART_PORT, UART_IRQ_OUT_PIN);
}

/**
 * @brief NACK応答送信（5バイト平文フレーム）
 * 
 * フォーマット: Header(0x90) + LEN(0x01) + ERR_CODE(1バイト) + CRC16(2バイト)
 * 
 * @param err_code エラーコード
 *        0x00: 未確認コマンド
 *        0x01: 処理タイムアウト
 *        0x02: データ内容エラー
 *        0x03: 長さエラー
 *        0x04: CRC16エラー
 *        0xFF: 予期せぬエラー
 * @return true:送信成功, false:送信失敗
 */
static bool send_nack_frame(uint8_t err_code)
{
    S2A_PACKET packet;
    memset(&packet, 0, sizeof(packet));
    
    // NACKパケット構築
    packet.nack.header   = 0x90;
    packet.nack.len      = 0x01;
    packet.nack.err_code = err_code;
    
    // ★仕様書準拠: Data部のみ（1バイト）をCRC計算
    // NACK構造体のData部開始 = Header(1) + LEN(1) の次 = offset 2
    packet.nack.crc16 = crc16_tep((uint8_t*)&packet.nack + 2, 1);  // ERR_CODEのみ
    
    // テストモード: NACK送信記録
    axon_test_record_response(0x90);
    
    // UART送信（高速版 - 応答は即座に返す）
    return uart_send_packet_fast((uint8_t*)&packet.nack, sizeof(NACK_PACKET));
}

/**
 * @brief SETAXON受信時の処理（AXON側）
 * @details SOMAからSETAXONコマンドを受信し、設定後ACK/NACK応答を返す
 * @param encrypted_frame 受信した平文フレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_setaxon(const uint8_t* encrypted_frame)
{
    // テストモード: コマンド受信記録
    axon_test_record_command(0x4A);
    
    if (encrypted_frame == NULL) {
        return false;
    }

    // 1. Header確認
    if (encrypted_frame[0] != 0x14) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 2. LEN確認
    if (encrypted_frame[1] != 0x20) {
        send_nack_frame(0x03);  // 長さエラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（32バイト）を計算対象とする
    uint16_t crc_recv = encrypted_frame[34] | (encrypted_frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&encrypted_frame[2], 32);  // Data部のみ（Byte[2-33]）
    
    if (crc_recv != crc_calc) {
        send_nack_frame(0x04);  // CRC16エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 4. 復号不要
    uint8_t* plain_data = (uint8_t*)&encrypted_frame[2];
    SETAXON_PLAIN32* setaxon = (SETAXON_PLAIN32*)plain_data;
    
    // 5. コマンドID確認
    if (setaxon->id != 0x4A) {
        send_nack_frame(0x00);  // 未確認コマンド
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 6. 設定値妥当性チェック
    // 6-1. FACE番号確認（0-9）
    // 注: 0 = 無効化/初期化状態, 1-9 = 有効な面番号
    if (setaxon->face_n > 9 || setaxon->set_face_n > 9) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 6-2. 金額範囲チェック（0-9999円、100円単位）
    // set_cash_vluは100円単位なので、0-99の範囲が妥当
    if (setaxon->set_cash_vlu > 99) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 6-3. タイムアウト設定範囲チェック（0x0-0xE）
    uint8_t timeout_check = setaxon->set_tout & 0x0F;
    if (timeout_check > 0x0E) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 7. 設定処理
    // SET_SOL (bit0: ソレノイド, bit4: 現金ブロック)
    bool sol_on = (setaxon->set_sol & 0x01) != 0;
    bool cash_block_on = (setaxon->set_sol & 0x10) != 0;
    
    // ソレノイド制御（ダイヤルロック）
    // 注: COIN_SOL_PORTが定義されている場合に有効化
    #ifdef COIN_SOL_PORT
    DL_GPIO_writePinsVal(COIN_SOL_PORT, COIN_SOL_PIN, 
                         sol_on ? COIN_SOL_PIN : 0);
    #endif
    
    // 現金ブロックソレノイド制御
    DL_GPIO_writePinsVal(BLOCK_SOL_PORT, BLOCK_SOL_PIN, 
                         cash_block_on ? BLOCK_SOL_PIN : 0);
    
    // SET_LED (bit0-2: RGB, bit4-6: 動作モード)
    uint8_t led_r = (setaxon->set_led & 0x01) != 0;
    uint8_t led_g = (setaxon->set_led & 0x02) != 0;
    uint8_t led_b = (setaxon->set_led & 0x04) != 0;
    uint8_t led_mode = (setaxon->set_led >> 4) & 0x07;
    
    // RGB LED制御（動作モードを考慮）
    // led_mode: 0b000=消灯, 0b001=点灯, それ以外=消灯（点滅等は未実装）
    if (led_mode == 0x01) {
        // 点灯モード: RGB値に従って色を制御
        if (led_r) DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_0);
        else       DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0);
        
        if (led_g) DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_1);
        else       DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_1);
        
        if (led_b) DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_2);
        else       DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_2);
    } else {
        // 消灯モード（0b000）またはその他: すべて消灯
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_0 | DL_GPIO_PIN_1 | DL_GPIO_PIN_2);
    }
    
    // SET_TOUT (下位4bit: タイムアウト設定)
    uint8_t timeout = setaxon->set_tout & 0x0F;
    
    // タイムアウト値を秒単位に変換してグローバル変数に保存
    const uint16_t timeout_table[] = {30, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 90, 120, 150, 255};
    g_axon_status_shared.timeout_seconds = timeout_table[timeout];
    
    // SET_FACE_N (設定面番号) → 7セグLED右側に表示
    // 0-9すべて有効（0=無効化状態も表示）
    
    // 【面番号重複検出ロジック】
    // ATIRQで送信したpending値とSETAXONで受信した値を比較
    // 不一致の場合、SOMAが現在値（重複のため受け入れ拒否）を返したと判断
    // → pending状態をクリアし、現在値を維持（SOMA-TG仕様書 7.4 準拠）
    
    if (g_pending_right_updated) {
        // ボタン押下後のpending状態
        
        if (g_pending_right_amount != setaxon->set_face_n) {
            // 不一致: ATIRQのL番 ≠ SETAXONのM番
            // → SOMAが重複検出して現在値M番を返した（SOMA-TG仕様書 7.4 ①準拠）
            log_event(LOG_DUPLICATE);
            
            // 受信した値（現在値）を設定
            g_right_amount = setaxon->set_face_n;
            
            // ★自動リトライ: 次の番号(L+1)をpending値に設定
            g_pending_right_amount = (g_pending_right_amount + 1) % 10;
            
            // ★CRITICAL: リトライ要求フラグを設定（メインループで自動IRQ送信）
            g_retry_pending = 1;
            // pending_updatedは維持（リトライ中）
            
            // メモリバリア
            __asm__ volatile("" ::: "memory");
        } else {
            // 一致: 正常受理（SOMA-TG仕様書 7.4 ②③準拠）
            log_event(LOG_ACCEPTED);
            
            g_right_amount = setaxon->set_face_n;
            g_pending_right_amount = g_right_amount;
            
            // メモリバリア
            __asm__ volatile("" ::: "memory");
            
            // ★CRITICAL: pending状態をクリア（正常完了）
            g_pending_right_updated = 0;
            g_retry_pending = 0;
            
            // メモリバリア
            __asm__ volatile("" ::: "memory");
        }
    } else {
        // pending更新なし → 通常の更新処理（初期化時など）
        g_right_amount = setaxon->set_face_n;
        g_pending_right_amount = g_right_amount;
    }
    
    // SET_CASH_VLU (設定金額) → 7セグLED左側に表示
    // 金額は100円単位（0-99の範囲）
    // set_cash_vlu: 0-99 → 金額: 0-9900円
    // 例: set_cash_vlu=5 → 500円 → 左7セグ=5
    // 例: set_cash_vlu=15 → 1500円 → 左7セグ=15（表示不可のため15に制限）
    uint8_t cash_value = setaxon->set_cash_vlu;
    if (cash_value > 99) cash_value = 99;  // 範囲外は99に制限
    
    // 左側7セグ: 百の位（0-9）
    // set_cash_vluの値をそのまま表示（1桁目は百円単位）
    
    // 【金額重複検出ロジック】（面番号と同様の処理）
    if (g_pending_left_updated) {
        if (g_pending_left_amount != cash_value) {
            // 重複検出: ATIRQの金額 ≠ SETAXONの金額
            // ※仕様上、金額の重複チェックは不要だが、安全のため実装
            g_pending_left_amount = (g_pending_left_amount + 1) % 100;
            if (g_pending_left_amount > 99) g_pending_left_amount = 0;
            
            // ★CRITICAL: pending更新フラグを明示的に維持
            g_pending_left_updated = 1;
            
            g_left_amount = g_pending_left_amount;
            // 金額のリトライは不要（面番号のみ）
        } else {
            // 正常受理
            g_left_amount = cash_value;
            g_pending_left_amount = g_left_amount;
            g_pending_left_updated = 0;
        }
    } else {
        // 通常更新
        g_left_amount = cash_value;
        g_pending_left_amount = g_left_amount;
    }
    
    // 7セグLED即座に更新（点滅モード対応）
    extern void update_segment_leds(const uint8_t amount1, const uint8_t amount2);
    update_segment_leds(g_left_amount, g_right_amount);
    
    // 注: 点滅モードの終了は両ボタン同時長押しで実施（axon_routine.c内で処理）
    //     SETAXON受信時は点滅を継続する
    
    // 共有ステータスに設定値を反映（重複検出時も正しい値を設定）
    // ★修正: 重複検出時はpending値、正常受理時は確定値を設定
    if (g_pending_right_updated && g_pending_right_amount != setaxon->set_face_n) {
        // 重複検出時: pending値（次の候補値）を設定
        g_axon_status_shared.face_number = g_pending_right_amount;
    } else {
        // 正常受理時: 確定値を設定
        g_axon_status_shared.face_number = g_right_amount;
    }
    g_axon_status_shared.cash_value = (uint16_t)g_left_amount;
    g_axon_status_shared.solenoid_on = sol_on ? 1U : 0U;
    g_axon_status_shared.block_solenoid_on = cash_block_on ? 1U : 0U;
    g_axon_status_shared.led_pattern = setaxon->set_led;
    
    // リセットフラグをクリア（SETAXONコマンド受信 = 正常動作中）
    g_axon_status_shared.reset_flag = 0;
    
    // コマンド受信トグル（SETAXONコマンド受信毎にトグル）
    g_cmd_recv_toggle ^= 1;
    
    // タイムアウト値を保存（秒単位に変換）
    // タイムアウト値マッピング: 0=30秒, 1=15秒, 2=20秒, ..., 0xE=無限秒
    static const uint8_t timeout_map[16] = {
        30, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 90, 120, 150, 255, 0
    };
    g_axon_status_shared.timeout_seconds = timeout_map[timeout];
    
    // 8. ACK応答送信
    bool ack_result = send_ack_frame();
    
    return ack_result;
}

/**
 * @brief NOP受信時の処理（AXON側）
 * @details SOMAからNOPコマンドを受信し、ACK応答を返す
 * @param encrypted_frame 受信した平文フレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_nop(const uint8_t* encrypted_frame)
{
    // テストモード: コマンド受信記録
    axon_test_record_command(0x50);
    
    if (encrypted_frame == NULL) {
        return false;
    }

    // 1. Header確認
    if (encrypted_frame[0] != 0x14) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 2. LEN確認
    if (encrypted_frame[1] != 0x20) {
        send_nack_frame(0x03);  // 長さエラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（32バイト）を計算対象とする
    uint16_t crc_recv = encrypted_frame[34] | (encrypted_frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&encrypted_frame[2], 32);  // Data部のみ（Byte[2-33]）
    
    if (crc_recv != crc_calc) {
        send_nack_frame(0x04);  // CRC16エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 4. 復号不要（平文のFWコード）
    uint8_t* plain_data = (uint8_t*)&encrypted_frame[2];
    NOP_PLAIN32* nop = (NOP_PLAIN32*)plain_data;
    
    // 5. コマンドID確認
    if (nop->id != 0x50) {
        send_nack_frame(0x00);  // 未確認コマンド
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 6. NOP処理（何もしない）
    // NOPコマンドは単にACKを返すだけ
    
    // 7. ACK応答送信
    bool ack_result = send_ack_frame();
    
    // 8. ACK送信成功後、IRQ信号をHigh(非アクティブ)にクリア
    if (ack_result) {
        clear_irq_signal();
    }
    
    return ack_result;
}

/**
 * @brief SETOKEY受信時の処理（AXON側）
 * @details SOMAからSETOKEY（運用鍵設定）コマンドを受信し、ACK/NACK応答を返す
 * @param frame 受信したフレーム（20バイト: Header[1] + LEN[1] + Key[16] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_setokey(const uint8_t* frame)
{
    if (frame == NULL) {
        return false;
    }

    // 1. Header確認（仕様書Table 4-20: HD=0x15）
    if (frame[0] != 0x15) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 2. LEN確認（仕様書Table 4-20: LEN=0x10=16バイト）
    if (frame[1] != 0x10) {
        send_nack_frame(0x03);  // 長さエラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 3. CRC16チェックなし（仕様書Table 4-20: CRC16フィールドなし）
    // 総フレーム長 = Header(1) + LEN(1) + OKEY(16) = 18バイト

    // 4. 運用鍵データ取得
    const uint8_t* key_data = &frame[2];
    
    // 5. 運用鍵設定処理
    static uint8_t g_operation_key[16] = {0};  // グローバルスコープで永続化
    memcpy(g_operation_key, key_data, 16);
    printf("[SETOKEY] Operation key saved to static storage\n");
    // TODO: 将来的にはFRAMまたはFlashへの永続化を検討
    
    // 6. ACK応答送信
    return send_ack_frame();
}

/**
 * @brief AFWUP受信時の処理（AXON側）
 * @details SOMAからAFWUP（FW更新要求）コマンドを受信し、ACK/NACK応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_afwup(const uint8_t* encrypted_frame)
{
    if (encrypted_frame == NULL) {
        return false;
    }

    // 1. Header確認
    if (encrypted_frame[0] != 0x14) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 2. LEN確認
    if (encrypted_frame[1] != 0x20) {
        send_nack_frame(0x03);  // 長さエラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（32バイト）を計算対象とする
    uint16_t crc_recv = encrypted_frame[34] | (encrypted_frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&encrypted_frame[2], 32);  // Data部のみ（Byte[2-33]）
    
    if (crc_recv != crc_calc) {
        send_nack_frame(0x04);  // CRC16エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 4. 平文データ取得
    uint8_t* plain_data = (uint8_t*)&encrypted_frame[2];
    AFWUP_PLAIN32* afwup = (AFWUP_PLAIN32*)plain_data;
    
    // 5. コマンドID確認
    if (afwup->id != 0x4B) {
        send_nack_frame(0x00);  // 未確認コマンド
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 6. FW更新モード準備
    static volatile bool g_fw_update_mode = false;
    g_fw_update_mode = true;  // FW更新モードフラグをセット
    g_fw_total_crc = 0;       // CRC累積をリセット
    g_fw_last_address = 0;    // 最終アドレスをリセット
    printf("[AFWUP] FW update mode enabled (ready to receive CODEPKT)\n");
    
    // 7. ACK応答送信
    return send_ack_frame();
}

/**
 * @brief AXONRBT受信時の処理（AXON側）
 * @details SOMAからAXONRBT（再起動要求）コマンドを受信し、ACK/NACK応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_axonrbt(const uint8_t* encrypted_frame)
{
    // テストモード: コマンド受信記録
    axon_test_record_command(0x7F);
    
    if (encrypted_frame == NULL) {
        return false;
    }

    // 1. Header確認（仕様書Table 4-40: HD=0x14）
    if (encrypted_frame[0] != 0x14) {
        send_nack_frame(0x02);  // データ内容エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }
    
    // 2. LEN確認（仕様書Table 4-40: LEN=0x20=32バイト）
    if (encrypted_frame[1] != 0x20) {
        send_nack_frame(0x03);  // 長さエラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（32バイト）を計算対象とする
    uint16_t crc_recv = encrypted_frame[34] | (encrypted_frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&encrypted_frame[2], 32);  // Data部のみ（Byte[2-33]）
    
    if (crc_recv != crc_calc) {
        send_nack_frame(0x04);  // CRC16エラー
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 4. 平文データ取得
    uint8_t* plain_data = (uint8_t*)&encrypted_frame[2];
    AXONRBT_PLAIN32* axonrbt = (AXONRBT_PLAIN32*)plain_data;
    
    // 5. コマンドID確認（仕様書Table 4-40: ID=0x7F）
    if (axonrbt->id != 0x7F) {
        send_nack_frame(0x00);  // 未確認コマンド
        clear_irq_signal();  // IRQクリア（NACK送信後）
        return false;
    }

    // 6. ACK応答送信
    bool ack_result = send_ack_frame();
    
    // 7. ACK送信成功後、IRQ信号をHigh(非アクティブ)にクリア
    if (ack_result) {
        clear_irq_signal();
    }
    
    // 8. 再起動処理（仕様書: 10秒後に再起動）
    if (ack_result) {
        printf("[AXONRBT] System reboot requested. Rebooting in 10 seconds...\n");
        // 注意: 実際の再起動はNVIC_SystemReset()を使用
        // TODO: 10秒待機後に再起動を実装
        // delay_ms(10000);
        // NVIC_SystemReset();
    }
    
    return ack_result;
}

/**
 * @brief CODEPKT受信時の処理（AXON側）
 * @details SOMAからCODEPKT（コードパケット）を受信し、CODEOK/CODENG応答を返す
 * @param frame 受信したフレーム（40バイト: Header[1] + LEN[1] + Address[4] + Code[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_codepkt(const uint8_t* frame)
{
    if (frame == NULL) {
        return false;
    }

    // 1. Header確認
    if (frame[0] != 0xA5) {
        // NACK送信は不要（仕様書記載なし）
        return false;
    }
    
    // 2. LEN確認
    if (frame[1] != 0x24) {
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（36バイト: Address[4] + Code[32]）を計算対象とする
    uint16_t crc_recv = frame[38] | (frame[39] << 8);
    uint16_t crc_calc = crc16_tep(&frame[2], 36);  // Data部のみ（Byte[2-37]）
    
    if (crc_recv != crc_calc) {
        // CRC NGの場合、CODENG送信
        CODENG_PACKET codeng;
        memset(&codeng, 0, sizeof(codeng));
        
        codeng.header = 0xBD;
        codeng.len = 0x04;
        // アドレスは受信データから取得（LSB first）
        memcpy(&codeng.address, &frame[2], 4);
        // ★Data部のみCRC計算: Address[4]
        codeng.crc16 = crc16_tep((uint8_t*)&codeng + 2, 4);
        
        return uart_send_packet((uint8_t*)&codeng, sizeof(CODENG_PACKET));
    }

    // 4. アドレスとFWコード取得
    uint32_t address;
    memcpy(&address, &frame[2], 4);
    const uint8_t* fw_code = &frame[6];
    
    // 5. FWコード書き込み処理（シミュレーション版）
    // 警告: 実際のFlash書き込みは慎重に実装する必要がある（誤書き込みでブリック）
    static uint8_t g_fw_buffer[2048] = {0};  // FWバッファ（シミュレーション用）
    bool write_success = false;
    
    // アドレス連続性チェック（初回または連続していることを確認）
    if (g_fw_last_address == 0 || address == g_fw_last_address + 32) {
        uint32_t offset = address % sizeof(g_fw_buffer);
        if (offset + 32 <= sizeof(g_fw_buffer)) {
            memcpy(&g_fw_buffer[offset], fw_code, 32);
            g_fw_last_address = address;
            write_success = true;
            printf("[CODEPKT] FW data buffered at offset 0x%08X (real flash write disabled for safety)\n", offset);
        } else {
            printf("[CODEPKT] ERROR: Buffer overflow prevented\n");
        }
    } else {
        printf("[CODEPKT] ERROR: Address discontinuity detected (expected 0x%08X, got 0x%08X)\n", 
               g_fw_last_address + 32, address);
    }
    
    // 6. 書き込み結果に応じて応答送信
    if (write_success) {
        // CODEOK送信
        CODEOK_PACKET codeok;
        memset(&codeok, 0, sizeof(codeok));
        
        codeok.header = 0xB4;
        codeok.len = 0x04;
        codeok.address = address;
        // ★Data部のみCRC計算: Address[4]
        codeok.crc16 = crc16_tep((uint8_t*)&codeok + 2, 4);
        
        return uart_send_packet((uint8_t*)&codeok, sizeof(CODEOK_PACKET));
    } else {
        // CODENG送信
        CODENG_PACKET codeng;
        memset(&codeng, 0, sizeof(codeng));
        
        codeng.header = 0xBD;
        codeng.len = 0x04;
        codeng.address = address;
        // ★Data部のみCRC計算: Address[4]
        codeng.crc16 = crc16_tep((uint8_t*)&codeng + 2, 4);
        
        return uart_send_packet((uint8_t*)&codeng, sizeof(CODENG_PACKET));
    }
}

/**
 * @brief ERRCHK受信時の処理（AXON側）
 * @details SOMAからERRCHK（エラーチェック）を受信し、CODEFIN応答を返す
 * @param frame 受信したフレーム（6バイト: Header[1] + LEN[1] + WholeCRC[2] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_errchk(const uint8_t* frame)
{
    if (frame == NULL) {
        return false;
    }

    // 1. Header確認
    if (frame[0] != 0xC4) {
        return false;
    }
    
    // 2. LEN確認
    if (frame[1] != 0x02) {
        return false;
    }

    // 3. CRC16チェック（リトルエンディアン形式で読み取り）
    // ★仕様書準拠: Data部のみ（2バイト: WholeCRC[2]）を計算対象とする
    uint16_t crc_recv = frame[4] | (frame[5] << 8);
    uint16_t crc_calc = crc16_tep(&frame[2], 2);  // Data部のみ（Byte[2-3]）
    
    if (crc_recv != crc_calc) {
        return false;
    }

    // 4. 受信したFW全体のCRC16取得（ビッグエンディアン形式で読み取り）
    uint16_t rx_whole_crc = (frame[2] << 8) | frame[3];
    
    // 5. 計算したFW全体のCRC16と比較
    uint16_t calc_whole_crc = (uint16_t)(g_fw_total_crc & 0xFFFF);
    
    // 6. CODEFIN応答送信
    CODEFIN_PACKET codefin;
    memset(&codefin, 0, sizeof(codefin));
    
    codefin.header = 0xD4;
    codefin.len = 0x04;
    codefin.rx_crc16 = rx_whole_crc;
    codefin.calc_crc16 = calc_whole_crc;
    // ★Data部のみCRC計算: RxCRC[2] + CalcCRC[2] = 4バイト
    codefin.crc16 = crc16_tep((uint8_t*)&codefin + 2, 4);
    
    bool result = uart_send_packet((uint8_t*)&codefin, sizeof(CODEFIN_PACKET));
    
    // 7. CRCが一致していれば、FW更新完了処理
    if (result && (rx_whole_crc == calc_whole_crc)) {
        static volatile bool g_fw_update_complete = false;
        g_fw_update_complete = true;  // FW更新完了フラグをセット
        printf("[ERRCHK] FW update completed (CRC verification passed)\n");
        // 注意: 実際の再起動は NVIC_SystemReset() を使用（現在は安全のため無効化）
        // NVIC_SystemReset();
    }
    
    return result;
}

#endif // defined(AXON_BOARD)