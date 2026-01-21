# SOMAプロトコル仕様書バージョンアップ 移行ドキュメント

**最終更新日**: 2026年1月19日
**ステータス**: 統合完了

このフォルダには、SOMA-TG Ver1.2 および SOMA-AXON Ver1.0 への移行に関するドキュメントがまとめられています。

---

## 🔍 対象仕様書バージョン変更

| プロトコル | 旧バージョン | 新バージョン | 変更内容 |
|-----------|----------|----------|----------|
| **SOMA-TG** | **Ver1.0** | **Ver1.2** | SETAXON/ATCKSB ビット定義拡張（Bit 5,3,2追加） |
| **SOMA-AXON** | **Ver0.1** | **Ver1.0** | ATIRQ/SETAXON Latch式センサー導入、rotation_count削除 |

---

## 📚 主要ドキュメント（3つに統合）

### 🔵 1. [01_SOMA-AXON差分仕様_Ver0.1→Ver1.0.md](01_SOMA-AXON差分仕様_Ver0.1→Ver1.0.md)

**目的**: SOMA-AXON プロトコルの Ver0.1 から Ver1.0 への詳細な変更内容

**内容**:
- ATIRQ STATUS フィールド変更（Bit 7追加）
- SETAXON コマンド変更（SET_SOL→SET_PRTS、ビット定義拡張）
- [15:8] RFU仕様の明確化
- rotation_count削除の理由
- Latch式センサークリア機能追加
- 下位互換性評価

**対象読者**: プロトコル仕様を理解したい開発者、システムアーキテクト

**ページ数**: 約470行

---

### 🔵 2. [02_SOMA-TG差分仕様_Ver1.0→Ver1.2.md](02_SOMA-TG差分仕様_Ver1.0→Ver1.2.md)

**目的**: SOMA-TG プロトコルの Ver1.0 から Ver1.2 への詳細な変更内容

**内容**:
- SETAXON SET_FACE ビット定義拡張（Bit 5,3,2追加）
- ATCKSB CHK_FACE 全ビット定義（2ビット→8ビット）
- CHKAXON コマンド仕様（変更なし）
- ビット対応表とマッピング
- 実装への影響分析

**対象読者**: プロトコル仕様を理解したい開発者、TG側開発者

**ページ数**: 約430行

---

### 🔵 3. [03_実装変更ガイド.md](03_実装変更ガイド.md)

**目的**: 新仕様への実装変更の完全ガイド

**内容**:
1. **変更概要**: rotation_countの扱い、データフロー変更
2. **実装変更箇所一覧**: 10ファイル、通信区間別マップ
3. **優先度別実装手順**:
   - 🔴 フェーズ1: 最優先（2-3時間）
   - 🔴 フェーズ2: 高優先（4-5時間）
   - 🟡 フェーズ3: 中優先（3-4時間）
4. **ファイル別変更詳細**: 具体的なコード変更内容
5. **データフローとFRAM影響**: FRAM操作への影響評価
6. **テスト計画**: 単体/統合/回帰テスト
7. **リスクと対策**: 修正漏れ対策、検証方法
8. **実装チェックリスト**: フェーズ別タスクリスト

**対象読者**: 実装担当開発者

**推定工数**: 合計 12-16時間

**ページ数**: 約800行

---

## 🎯 ドキュメント使用ガイド

### 新規開発者向け

1. **まず読むべき**: 本README
2. **仕様理解**: 01_SOMA-AXON差分仕様 → 02_SOMA-TG差分仕様
3. **実装準備**: 03_実装変更ガイド

### 実装担当者向け

1. **最優先**: 03_実装変更ガイド のフェーズ1
2. **並行確認**: 01, 02 で仕様詳細確認
3. **テスト**: 03_実装変更ガイド のテスト計画参照

### レビュアー向け

1. **差分確認**: 01, 02 で仕様変更理解
2. **実装確認**: 03_実装変更ガイド のチェックリスト使用
3. **整合性確認**: データフロー図で全体把握

---

## 📊 ドキュメント統合前後の比較

### 統合前（10ファイル）

| ファイル | 内容 | 統合先 |
|---------|------|--------|
| SOMA-AXON_ATIRQ_SETAXON_v0.1_vs_v1.0_diff.md | SOMA-AXON差分 | ✅ 01 |
| SOMA-TG_SETAXON_CHKAXON_ATCKSB_v1.0_vs_v1.2_diff.md | SOMA-TG差分 | ✅ 02 |
| IMPLEMENTATION_CHANGE_SUMMARY.md | 変更箇所一覧 | ✅ 03 |
| IMPLEMENTATION_STATUS_REPORT_20260119.md | 実装状態 | ✅ 03 |
| FRAM_DATAFLOW_CONSISTENCY_REPORT_20260119.md | FRAM影響 | ✅ 03 |
| DATA_FLOW_VERIFICATION_REPORT_20260119.md | データフロー | ✅ 03 |
| MIGRATION_PLAN_V1.0_TO_V1.2.md | 移行計画 | ✅ 03 |
| PROTOCOL_INTEGRATION_ANALYSIS.md | 統合分析 | ✅ 03 |
| VERIFICATION_REPORT_20260119.md | 検証レポート | ✅ 03 |
| README.md | インデックス | ✅ 本ファイル |

