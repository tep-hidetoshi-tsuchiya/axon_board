#ifndef __S2A_PACKET_H__
#define __S2A_PACKET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

// AES初期化関数（AXON起動時に1回だけ呼ぶ）
void axon_aes_init(void);

#define PACKET_DATA_LEN     0x20
#define PACKET_DIC_LEN      0x02
#define PACKET_AUTHCODE_LEN 0x02
#define PACKET_RND_LEN      0x02
#define PACKET_CRC16_LEN    0x02
#define PACKET_SIZE         (1 + 1 + PACKET_DATA_LEN + PACKET_CRC16_LEN)

// 仕様準拠: CHKIRQ平文32バイト
typedef struct __attribute__((packed)) {
    uint8_t  id;        // 0x49
    uint8_t  rfu[25];   // all 0
    uint16_t dic;       // 0x0123 (LE)
    uint16_t auth_code; // 認証コード
    uint16_t rnd;       // 乱数
} CHKIRQ_PLAIN32;
_Static_assert(sizeof(CHKIRQ_PLAIN32) == 32, "CHKIRQ_PLAIN32 size");

// 仕様準拠: ATIRQ平文32バイト (Table 4-13, 4-14準拠)
typedef struct __attribute__((packed)) {
    uint8_t  id;        // 3rd:  0x6A
    uint8_t  md;        // 4th:  モード通知
    uint8_t  face_n;    // 5th:  FACE_N (bit[3:0]=面番号, bit[7:4]=RFU)
    uint16_t cash_vlu;  // 6-7th: 設定金額 (LE, 100円単位)
    uint16_t status;    // 8-9th: STATUS (LE, bit0=FACE有効, bit1=売切れ, bit6=ドア開閉)
    uint8_t  msn[6];    // 10-15th: AXON基板シリアル番号 (SSN)
    uint8_t  afw_ver;   // 16th: FWバージョン (bit[7:4]=Major, bit[3:0]=Minor)
    uint8_t  chk_led;   // 17th: LED状態 (bit[6:4]=動作, bit[2:0]=RGB)
    uint8_t  chk_tout;  // 18th: タイムアウト設定 (0x0=30秒)
    uint8_t  rfu[10];   // 19-28th: RFU（10バイト、ALL 0固定）
    uint16_t dic;       // 29-30th: データ整合性チェック (LE)
    uint16_t auth_code; // 31-32th: 認証コード (LE)
    uint16_t rnd;       // 33-34th: 乱数 (LE)
} ATIRQ_PLAIN32;
_Static_assert(sizeof(ATIRQ_PLAIN32) == 32, "ATIRQ_PLAIN32 size");

// 仕様準拠: NOP平文32バイト
typedef struct __attribute__((packed)) {
    uint8_t  id;        // 0x50
    uint8_t  rfu[25];   // all 0
    uint16_t dic;       // データ整合性チェック
    uint16_t auth_code; // 認証コード
    uint16_t rnd;       // 乱数
} NOP_PLAIN32;
_Static_assert(sizeof(NOP_PLAIN32) == 32, "NOP_PLAIN32 size");

// ACK - Acknowledgement response (HD:0x10, ID:0x00)
// 正常応答
// SOMA-AXON specification
typedef struct __ACK_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED ACK_PACKET;

// NACK - Negative acknowledgement response (HD:0x90, ID:ERR_CODE)
// 異常応答
// SOMA-AXON specification
typedef struct __NACK_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  err_code;
    uint16_t crc16;
} __PACKED NACK_PACKET;

// NOP - No Operation command (HD:0x10, ID:0x10)
// No Operationコマンド
// SOMA-AXON specification
typedef struct __NOP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED NOP_PACKET;

