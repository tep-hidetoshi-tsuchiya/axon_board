# Latchクリア実装詳細ガイド

**作成日**: 2026年1月19日
**目的**: SOMA-AXON Ver1.0のLatchクリア機能実装方法を詳細に記述
**参照**: 03_実装変更ガイド.md, 通信シーケンス仕様書.md

---

## 1. 既存インターフェースの確認結果

### 1.1 SETAXON送信関数

**関数シグネチャ**:
```c
// uart_comm_axon.h Line 88
bool soma_send_setaxon(uint8_t port_num, const S2A_SETAXON_PAYLOAD* payload);
```

**ペイロード構造体**:
```c
// s2a_packet.h Line 202-212
typedef struct __S2A_SETAXON_PAYLOAD {
    uint8_t  face_n;         // 面番号
    uint8_t  set_sol;        // ★ Ver1.0で set_prts に変更必要
    uint8_t  set_led;        // LED設定
    uint8_t  set_tout;       // タイムアウト設定
    uint8_t  set_face_n;     // 面番号設定
    uint16_t set_cash_vlu;   // 金額設定
    uint16_t auth_code;      // 認証コード
    uint16_t rnd;            // 乱数
} S2A_SETAXON_PAYLOAD;
```

**関数動作**:
1. MUX切替（ポート選択）
2. SETAXONパケット構築・送信
3. ACK/NACK受信待ち（リトライ最大5回）
4. MUXリリース
5. 成功/失敗を返却

---

### 1.2 AXONRBT送信関数

**関数シグネチャ**:
```c
// uart_comm_axon.h Line 110
bool soma_send_axonrbt(uint8_t port_num);
```

**関数動作**:
```c
// uart_comm_axon.c Line 1142-1190
bool soma_send_axonrbt(uint8_t port_num) {
    // 1. MUX切替
    if (uart_comm_axon_select_port(port_num) != ESP_OK) {
        return false;
    }

    // 2. AXONRBTパケット構築
    S2A_PACKET pkt;
    uint16_t auth_code, rnd;
    get_auth_params(&auth_code, &rnd);
    s2a_packet_build_axonrbt(&pkt, auth_code, rnd);

    // 3. 送信（リトライ最大5回）
    for (int retry = 0; retry < S2A_MAX_RETRY_COUNT; retry++) {
        axon_uart_send_frame((T2S_PACKET*)&pkt, PACKET_SIZE);
        if (wait_for_response(response_buf, S2A_TIMEOUT_MS)) {
            response_received = true;
            break;
        }
    }

    // 4. ACK/NACK確認
    if (response_buf[0] == T2S_HEADER_ACK) {
        ESP_LOGI(TAG, "AXONRBT ACK受信 (再起動開始)");
        success = true;
    }

    // 5. MUXリリース
    axon_uart_release_bus();
    return success;
}
```

**注意事項**:
- AXON再起動は送信後**約10秒後**に実行される
- 再起動後はATIRQ MODE bit7=1（リセットフラグ）が立つ
- 再初期化シーケンス（CHKIRQ → ATIRQ → SETAXON）が必要

---

## 2. Latchクリア実装パターン

### 2.1 単一ビットクリア（ダイヤル回転のみ）

**使用例**: rotation_detector.c でダイヤル回転検知後のクリア

```c
// rotation_detector.c (提案実装)
bool rotation_detector_process(rotation_detector_t *detector, const S2A_ATIRQ_PACKET *atirq) {
    bool dial_rotated = (atirq->status & S2A_STATUS_DIAL_ROTATE) != 0;

    // 立ち上がりエッジ検知
    if (dial_rotated && !detector->prev_dial_rotated) {
        // FRAM prize_counter +1
        soma_data_increment_prize_counter(detector->port_num);

        // ★ Latchクリア送信
        S2A_SETAXON_PAYLOAD payload = {
            .face_n = 0,  // 現在の面番号（FRAMから取得推奨）
            .set_prts = S2A_SETAXON_PRTS_DIAL_CLEAR,  // bit5=1
            .set_led = 0x00,
            .set_tout = 0x00,
            .set_face_n = 0,
            .set_cash_vlu = 0,
            .auth_code = 0,
            .rnd = 0
        };
        soma_send_setaxon(detector->port_num, &payload);

        detector->prev_dial_rotated = dial_rotated;
        return true;
    }

    detector->prev_dial_rotated = dial_rotated;
    return false;
}
```