### 統合後（4ファイル）

1. **01_SOMA-AXON差分仕様_Ver0.1→Ver1.0.md** (470行)
2. **02_SOMA-TG差分仕様_Ver1.0→Ver1.2.md** (430行)
3. **03_実装変更ガイド.md** (800行)
4. **README.md** (本ファイル)

**メリット**:
- ✅ 必要な情報が3つのドキュメントに集約
- ✅ 仕様と実装が明確に分離
- ✅ 実装ガイドが体系的に整理
- ✅ 重複情報の削除

---

## 🚀 実装開始クイックスタート

### ステップ1: 環境確認

```powershell
# ワークスペース移動
cd C:\DATA\DEVELOP\Git\Synapse_wifi_module

# ブランチ作成（推奨）
git checkout -b feature/protocol-v1.0-v1.2
```

### ステップ2: 最優先修正（30分）

```c
// 1. uart_comm_tg.c Line 1337
uint16_t status_word = 0;  // [15:8] = RFU

// 2. uart_comm_axon.c Line 1055
bool dial_rotated = (atirq->status & S2A_STATUS_DIAL_ROTATE) != 0;

// 3. s2a_packet.h
#define S2A_STATUS_DIAL_ROTATE (1 << 7)
```

### ステップ3: コンパイル確認

```powershell
idf.py build
```

### ステップ4: 詳細実装

**03_実装変更ガイド.md**の「3. 優先度別実装手順」に従って実施

---

## 🔗 関連ドキュメント（参考）

### アーカイブされた詳細ドキュメント

以下のドキュメントは03_実装変更ガイドに統合されましたが、詳細確認用に保持されています：

- `IMPLEMENTATION_CHANGE_SUMMARY.md` - 変更箇所一覧（詳細版）
- `IMPLEMENTATION_STATUS_REPORT_20260119.md` - 実装状態レポート
- `FRAM_DATAFLOW_CONSISTENCY_REPORT_20260119.md` - FRAM影響詳細
- `DATA_FLOW_VERIFICATION_REPORT_20260119.md` - データフロー検証
- `MIGRATION_PLAN_V1.0_TO_V1.2.md` - 7フェーズ移行計画
- `PROTOCOL_INTEGRATION_ANALYSIS.md` - プロトコル統合分析
- `VERIFICATION_REPORT_20260119.md` - 検証レポート

**用途**: 深堀り調査、トラブルシューティング時の参照

---

## 📝 ドキュメント更新履歴

| 日付 | バージョン | 変更内容 |
|-----|----------|---------|
| 2026/01/19 | 2.0 | 10ファイルを3ファイルに統合、README全面改訂 |
| 2026/01/19 | 1.5 | FRAM影響レポート追加 |
| 2026/01/19 | 1.4 | データフロー検証レポート追加 |
| 2026/01/19 | 1.3 | 実装状態レポート追加 |
| 2026/01/18 | 1.2 | 統合分析レポート追加 |
| 2026/01/18 | 1.1 | 移行計画追加 |
| 2026/01/18 | 1.0 | 初版作成 |

---

## 📚 ドキュメント一覧（旧版）

### 1. 主要ドキュメント

#### [MIGRATION_PLAN_V1.0_TO_V1.2.md](MIGRATION_PLAN_V1.0_TO_V1.2.md)
**目的**: Ver1.0からVer1.2への完全な移行計画書

**内容**:
- エグゼクティブサマリー
- 7フェーズの実装計画（約9時間）
- ビット定義マクロ追加手順
- SETAXON/ATCKSB送受信処理の更新
- マッピング関数実装
- テスト計画（単体/統合/回帰）
- リスクと対策

**対象読者**: 実装担当開発者

---

#### [PROTOCOL_INTEGRATION_ANALYSIS.md](PROTOCOL_INTEGRATION_ANALYSIS.md)
**目的**: TG-SOMA-AXON間のプロトコル統合分析

**内容**:
- TG-SOMA (T2S) とSAMA-AXON (S2A) のプロトコル相関関係
- SETAXON/ATCKSB/ATIRQ の完全なビットマッピング表
- データフロー図（テキストベース）
- プロトコル変換関数の実装例（C言語）
- 整合性チェック結果
- データ損失分析

**対象読者**: システムアーキテクト、実装担当開発者

---

### 2. 仕様差分ドキュメント

#### [SOMA-TG_SETAXON_CHKAXON_ATCKSB_v1.0_vs_v1.2_diff.md](SOMA-TG_SETAXON_CHKAXON_ATCKSB_v1.0_vs_v1.2_diff.md)
**目的**: SOMA-TG仕様書の Ver1.0 vs Ver1.2 詳細差分

