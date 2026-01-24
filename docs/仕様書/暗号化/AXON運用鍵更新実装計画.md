# AXON運用鍵更新実装計画書 v3.2

**作成日**: 2026年1月24日  
**対象基板**: AXON基板 (TI MSPM0G3507)  
**プロジェクト**: SOMA-AXON運用鍵更新機能  
**ベース仕様**: 通信シーケンス仕様書 第15章「運用鍵更新シーケンス」  

**重要な前提条件:**
- ✅ MSPM0G3507にはFRAMなし（Flashメモリのみ）
- ✅ **今回は運用鍵の永続化は実装しない**（RAM上での管理のみ）
- ✅ 設定鍵はソースコードに埋め込み
- ✅ **SOMA-AXON間でもチャレンジ・レスポンス機能が必要**(TG-SOMA間と同様のシーケンス)
- ✅ AESはソフト実装（tiny-AES-c）のAES-256/ECBのみを使用（MSPM0ハードAESや既存AES-128/CBCコードは不使用）

---

## 📋 目次

1. [現状分析](#1-現状分析)
2. [要求仕様](#2-要求仕様)
3. [設計方針](#3-設計方針)
4. [実装計画](#4-実装計画)
5. [リスク評価](#5-リスク評価)
6. [テスト計画](#6-テスト計画)

---

## 1. 現状分析

### 1.1. 仕様書の要求事項

**AXON側の役割:**
- SOMAから新運用鍵を受信(SETOKEYコマンド)
- 設定鍵で暗号化されたSETOKEYコマンドを受信・復号
- 運用鍵を更新してACK応答
- **チャレンジ・レスポンス機能の実装**(TG-SOMA間と同様のシーケンス)
  - CHALLENGEパケット送信: 32バイト乱数を設定鍵で暗号化
  - RESPONSEパケット受信: 新運用鍵で暗号化された乱数を検証
  - 乱数が一致すれば新運用鍵を正式採用

### 1.2. 現在の実装状況

**実装ファイル:** [s2a_packet.c#L1141](../../../axon_board/protocol/src/s2a_packet.c#L1141)

```c
bool axon_handle_setokey(const uint8_t* frame)
{
    // 現在の実装:
    // - Header: 0x15
    // - LEN: 0x10 (16バイト)
    // - フレーム長: 18バイト
    // - 暗号化: なし
    // - CRC16: なし
}
```

**問題点と対応方針:**
1. ❌ 鍵長が16バイト（仕様は**32バイト**）
    - 対応: 32バイト鍵前提に仕様/処理フローを修正済み（実装はPhase 3で反映）
2. ❌ 暗号化処理なし（仕様は**設定鍵で暗号化**）
    - 対応: 設定鍵によるAES-256/ECB復号を処理フローに明記（実装はPhase 3）
3. ❌ CRC16検証なし（仕様は**必須**）
    - 対応: Data部CRC16検証を必須ステップとして追加（実装はPhase 3）

---

## 2. 要求仕様

### 2.1. SETOKEYコマンド仕様（SOMA→AXON）

**パケット構造:**

| フィールド | バイト位置 | サイズ | 値 | 説明 |
|-----------|-----------|--------|-----|------|
| Header | 1st | 1 byte | 0x15 | コマンド識別子 |
| LEN | 2nd | 1 byte | 0x20 | データ長(32バイト) |
| OKEY | 3rd ~ 34th | 32 bytes | [暗号化データ] | 新運用鍵（設定鍵で暗号化） |
| CRC16 | 35th ~ 36th | 2 bytes | [0:65535] | CRC16（Data部のみ） |

**総フレーム長:** 36バイト

**暗号化方式:**
- **アルゴリズム**: AES-256 ECB
- **暗号化対象**: 3rd Byte ~ 34th Byte（32バイトの新運用鍵）
- **使用鍵**: 設定鍵（32バイト、ソースコード埋め込み）

**処理フロー:**
1. フレーム受信（36バイト）
2. Header/LEN検証
3. CRC16検証（Data部: 3rd~34th Byte）
4. 暗号化データを設定鍵で**復号**
5. 復号結果を新運用鍵として一時保存
6. **チャレンジ・レスポンスシーケンス開始**
   - 32バイト乱数生成
   - 乱数を設定鍵で暗号化してCHALLENGEパケット送信
   - RESPONSEパケット受信(3秒以内)
   - RESPONSEを新運用鍵で復号し乱数を検証
   - 乱数一致で新運用鍵を正式採用
7. ACK応答送信(新運用鍵で通信再開)

### 2.2. 鍵管理要件

**鍵の種類:**
- **設定鍵（Setup Key）**: 32バイト
  - ソースコードに埋め込み（const配列）
  - 変更不可
  - 用途: SETOKEYコマンドの復号

- **運用鍵（Operation Key）**: 32バイト
  - SETOKEYコマンドで更新可能
  - **RAM上で管理**（今回は永続化なし）
  - 電源断で初期値にリセット
  - 将来の通信暗号化に使用予定

---

## 3. 設計方針

### 3.1. アーキテクチャ（簡略版）

```
┌─────────────────────────────────────────────────┐
│              AXON Application Layer             │
│        (axon_routine.c, s2a_packet.c)          │
└────────────────┬────────────────────────────────┘
                 │ 静的グローバル/ヘルパー関数
┌────────────────▼────────────────────────────────┐
│      Key Storage (static globals)               │
│  - 設定鍵/運用鍵をRAM保持（グローバル配列）        │
│  - 乱数生成ヘルパー（チャレンジ用）                │
└────────────────┬────────────────────────────────┘
                 │ 暗号化API
┌────────────────▼────────────────────────────────┐
│         AES-256 Crypto Module                   │
│            (aes256.h/c)                        │
│  - AES-256 ECBモード                            │
│  - Tiny-AES-c ライブラリ使用                     │
└─────────────────────────────────────────────────┘
```

### 3.2. モジュール設計

#### 3.2.1. AES-256 Crypto Module

**方針:** ソフト実装（tiny-AES-c）によるAES-256/ECBのみを使用。MSPM0GのハードAES（128bit/CBC）や soma_uart_test.c のAES-128/CBCコードは使用しない。

**API:**
```c
// aes256.h
typedef enum {
    AES256_SUCCESS = 0,
    AES256_ERROR_NULL_POINTER,
    AES256_ERROR_INVALID_KEY_SIZE
} aes256_status_t;

aes256_status_t aes256_decrypt_ecb(
    const uint8_t* ciphertext,  // 暗号化データ（32バイト）
    const uint8_t* key,         // 暗号鍵（32バイト）
    uint8_t* plaintext          // 平文データ出力（32バイト）
);
```

#### 3.2.2. Key Storage（静的グローバル）

**方針:** 専用モジュールは作らず、`s2a_packet.c` 内の静的グローバルで保持。必要なヘルパー関数のみを同ファイル内に実装。

**データ構造（s2a_packet.c 内）:**
```c
#define KEY_SIZE_BYTES 32

static const uint8_t g_setup_key[KEY_SIZE_BYTES] = { /* 固定設定鍵 */ };
static uint8_t g_operation_key[KEY_SIZE_BYTES];
static bool g_operation_key_init = false;
```

**ヘルパー関数（例）:**
```c
static bool get_setup_key(uint8_t* key_out);
static bool get_operation_key(uint8_t* key_out);
static bool update_operation_key(const uint8_t* new_key);
static bool generate_random32(uint8_t* random_out);  // 32バイト乱数
static bool ensure_operation_key_initialized(void);  // 初期値セット
```

#### 3.2.3. SETOKEY Handler

```c
bool axon_handle_setokey(const uint8_t* frame)
{
    // 1. 基本検証
    if (!frame || frame[0] != 0x15 || frame[1] != 0x20) {
        return send_nack_frame(0x02);
    }

    // 2. CRC16検証
    uint16_t crc_recv = frame[34] | (frame[35] << 8);
    uint16_t crc_calc = crc16_tep(&frame[2], 32);
    if (crc_recv != crc_calc) {
        return send_nack_frame(0x04);
    }

    // 3. 設定鍵取得
    uint8_t setup_key[32];
    if (!get_setup_key(setup_key)) {
        return send_nack_frame(0x05);
    }

    // 4. 復号（設定鍵使用）
    uint8_t new_operation_key[32];
    if (aes256_decrypt_ecb(&frame[2], setup_key, new_operation_key) != AES256_SUCCESS) {
        return send_nack_frame(0x06);
    }

    // 5. チャレンジ・レスポンスシーケンス
    uint8_t challenge_random[32];
    if (!generate_random32(challenge_random)) {
        return send_nack_frame(0x07);
    }

    // 6. CHALLENGEパケット送信(乱数を設定鍵で暗号化)
    uint8_t challenge_encrypted[32];
    if (aes256_encrypt_ecb(challenge_random, setup_key, challenge_encrypted) != AES256_SUCCESS) {
        return send_nack_frame(0x08);
    }
    if (!send_challenge_packet(challenge_encrypted)) {
        return send_nack_frame(0x09);
    }

    // 7. RESPONSEパケット受信(3秒タイムアウト)
    uint8_t response_frame[34];  // Header(1) + LEN(1) + Data(32) + CRC16(2)
    if (!wait_for_response_packet(response_frame, 3000)) {
        return send_nack_frame(0x0A);  // タイムアウト
    }

    // 8. RESPONSE検証
    uint8_t response_decrypted[32];
    if (aes256_decrypt_ecb(&response_frame[2], new_operation_key, response_decrypted) != AES256_SUCCESS) {
        return send_nack_frame(0x0B);
    }

    // 9. 乱数一致確認
    if (memcmp(challenge_random, response_decrypted, 32) != 0) {
        return send_nack_frame(0x0C);  // 乱数不一致
    }

    // 10. 運用鍵更新(RAM上)
    if (!update_operation_key(new_operation_key)) {
        return send_nack_frame(0x0D);
    }

    // 11. ACK応答(新運用鍵で暗号化)
    return send_ack_frame();
}
```

---

## 4. 実装計画

### 4.1. 実装フェーズ

#### **Phase 1: AES-256暗号化ライブラリの統合**

**目標:** AES-256 ECBモード復号化機能を提供

**タスク:**
1. Tiny-AES-cライブラリのダウンロード
   - GitHub: https://github.com/kokke/tiny-AES-c
   - ファイル: aes.c, aes.h

2. プロジェクトへの統合
   - `axon_board/driver/crypto/` ディレクトリ作成
    - ラッパー関数実装（aes256.h/c）
    - ハードウェアAES(DL_AES)は使用しない（鍵長128bit固定のため）
    - soma_uart_test.c のAES-128/CBC実装は参考のみで流用しない

3. 単体テスト
   - NIST AES-256テストベクターで検証

**所要時間:** 1-2日

---

#### **Phase 2: 鍵管理（グローバル変数管理）**

**目標:** RAM上での鍵管理（静的グローバル）

**タスク:**
1. 設定鍵の定義
    - `s2a_packet.c` 内にconst配列で定義
    - テスト用ダミー値を設定

2. 運用鍵のRAM管理
    - 静的グローバル配列で保持
    - 初期化ヘルパー実装（未初期化時にデフォルト値セット）

3. 乱数生成機能
    - MSPM0Gのハードウェア乱数生成器またはソフトPRNG実装
    - 32バイト乱数生成ヘルパー

4. ヘルパー実装
    - 取得・更新・乱数生成関数（static関数）
    - エラーハンドリング

**所要時間:** 1-1.5日

---

#### **Phase 3: SETOKEY処理の拡張**

**目標:** 仕様準拠のSETOKEYコマンド処理

**タスク:**
1. パケット構造定義更新
   ```c
   // s2a_packet.h
   #define S2A_HEADER_SETOKEY     0x15
   #define S2A_SETOKEY_LEN        0x20  // 32バイト
   #define S2A_SETOKEY_FRAME_SIZE 36
   
   #define S2A_HEADER_CHALLENGE   0x11
   #define S2A_CHALLENGE_LEN      0x20  // 32バイト
   #define S2A_CHALLENGE_FRAME_SIZE 34
   
   #define S2A_HEADER_RESPONSE    0x11
   #define S2A_RESPONSE_LEN       0x20  // 32バイト
   #define S2A_RESPONSE_FRAME_SIZE 34
   ```

2. `axon_handle_setokey()` 拡張
   - 36バイトフレーム対応
   - CRC16検証追加
   - AES-256復号処理統合
   - **CHALLENGE/RESPONSEシーケンス実装**

3. CHALLENGE/RESPONSEパケット処理
   - `send_challenge_packet()` 実装
   - `wait_for_response_packet()` 実装(3秒タイムアウト)
   - 乱数検証ロジック

4. 可変長フレーム受信処理の更新
   ```c
   // axon_routine.c
   if (header == 0x15 && length == 0x20 && rx_variable_length == 36) {
       handled = axon_handle_setokey(rx_variable_frame);
   }
   else if (header == 0x11 && length == 0x20 && rx_variable_length == 34) {
       handled = axon_handle_response(rx_variable_frame);
   }
   ```

**所要時間:** 1.5-2日

---

#### **Phase 4: 統合テスト**

**タスク:**
1. Pythonテストスクリプト作成
   - SETOKEYコマンド送信
   - CHALLENGE受信・検証
   - RESPONSE送信
   - 新運用鍵での通信確認

2. 正常系・異常系テスト
   - 正常系: 有効なSETOKEY + CHALLENGE/RESPONSE → ACK
   - 異常系: Header/LEN不正 → NACK
   - 異常系: CRC不一致 → NACK
   - 異常系: 復号失敗 → NACK
   - 異常系: RESPONSEタイムアウト → NACK
   - 異常系: 乱数不一致 → NACK

3. 性能測定
   - SETOKEY処理時間(目標: 10ms以内)
   - CHALLENGE/RESPONSE全体時間(目標: 50ms以内)

**所要時間:** 1.5-2日

---

### 4.2. ファイル構成

```
axon_board/
├── driver/
│   └── crypto/
│       ├── aes.h                       ← Tiny-AES-c
│       ├── aes.c                       ← Tiny-AES-c
│       ├── aes256.h                    ← ラッパー
│       ├── aes256.c                    ← ラッパー
├── protocol/src/
│   ├── s2a_packet.h                    ← 更新
│   └── s2a_packet.c                    ← 更新（鍵管理の静的グローバル/ヘルパー含む）
└── axon_routine.c                      ← 更新

tools/
└── test_setokey.py                     ← 新規作成
```

---

## 5. リスク評価

### 5.1. 技術的リスク

| リスク | 影響 | 発生確率 | 対策 |
|--------|------|---------|------|
| AES暗号化のFlash/RAM不足 | 高 | 中 | Tiny-AES-c採用（約2KB） |
| 暗号化処理時間オーバーヘッド | 中 | 低 | 5ms以内を目標 |
| 設定鍵の漏洩 | 高 | 低 | デバッグ出力禁止 |
| 運用鍵の揮発性 | 中 | 高 | 仕様として認識（電源断でリセット） |
| 既存AES-128/CBCコードの混在 | 中 | 低 | soma_uart_test.cは流用せず、ビルド対象外にする |

---

## 6. テスト計画

### 6.1. 単体テスト

#### AES-256モジュール
- NIST AES-256テストベクター検証
- NULL ポインタ処理

#### 鍵管理（グローバル）
- 運用鍵更新・読み出し
- 設定鍵の不変性確認

### 6.2. 統合テスト

**テストケース:**
1. 正常系: 有効なSETOKEYコマンド + CHALLENGE/RESPONSE → ACK
2. 異常系: Header/LEN不正 → NACK
3. 異常系: CRC不一致 → NACK
4. 異常系: 復号失敗 → NACK
5. 異常系: RESPONSEタイムアウト → NACK
6. 異常系: 乱数不一致 → NACK

---

## 7. 実装優先度

| Priority | Task | Complexity | Impact |
|----------|------|------------|--------|
| P0 | 32バイト鍵対応 | Low | High |
| P0 | CRC16検証追加 | Low | High |
| P1 | AES-256復号処理 | Medium | High |
| P1 | 鍵管理機能（RAM） | Low | Medium |
| P3 | Flash永続化 | High | Low（将来実装） |

---

## 8. マイルストーン

| Phase | タスク | 期間 | 完了日（予定） |
|-------|--------|------|---------------|
| Phase 1 | AES-256ライブラリ統合 | 1-2日 | 2026/1/26 |
| Phase 2 | 鍵管理(グローバル変数)+乱数生成 | 1-1.5日 | 2026/1/27 |
| Phase 3 | SETOKEY処理拡張+CHALLENGE/RESPONSE | 1.5-2日 | 2026/1/29 |
| Phase 4 | 統合テスト | 1.5-2日 | 2026/1/31 |
| - | **総計** | **5-7.5日** | **2026/1/31** |

---

## 変更履歴

| 版 | 日付 | 変更内容 | 作成者 |
|----|------|---------|--------|
| 1.0 | 2026/1/24 | 初版作成 | GitHub Copilot |
| 2.0 | 2026/1/24 | Flash永続化を削除、RAM管理のみに変更 | GitHub Copilot |
| 2.1 | 2026/1/24 | ハードAES/既存AES-128/CBCを不使用と明記、ソフトAES-256/ECB専用に統一 | GitHub Copilot |
| 3.0 | 2026/1/24 | SOMA-AXON間でもチャレンジ・レスポンス機能が必要と修正、実装詳細を追加 | GitHub Copilot |
| 3.1 | 2026/1/24 | CHALLENGEパケットのデータサイズを16バイトから32バイトに修正 | GitHub Copilot |
| 3.2 | 2026/1/24 | 問題点1-3の対応方針を明示（32B鍵/AES-256+設定鍵/CRC16必須） | GitHub Copilot |

---

**次のステップ:**
1. 本計画書のレビュー・承認
2. Phase 1実装開始（AES-256ライブラリ統合）