---

### 2.2 複数ビット同時クリア（現金センサー + ダイヤル回転）

**使用例**: 通信シーケンス仕様書 シーケンス7（現金決済）Line 387-388

```c
// purchase_handler.c (修正例)
// シーケンス7: 現金決済完了後のLatchクリア
void purchase_handler_reset_cash_latch(uint8_t port_num, uint8_t face_n, uint16_t cash_vlu) {
    // SET_PRTS bit2: 現金センサーLatchクリア
    // SET_PRTS bit5: ダイヤル回転Latchクリア
    uint8_t set_prts = S2A_SETAXON_PRTS_COIN_SENSOR_RST | S2A_SETAXON_PRTS_DIAL_CLEAR;

    S2A_SETAXON_PAYLOAD payload = {
        .face_n = face_n,
        .set_prts = set_prts,  // bit2=1, bit5=1
        .set_led = 0x16,       // 水色点灯
        .set_tout = 0x00,
        .set_face_n = face_n,
        .set_cash_vlu = cash_vlu,
        .auth_code = 0,
        .rnd = 0
    };

    soma_send_setaxon(port_num, &payload);
}
```

---

### 2.3 複数ビット同時クリア（現金センサー + 返却ボタン）

**使用例**: 通信シーケンス仕様書 シーケンス9（返金）Line 543-545

```c
// purchase_handler.c Line 288-304 (既存コード修正)
// 修正前:
uint8_t set_sol = S2A_SETAXON_SOL_COIN_SENSOR_RST | S2A_SETAXON_SOL_CASH_BLOCK;  // 0x18

// 修正後:
uint8_t set_prts = S2A_SETAXON_PRTS_COIN_SENSOR_RST | S2A_SETAXON_PRTS_COIN_RETURN_RST;
//                                   bit2=1                        bit3=1

S2A_SETAXON_PAYLOAD payload = {
    .face_n = context->face_n,
    .set_prts = set_prts,  // bit2=1（現金センサー）+ bit3=1（返却ボタン）
    .set_led = 0x16,       // 水色点灯
    .set_tout = 0x00,
    .set_face_n = context->face_n,
    .set_cash_vlu = context->cash_vlu,
    .auth_code = 0,
    .rnd = 0
};

soma_send_setaxon(context->port_num, &payload);
```

---

## 3. 実装上の注意事項

### 3.1 フィールド名の変更

**重要**: 構造体フィールド名は `set_sol` から `set_prts` に変更必須

```c
// ❌ 旧実装（Ver0.1）
S2A_SETAXON_PAYLOAD payload;
payload.set_sol = 0x08;  // コンパイルエラー（フィールド存在しない）

// ✅ 新実装（Ver1.0）
S2A_SETAXON_PAYLOAD payload;
payload.set_prts = S2A_SETAXON_PRTS_COIN_SENSOR_RST;
```

**修正対象ファイル**:
1. `s2a_packet.h` Line 202-212: 構造体定義
2. `s2a_packet.c` Line 286: send_setaxon_packet() 引数
3. `uart_comm_axon.c` Line 939: send_setaxon_packet() 呼び出し
4. `purchase_handler.c` Line 56, 92-99, 246-304: 各使用箇所
5. `axon_irq_handler.c` Line 96: 使用箇所

---

### 3.2 ビットマスクの組み立て

**OR演算子で複数ビット設定**:
```c
// 例1: bit2 + bit5
uint8_t set_prts = S2A_SETAXON_PRTS_COIN_SENSOR_RST | S2A_SETAXON_PRTS_DIAL_CLEAR;
// → 0x04 | 0x20 = 0x24

// 例2: bit2 + bit3
uint8_t set_prts = S2A_SETAXON_PRTS_COIN_SENSOR_RST | S2A_SETAXON_PRTS_COIN_RETURN_RST;
// → 0x04 | 0x08 = 0x0C

// 例3: bit4のみ
uint8_t set_prts = S2A_SETAXON_PRTS_CASH_BLOCK;
// → 0x10
```

**RFUビットの誤使用防止**:
```c
// ❌ 誤り: bit0はVer1.0でRFU
uint8_t set_prts = 0x01;  // bit0 = 電子マネー用ソレノイド（Ver0.1で廃止）

// ✅ 正しい: Ver1.0定義マクロのみ使用
uint8_t set_prts = S2A_SETAXON_PRTS_DIAL_CLEAR;  // bit5
```

