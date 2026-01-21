# 実装プロジェクト概要 - SOMA-TG Ver1.2 / SOMA-AXON Ver1.0 対応

**開始日**: 2026年1月19日
**ステータス**: 準備フェーズ
**目的**: プロトコルバージョンアップを段階的に実装

---

## 📊 プロジェクト規模

| 項目 | 内容 |
|------|------|
| **対象プロトコル** | SOMA-AXON Ver0.1→Ver1.0、SOMA-TG Ver1.0→Ver1.2 |
| **対象ファイル** | 10ファイル |
| **推定工数** | 12-16時間 |
| **段階** | 3フェーズ（+ テスト） |

---

## 🎯 実装フェーズ構成

### 🔴 Phase 1: プロトコル定義更新（2-3時間）
**目的**: 両プロトコルのビット定義・マクロを更新
- s2a_packet.h/c: STATUS Bit 7追加、SET_SOL→SET_PRTS、rotation_count削除
- t2s_packet.h: SET_FACE/CHK_FACE定義追加

**成果物**: プロトコル定義が新仕様に完全準拠

---

### 🟡 Phase 2: 通信処理・ハンドラ更新（4-5時間）
**目的**: SOMA-AXON/SOMA-TG間通信処理を新仕様に対応
- uart_comm_axon.c: rotation_count削除、STATUS Bit 7対応
- uart_comm_tg.c: rotation_count送信ロジック削除
- 各ハンドラ(4ファイル): FRAM構造・状態管理の更新

**成果物**: 新仕様での基本的な通信・データ管理が動作

---

### 🟢 Phase 3: Latchクリア・AXON Reset実装（3-4時間）
**目的**: Ver1.0新機能を完全実装
- Latchクリア処理: SET_PRTSビット操作
- AXON Reset: soma_send_axonrbt()実装
- 詳細制御ロジック

**成果物**: 新仕様による高度な制御機能が動作

---

## 📝 ドキュメント構成

本フォルダに以下3ファイルを段階的に更新：

1. **00_overview.md** ← このファイル
   - プロジェクト全体像
   - 各フェーズの概要

2. **01_phase_log.md**
   - Phase完了ごとに更新
   - 変更内容の要点
   - ビルド/テスト結果

3. **02_questions.md**
   - 推測が必要な箇所
   - 設計意思確認が必要な項目

---

## 🔐 実装制約

### 1. デグレ禁止
- 変更箇所以外は既存ロジック維持
- FRAM書込み/タイミングの不整合禁止
- 他の仕様準拠ロジックへの影響なし

### 2. 差分最小
- 既存API/構造体は可能な限り維持
- 目的外のリファクタ禁止

### 3. 段階的進行
- **各フェーズでビルドが通る状態を維持**
- フェーズ完了時に変更点を要約

### 4. 推測禁止
- 曖昧な仕様は推測で改変しない
- 02_questions.md に記録して確認

---

## 🔍 重要な確認項目

### Phase 1準備で確認済み
- ✅ s2a_packet.h: STATUS Bit 7未定義（追加予定）
- ✅ s2a_packet.h: SET_SOL定義あり（SET_PRTS に変更予定）
- ✅ s2a_packet.h: rotation_count マクロあり（削除予定）

### Phase 1開始前に確認が必要な項目
- ❓ rotation_detector.c の現在の実装内容
- ❓ atirq_status_get_rotation_count() の使用箇所
- ❓ FRAM の prize_counter の初期化・更新ロジック
- ⚠️ 詳細は 02_questions.md 参照

---

## 📅 次のステップ

1. **02_questions.md** に推測が必要な項目をまとめる
2. 質問に対する回答を確認
3. **Phase 1** を開始（プロトコル定義更新）
4. ビルド検証 → 02_phase_log.md に進捗記録
5. **Phase 2** 開始
6. ...

---

**作成**: AI Assistant
**最終更新**: 2026年1月19日
