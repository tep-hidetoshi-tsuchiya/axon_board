# 推測が必要な項目・質問リスト

**作成日**: 2026年1月19日
**ステータス**: ❓ 回答待ち
**対象フェーズ**: Phase 1-3

---

## ⚠️ Phase 1 開始前の重要質問

### Q1: rotation_count の使用・削除範囲の確認

**現在の状況**:
- s2a_packet.h: `#define S2A_STATUS_GET_ROTATION_COUNT(status)` マクロあり
- uart_comm_axon.c Line 1055-1058: `S2A_STATUS_GET_ROTATION_COUNT()` 使用
- uart_comm_tg.c Line 1316: `fram_config.prize_counter` を rotation_count として送信
- atirq_status.h Line 93: `atirq_status_get_rotation_count()` 関数あり
- atirq_status.c Line 158-186: 実装あり

**質問**:
1. ❓ S2A_STATUS_GET_ROTATION_COUNT() マクロは **削除してよい** でしょうか？
   - または、**コメントアウト（互換性保持）** が必要ですか？

2. ❓ atirq_status_get_rotation_count() 関数は **削除してよい** でしょうか？
   - 呼び出し箇所確認済み（uart_comm_axon.c Line 1055のみ）
   - 他の隠れた使用箇所はありますか？

3. ❓ uart_comm_tg.c Line 1316 の修正について
   - 現在: `status_payload.port_status[idx].rotation_count = fram_config.prize_counter;`
   - Ver1.0では: rotation_count をどこから取得すればいいですか？
   - オプション A: `atirq_port_status_t.rotation_count` を使用（STATUS[15:8]=RFU）
   - オプション B: `fram_config.prize_counter` をそのまま使用（互換性）
   - オプション C: 別の値を使用

**判定基準**: デグレ禁止制約があるため、TG への STATUS 送信内容の変更は慎重に必要

---

### Q2: FRAM prize_counter の管理について

**現在の状況**:
- FRAM に prize_counter が存在（ポート別管理）
- purchase_handler.c で回転検出時に prize_counter++

**質問**:
1. ❓ Latch式の Ver1.0 では、prize_counter はどのタイミングで更新されますか？
   - 現在（Ver0.1): AXON STATUS[15:8]=rotation_count受信時→別途フロー不明
   - Ver1.0: STATUS bit7(Latch式)検知時？

2. ❓ prize_counter の初期化タイミング：
   - 起動時のみ？
   - 面番号変更時に初期化される？
   - 他に初期化ポイントはある？

3. ❓ prize_counter は TG 側でも同期管理が必要ですか？
   - または、SOMA 側ローカルのみ？

**判定基準**: rotation_count 削除による FRAM 書込み/読込み タイミングの不整合を避けるため

---

### Q3: SET_SOL → SET_PRTS の影響範囲確認

**現在の状況**:
- s2a_packet.h Line 59-63: SET_SOL ビット定義あり
  ```c
  #define S2A_SETAXON_SOL_SOLENOID        (1 << 0)
  #define S2A_SETAXON_SOL_COIN_SENSOR_RST (1 << 3)
  #define S2A_SETAXON_SOL_CASH_BLOCK      (1 << 4)
  ```

**質問**:
1. ❓ SET_SOL ビット定義の削除は安全ですか？
   - 他のファイルで `SET_SOL` 文字列で検索 → 使用箇所確認済み（コメント除く）？

2. ❓ SET_PRTS への名称変更時、既存コードの互換性は不要ですか？
   - または、マクロ定義でのエイリアス `#define SET_SOL SET_PRTS` 等の方が安全？

3. ❓ S2A_SETAXON_SOL_SOLENOID (bit0) について
   - Ver1.0仕様では bit0 は RFU（使用禁止）
   - 現在のコードでbit0を使用している箇所は？
   - **削除してよいか、または用途変更が必要ですか？**

**判定基準**: 既存ロジックでbit0が使用されている場合、機能変更の可能性

---

### Q4: atirq_status 構造体の更新スコープ

**現在の状況**:
- atirq_port_status_t 構造体 (Line 100-109):
  ```c
  typedef struct {
      uint8_t rotation_count;
      bool soldout, cashless_sol, coin_sensor, coin_return, cash_block, door_open;
  } atirq_port_status_t;
  ```

**質問**:
1. ❓ rotation_count フィールドは **削除してよい** でしょうか？
   - または、**廃止予定（deprecated）フィールド** として残す？