---

### 3.3 face_n/set_face_n/set_cash_vluの設定

**推奨実装**:
```c
// FRAMから現在設定を取得
fram_axon_data_t fram_config;
soma_data_get_config(port_num, &fram_config);

S2A_SETAXON_PAYLOAD payload = {
    .face_n = fram_config.face_num,      // 現在の面番号
    .set_prts = latch_clear_bits,        // Latchクリアビット
    .set_led = 0x16,                     // 水色点灯
    .set_tout = 0x00,                    // 30秒タイムアウト
    .set_face_n = fram_config.face_num,  // 面番号設定
    .set_cash_vlu = fram_config.cash,    // 金額設定
    .auth_code = 0,
    .rnd = 0
};
```

**理由**:
- AXONは`face_n`と`set_face_n`が一致しないと設定を拒否する場合がある
- `set_cash_vlu`も現在値を保持すべき（意図しない金額リセット防止）

---

## 4. AXON Reset実装詳細

### 4.1 TG SETAXON受信時のReset検出

**実装箇所**: uart_comm_tg.c SETAXON受信処理（Line 712-758）

```c
void handle_t2s_setaxon(const T2S_SETAXON_PACKET *packet) {
    uint8_t face_num = packet->face_num;
    uint8_t t2s_set_face = packet->set_face;  // Ver1.2

    // Ver1.2ビット詳細ログ
    ESP_LOGI(TAG, "SET_FACE=0x%02X [Reset=%d, DialClr=%d, Block=%d, BtnRst=%d, SnsrRst=%d]",
             t2s_set_face,
             (t2s_set_face & T2S_SETAXON_FACE_RESET) ? 1 : 0,        // bit7
             (t2s_set_face & T2S_SETAXON_FACE_DIAL_CLEAR) ? 1 : 0,   // bit5
             (t2s_set_face & T2S_SETAXON_FACE_CASH_BLOCK) ? 1 : 0,   // bit4
             (t2s_set_face & T2S_SETAXON_FACE_BTN_RESET) ? 1 : 0,    // bit3
             (t2s_set_face & T2S_SETAXON_FACE_SENSOR_RST) ? 1 : 0);  // bit2

    // ★ Bit 7: AXON Reset検出
    if (t2s_set_face & T2S_SETAXON_FACE_RESET) {
        uint8_t axon_port = face_num_to_port(face_num);

        ESP_LOGI(TAG, "AXON Reset requested for FACE_NUM %d (PORT %d)", face_num, axon_port);

        // AXONRBTコマンド送信
        if (soma_send_axonrbt(axon_port)) {
            ESP_LOGI(TAG, "AXONRBT sent successfully, waiting 10 seconds for reboot...");

            // 10秒待機（AXON再起動時間）
            vTaskDelay(pdMS_TO_TICKS(10000));

            // AXON再初期化シーケンス
            axon_reinitialize_after_reboot(axon_port);

        } else {
            ESP_LOGE(TAG, "AXONRBT failed for PORT %d", axon_port);
            // TGへNACK応答（エラーコード設定）
            send_tg_nack(T2S_NACK_ERR_FATAL_ERROR);
            return;
        }
    }

    // Bit 7以外の制御ビットをマッピング
    uint8_t s2a_set_prts = protocol_map_t2s_to_s2a_setaxon(t2s_set_face);

    // SOMA→AXON SETAXON送信
    // ...
}
```

---

### 4.2 AXON再初期化シーケンス