**内容**:
- SETAXON SET_FACE ビット定義の変更（3機能→5機能）
- ATCKSB CHK_FACE ビット定義の変更（2機能→8機能）
- CHKAXON（変更なし）
- 各ビットの詳細説明
- 互換性影響分析

**対象読者**: 仕様確認者、レビュアー

---

#### [SOMA-AXON_ATIRQ_SETAXON_v0.1_vs_v1.0_diff.md](SOMA-AXON_ATIRQ_SETAXON_v0.1_vs_v1.0_diff.md)
**目的**: SOMA-AXON仕様書の Ver0.1 vs Ver1.0 詳細差分

**内容**:
- ATIRQ STATUS ビット定義の変更（Bit 7追加のみ、[15:8]はRFU維持）
- SETAXON SET_PRTS ビット定義の変更
- Latch式センサー/ボタンの導入
- ダイヤル回転検知追加（Bit 7のみ）
- 互換性影響分析

**対象読者**: 仕様確認者、AXON連携担当者

---

## 🔗 ドキュメント間の関係

```
SOMA-TG仕様差分 ────┐
                    ├──→ プロトコル統合分析 ──→ 移行計画書
SOMA-AXON仕様差分 ──┘

1. まず仕様差分で「何が変わったか」を理解
2. 統合分析で「どう繋がるか」を理解
3. 移行計画で「どう実装するか」を実行
```

---

## 📋 推奨読む順序

### 新規参加者向け
1. **SOMA-TG_SETAXON_CHKAXON_ATCKSB_v1.0_vs_v1.2_diff.md** - TG側の変更を理解
2. **SOMA-AXON_ATIRQ_SETAXON_v0.1_vs_v1.0_diff.md** - AXON側の変更を理解
3. **PROTOCOL_INTEGRATION_ANALYSIS.md** - 全体の統合関係を理解
4. **MIGRATION_PLAN_V1.0_TO_V1.2.md** - 実装手順を確認

### 実装担当者向け
1. **MIGRATION_PLAN_V1.0_TO_V1.2.md** - 実装計画を確認
2. **PROTOCOL_INTEGRATION_ANALYSIS.md** - マッピング関数を参照
3. **SOMA-TG仕様差分** / **SOMA-AXON仕様差分** - 疑問点があれば参照

### レビュアー向け
1. **仕様差分ドキュメント** - 変更内容の妥当性確認
2. **PROTOCOL_INTEGRATION_ANALYSIS.md** - 統合設計の妥当性確認
3. **MIGRATION_PLAN_V1.0_TO_V1.2.md** - 実装計画の妥当性確認

---

## ⚠️ 重要な発見事項

### 1. バージョンアップの状況が異なる
- **SOMA-TG**: Ver1.0 → Ver1.2 (✅ **未対応** - 実装が必要)
- **SOMA-AXON**: Ver0.1 → Ver1.0 (✅ **対応済み** - 既に実装済み)

### 2. SOMA-AXON (S2A) は既にVer1.0対応済み
現在の実装ファイル `s2a_packet.h` は既にVer1.0のビット定義を含んでいます。
- `S2A_STATUS_*` マクロ: Ver1.0対応
- `S2A_SETAXON_SOL_*` マクロ: Ver1.0対応

### 3. TG-SOMA (T2S) のみ更新が必要
以下のファイルがVer1.0のまま:
- `t2s_packet.h` - ビット定義マクロ未定義
- `t2s_packet.c` - パース処理がVer1.0想定
- `uart_comm_tg.c` - ログ出力がVer1.0形式

### 4. Ver1.2(TG) と Ver1.0(AXON) でビット位置が完全一致
SOMA-AXON (S2A Ver1.0) ATIRQ STATUS Bit 7-0 と SOMA-TG (T2S Ver1.2) ATCKSB CHK_FACE Bit 7-0 が完全に一致するため、マッピングが極めてシンプルになりました。

---

## 🛠️ 実装ステータス

| フェーズ | 内容 | ステータス |
|---------|------|----------|
| フェーズ1 | ビット定義マクロ追加 | ⬜ 未着手 |
| フェーズ2 | 構造体コメント更新 | ⬜ 未着手 |
| フェーズ3 | SETAXON送信処理 | ⬜ 未着手 |
| フェーズ4 | ATCKSB受信処理 | ⬜ 未着手 |
| フェーズ5 | UART通信更新 | ⬜ 未着手 |
| フェーズ6 | マッピング関数 | ⬜ 未着手 |
| フェーズ7 | ハンドラー更新 | ⬜ 未着手 |
| テスト | 単体/統合/回帰 | ⬜ 未着手 |

---

## 📞 問い合わせ

本ドキュメントに関する質問や不明点がある場合は、プロジェクト担当者に連絡してください。

---

**作成日**: 2026年1月18日
**最終更新**: 2026年1月18日
**バージョン**: 1.0