2. ❓ Ver1.0で追加すべき新フィールドはありますか？
   - 例: `bool dial_rotated;` (STATUS bit7)
   - 他にありますか？

3. ❓ atirq_port_status_t を使用しているコード（構造体の依存者）：
   - uart_comm_tg.c 行番号？
   - その他のファイル？

**判定基準**: 構造体の削除/追加は API 互換性に影響するため慎重に

---

## ⚠️ Phase 2-3 開始前の推測が必要な項目

### Q5: Latchクリアの実装パターン確認

**現在の状況**:
- 仕様書：Latchクリアは SET_PRTS ビット設定で実行
- 実装ガイド：soma_send_setaxon() で実装予定

**質問**:
1. ❓ soma_send_setaxon() 関数の署名（既存コード確認済み）:
   - Location: uart_comm_axon.c Line 914
   - 関数: `static esp_err_t soma_send_setaxon(uint8_t port_num, const S2A_SETAXON_PAYLOAD* payload)`
   - この関数を使用してよいですか？（static → public への変更は必要？）

2. ❓ purchase_handler.c の Latchクリア実装について：
   - Line 246-261, 288-304 にLatクリア例があります
   - これらの既存ロジックは **そのまま使用可能** でしょうか？
   - または、Ver1.0 仕様に合わせて修正が必要ですか？

3. ❓ SET_PRTS 複数ビット同時クリア（OR演算）について：
   - 実装ガイド Page 298 の例：bit2 と bit5 を同時に ON
   - AXON側で複数ビット同時ON は正常に動作しますか？
   - 順序・タイミングの制約はありますか？

---

### Q6: AXON Reset (soma_send_axonrbt) の実装について

**現在の状況**:
- uart_comm_axon.c に関数 soma_send_axonrbt() あり (Line 1142)
- 実装ガイド：ドア OPEN 時に使用予定

**質問**:
1. ❓ soma_send_axonrbt() の現在の実装は **そのまま使用可能** ですか？
   - 関数名、引数、戻り値、リトライロジックを確認してよいですか？

2. ❓ AXON Reset 後の処理フロー：
   - Reset 後 → SETAXON で状態復帰？
   - Reset 後 → 遅延（何ms？）が必要？
   - 仕様書の「10秒」ルールは実装に反映されていますか？

3. ❓ ドア OPEN 時の処理タイミング：
   - いつ soma_send_axonrbt() を呼び出しますか？
   - TG からのコマンド？
   - SOMA 独自判定？

---

## 📋 回答チェックリスト

### Phase 1 開始前の必須確認

- ⬜ **Q1**: rotation_count マクロ・関数の削除/保持 判定
- ⬜ **Q2**: prize_counter の更新タイミング確認
- ⬜ **Q3**: SET_SOL → SET_PRTS の影響範囲確認
- ⬜ **Q4**: atirq_status 構造体の更新スコープ

### Phase 2 開始前の推測排除

- ⬜ **Q5**: Latchクリア実装パターンの確認
- ⬜ **Q6**: AXON Reset 実装の詳細確認

---

## 📝 回答記入欄

**以下に回答をお願いします**:

```
【Q1 回答】
- rotation_count マクロ: [削除 / 保持 / コメントアウト]
- atirq_status_get_rotation_count(): [削除 / 保持]
- uart_comm_tg.c Line 1316 修正案: [A / B / C / その他]

【Q2 回答】
- prize_counter 更新タイミング (Ver1.0):
- prize_counter 初期化タイミング:
- TG側との同期: [必要 / 不要]

【Q3 回答】
- SET_SOL マクロ削除: [可能 / 注意あり]
- bit0 使用状況: [使用なし / 使用あり→場所]
- 互換性保持方法: [削除のみ / エイリアス定義 / 他]

【Q4 回答】
- rotation_count フィールド削除: [可能 / 廃止予定フィールド化]
- Ver1.0 新フィールド: [なし / あり→フィールド名]
- atirq_port_status_t の依存者: [ファイル/行番号]

【Q5 回答】
- soma_send_setaxon() 使用: [可能 / static のまま / public化必要]
- 既存 Latchクリア例の使用: [そのまま可 / 修正必要]
- 複数ビット同時ON の制約: [なし / あり→内容]

【Q6 回答】
- soma_send_axonrbt() 使用: [そのまま可 / 修正必要]
- Reset 後処理: [SETAXON復帰 / 遅延→ms / 他]
- 呼び出しタイミング: [TGコマンド / SOMA判定 / 他]
```

---

**作成**: AI Assistant
**最終更新**: 2026年1月19日