**実装提案**:
```c
/**
 * @brief AXON再起動後の再初期化シーケンス
 *
 * 1. CHKIRQ送信
 * 2. ATIRQ受信（MODE bit7=1を確認）
 * 3. FRAM設定読み出し
 * 4. SETAXON送信（面番号・金額・LED復元）
 */
esp_err_t axon_reinitialize_after_reboot(uint8_t port_num) {
    ESP_LOGI(TAG, "Starting AXON reinitialize sequence for PORT %d", port_num);

    // 1. CHKIRQ送信 → ATIRQ受信
    if (!soma_send_chkirq(port_num)) {
        ESP_LOGE(TAG, "CHKIRQ failed during reboot reinit");
        return ESP_FAIL;
    }

    // 2. ATIRQ解析（MODE bit7=1確認）
    S2A_ATIRQ_PACKET atirq;
    if (wait_for_atirq(&atirq, 1000) != ESP_OK) {
        ESP_LOGE(TAG, "ATIRQ timeout during reboot reinit");
        return ESP_FAIL;
    }

    if (!(atirq.mode & S2A_MODE_RESET_FLAG)) {
        ESP_LOGW(TAG, "ATIRQ MODE bit7=0 (reset flag not set)");
    }

    // 3. FRAM設定復元
    fram_axon_data_t fram_config;
    if (soma_data_get_config(port_num, &fram_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read FRAM config");
        return ESP_FAIL;
    }

    // 4. SETAXON送信（設定復元）
    S2A_SETAXON_PAYLOAD payload = {
        .face_n = fram_config.face_num,
        .set_prts = 0x00,  // Latchクリアなし
        .set_led = 0x16,   // 水色点灯（デフォルト）
        .set_tout = 0x00,  // 30秒タイムアウト
        .set_face_n = fram_config.face_num,
        .set_cash_vlu = fram_config.cash,
        .auth_code = 0,
        .rnd = 0
    };

    if (!soma_send_setaxon(port_num, &payload)) {
        ESP_LOGE(TAG, "SETAXON failed during reboot reinit");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "AXON reinitialize completed for PORT %d", port_num);
    return ESP_OK;
}
```

---

## 5. テストケース

### 5.1 単一ビットクリアテスト

**TC-01: ダイヤル回転Latchクリア**

```
前提条件:
  - AXON PORT 1 接続済み
  - STATUS bit7=1（ダイヤル回転検知）

実行手順:
  1. rotation_detector_process() 呼び出し
  2. soma_send_setaxon() 呼び出し確認（set_prts=0x20）
  3. AXON STATUS bit7=0 確認（Latchクリア完了）

期待結果:
  - FRAM prize_counter +1
  - SET_PRTS bit5=1 送信
  - STATUS bit7=0（クリア成功）
```

---

### 5.2 複数ビットクリアテスト

**TC-02: 現金決済後のLatchクリア**

```
前提条件:
  - AXON PORT 1 接続済み
  - STATUS bit3=0（現金投入中）、bit7=1（ダイヤル回転検知）

実行手順:
  1. purchase_handler_reset_cash_latch() 呼び出し
  2. soma_send_setaxon() 呼び出し確認（set_prts=0x24）
  3. AXON STATUS bit3=1, bit7=0 確認

期待結果:
  - SET_PRTS bit2=1, bit5=1 送信
  - STATUS bit3=1（現金なし）
  - STATUS bit7=0（ダイヤル回転クリア）
```

---

### 5.3 AXON Resetテスト

**TC-03: TGからのReset要求**

```
前提条件:
  - AXON PORT 1 接続済み、正常動作中

実行手順:
  1. TGからSETAXON受信（SET_FACE bit7=1）
  2. soma_send_axonrbt(1) 呼び出し
  3. 10秒待機
  4. CHKIRQ送信 → ATIRQ受信（MODE bit7=1確認）
  5. SETAXON送信（設定復元）

期待結果:
  - AXONRBT ACK受信
  - 10秒後にAXON再起動完了
  - ATIRQ MODE bit7=1
  - 面番号・金額設定復元完了
```

---

## 6. まとめ

### 6.1 実装完了チェックリスト

- [x] **既存関数確認完了**
  - soma_send_setaxon() 使用可能
  - soma_send_axonrbt() 使用可能

- [ ] **フィールド名変更**
  - s2a_packet.h 構造体定義（set_sol → set_prts）
  - 全使用箇所の修正（5ファイル）

- [ ] **Latchクリア実装**
  - rotation_detector.c: 単一ビットクリア
  - purchase_handler.c: 複数ビット同時クリア（2箇所）

- [ ] **AXON Reset実装**
  - uart_comm_tg.c: TG SETAXON受信処理
  - axon_reinitialize_after_reboot() 新規実装

- [ ] **テスト実施**
  - TC-01: 単一ビットクリア
  - TC-02: 複数ビットクリア
  - TC-03: AXON Reset

### 6.2 推定工数（追加分）

| 項目 | 工数 |
|-----|------|
| フィールド名変更 | 1時間 |
| Latchクリア実装 | 2時間 |
| AXON Reset実装 | 2時間 |
| テスト実施 | 1時間 |
| **合計** | **6時間** |

---

**作成者**: AI Assistant
**レビュー**: 未実施
**承認**: 未実施