// SETAXON - AXON board setting command (HD:0x14, ID:0x4A)
// AXON基板設定コマンド
// SOMA-AXON specification
typedef struct __SET_AXON_REQ_PACKET {
    uint8_t  header;           // 0x14
    uint8_t  len;              // 0x20
    uint8_t  id;               // 0x4A
    uint8_t  face_n;           // FACE番号 (1-9)
    uint8_t  set_sol;          // ソレノイド/現金ブロック設定
    uint8_t  set_led;          // LED設定
    uint8_t  set_tout;         // タイムアウト設定
    uint8_t  rfu1[2];          // RFU
    uint8_t  set_face_n;       // 設定面番号
    uint16_t set_cash_vlu;     // 設定金額
    uint8_t  rfu2[16];         // RFU
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SET_AXON_REQ_PACKET;

// SETAXON平文32バイト（暗号化前）
typedef struct __attribute__((packed)) {
    uint8_t  id;               // 0x4A
    uint8_t  face_n;           // FACE番号 (1-9)
    uint8_t  set_sol;          // ソレノイド/現金ブロック設定
    uint8_t  set_led;          // LED設定
    uint8_t  set_tout;         // タイムアウト設定
    uint8_t  rfu1[2];          // RFU
    uint8_t  set_face_n;       // 設定面番号
    uint16_t set_cash_vlu;     // 設定金額
    uint8_t  rfu2[16];         // RFU
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
} SETAXON_PLAIN32;
_Static_assert(sizeof(SETAXON_PLAIN32) == 32, "SETAXON_PLAIN32 size");

// AFWUP - AXON board firmware update request (HD:0x14, ID:0x18)
// AXON基板FWアップデート要求コマンド
// SOMA-AXON specification
typedef struct __AXON_FIRM_UPDATE_REQ_PACKET {
    uint8_t  header;           // 0x14
    uint8_t  len;              // 0x20
    uint8_t  id;               // 0x18
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED AXON_FIRM_UPDATE_REQ_PACKET;

// AFWUP平文32バイト（暗号化前）
typedef struct __AFWUP_PLAIN32 {
    uint8_t  id;               // 0x18
    uint8_t  rfu[25];          // RFU
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
} __PACKED AFWUP_PLAIN32;
_Static_assert(sizeof(AFWUP_PLAIN32) == 32, "AFWUP_PLAIN32 size");

// SETOKEY - Operational key setting command (HD:0x11, LEN:0x12)
// 運用鍵設定コマンド
// SOMA-AXON specification
typedef struct __SET_OP_KEY_PACKET {
    uint8_t  header;           // 0x11
    uint8_t  len;              // 0x12 (18 bytes)
    uint8_t  key[16];          // 運用鍵16バイト
    uint16_t crc16;
} __PACKED SET_OP_KEY_PACKET;

// ============================================================
// SOMA-AXON internal communication packets
// ============================================================
// ATIRQ - Interrupt response (HD:0x10, ID:0x50)
// 割込み応答
// SOMA-AXON specification
typedef struct __ATIRQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  mode;
    uint8_t  face_n;
    uint16_t cash_vlu;
    uint8_t  status;
    uint8_t  msn[6];
    uint8_t  afw_ver;
    uint8_t  chk_led;
    uint8_t  chk_tout;
    uint8_t  rfu[11];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED ATIRQ_PACKET;

// CHKIRQ - Check interrupt request (HD:0x10, ID:0x49)
// 割込み確認リクエスト
// SOMA-AXON specification
typedef struct __CHKIRQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  face_n;
    uint8_t  chk_irq;
    uint8_t  rfu[23];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED CHKIRQ_PACKET;

// SETTBL - Setting table command (HD:0x10, ID:0x6A)
// テーブル設定コマンド（構造体定義のみ、実装なし）
// SOMA-AXON specification
typedef struct __SETTBL_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  face_n;
    uint16_t cash_vlu;
    uint8_t  set_led;
    uint8_t  set_tout;
    uint8_t  rfu[20];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SETTBL_PACKET;

// ATSET - Setting response (HD:0x10, ID:0x4A)
// 設定応答
// SOMA-AXON specification
typedef struct __ATSET_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED ATSET_PACKET;

// GETFACE - Get FACE information (HD:0x10, ID:0x4B)
// FACE情報取得（構造体定義のみ、実装なし）
// SOMA-AXON specification
typedef struct __GETFACE_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  face_n;
    uint8_t  rfu[24];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED GETFACE_PACKET;

// TSTEND - Test end command (HD:0x10, ID:0x7F)
// テスト終了コマンド（構造体定義のみ、実装なし）
// SOMA-AXON specification
typedef struct __TSTEND_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED TSTEND_PACKET;

// ============================================================
// Firmware update commands
// ============================================================

// CODEPKT - Code packet for firmware update
// コードパケット
// SOMA-AXON specification (HD:0xA5)
typedef struct __CODEPKT_PACKET {
    uint8_t  header;        // 0xA5
    uint8_t  len;           // 0x24 (36 bytes)
    uint32_t address;       // FW address (LSB first)
    uint8_t  fw_code[32];   // Firmware code (encrypted)
    uint16_t crc16;
} __PACKED CODEPKT_PACKET;

// CODEPKTデータ（address + fw_code）
typedef struct __CODEPKT_DATA36 {
    uint32_t address;       // FW address (LSB first)
    uint8_t  fw_code[32];   // Firmware code (encrypted)
} __PACKED CODEPKT_DATA36;
_Static_assert(sizeof(CODEPKT_DATA36) == 36, "CODEPKT_DATA36 size");

// CODEOK - Code reception OK response
// コード受信OKレスポンス
// SOMA-AXON specification (HD:0xB4)
typedef struct __CODEOK_PACKET {
    uint8_t  header;        // 0xB4
    uint8_t  len;           // 0x04 (4 bytes)
    uint32_t address;       // FW address received
    uint16_t crc16;
} __PACKED CODEOK_PACKET;

// CODENG - Code reception NG response
// コード受信NGレスポンス
// SOMA-AXON specification (HD:0xBD)
typedef struct __CODENG_PACKET {
    uint8_t  header;        // 0xBD
    uint8_t  len;           // 0x04 (4 bytes)
    uint32_t address;       // FW address received
    uint16_t crc16;
} __PACKED CODENG_PACKET;

// ERRCHK - Error check command
// エラー確認コマンド
// SOMA-AXON specification (HD:0xC4)
typedef struct __ERRCHK_PACKET {
    uint8_t  header;        // 0xC4
    uint8_t  len;           // 0x02 (2 bytes)
    uint16_t whole_code_crc16;  // CRC16 of entire FW code
    uint16_t crc16;
} __PACKED ERRCHK_PACKET;

// CODEFIN - Code completion response
// コード完了レスポンス
// SOMA-AXON specification (HD:0xD4)
typedef struct __CODEFIN_PACKET {
    uint8_t  header;        // 0xD4
    uint8_t  len;           // 0x04 (4 bytes)
    uint16_t rx_crc16;      // Received CRC16
    uint16_t calc_crc16;    // Calculated CRC16
    uint16_t crc16;
} __PACKED CODEFIN_PACKET;

// ============================================================
// Packet union (AXON board で使用するパケットのみ)
// ============================================================
typedef union __S2A_PACKET {
    ACK_PACKET                  ack;
    NACK_PACKET                 nack;
    NOP_PACKET                  nop;
    SET_AXON_REQ_PACKET         set_axon_req;
    AXON_FIRM_UPDATE_REQ_PACKET axon_firm_update_req;
    SET_OP_KEY_PACKET           set_op_key;
    ATIRQ_PACKET                atirq;
    CHKIRQ_PACKET               chkirq;
    SETTBL_PACKET               settbl;
    ATSET_PACKET                atset;
    GETFACE_PACKET              getface;
    TSTEND_PACKET               tstend;
    CODEPKT_PACKET              codepkt;
    CODEOK_PACKET               codeok;
    CODENG_PACKET               codeng;
    ERRCHK_PACKET               errchk;
    CODEFIN_PACKET              codefin;
} __PACKED S2A_PACKET;

// ============================================================
// Function Declarations
// ============================================================

#if defined(AXON_BOARD)
/**
 * @brief CHKIRQ受信時の処理（AXON側）
 * @details SOMAからCHKIRQコマンドを受信し、ATIRQ応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_chkirq(const uint8_t* encrypted_frame);

/**
 * @brief SETAXON受信時の処理（AXON側）
 * @details SOMAからSETAXONコマンドを受信し、設定後ACK/NACK応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_setaxon(const uint8_t* encrypted_frame);

/**
 * @brief NOP受信時の処理（AXON側）
 * @details SOMAからNOPコマンドを受信し、ACK応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_nop(const uint8_t* encrypted_frame);

/**
 * @brief SETOKEY受信時の処理（AXON側）
 * @details SOMAからSETOKEYコマンドを受信し、運用鍵を設定してACK/NACK応答を返す
 * @param frame 受信したフレーム（20バイト: Header[1] + LEN[1] + Key[16] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_setokey(const uint8_t* frame);

/**
 * @brief AFWUP受信時の処理（AXON側）
 * @details SOMAからAFWUP（FW更新要求）を受信し、ACK/NACK応答を返す
 * @param encrypted_frame 受信したフレーム（36バイト: Header[1] + LEN[1] + Data[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_afwup(const uint8_t* encrypted_frame);

/**
 * @brief CODEPKT受信時の処理（AXON側）
 * @details SOMAからCODEPKT（コードパケット）を受信し、CODEOK/CODENG応答を返す
 * @param frame 受信したフレーム（40バイト: Header[1] + LEN[1] + Address[4] + Code[32] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_codepkt(const uint8_t* frame);

/**
 * @brief ERRCHK受信時の処理（AXON側）
 * @details SOMAからERRCHK（エラーチェック）を受信し、CODEFIN応答を返す
 * @param frame 受信したフレーム（6バイト: Header[1] + LEN[1] + WholeCRC[2] + CRC[2]）
 * @return true: 成功, false: 失敗
 */
bool axon_handle_errchk(const uint8_t* frame);

// デバッグ変数（CCSデバッガーで監視可能）
extern volatile uint32_t debug_chkirq_call_count;
extern volatile uint32_t debug_header_ng_count;
extern volatile uint32_t debug_len_ng_count;
extern volatile uint32_t debug_crc_ng_count;
extern volatile uint32_t debug_cmd_ng_count;
extern volatile uint32_t debug_decrypt_ng_count;
extern volatile uint32_t debug_encrypt_ng_count;
extern volatile uint32_t debug_send_ok_count;
extern volatile uint16_t debug_crc_recv;
extern volatile uint16_t debug_crc_calc;
extern volatile uint8_t debug_decrypted_id;
extern volatile uint8_t debug_decrypted_axon_num;
#endif

#ifdef __cplusplus
}
#endif
#endif  // __S2A_PACKET_H__