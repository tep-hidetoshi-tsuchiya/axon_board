# FRAM 保存内容（ESP32-C6版）

## 概要
SOMA基板（ESP32-C6）でAXON設定情報を保存する仕様。各ポート（1-9）ごとに設定情報を管理し、電源OFF時も設定を保持する。

**注意**: 元仕様はSPI FRAM（TI MSPM0: PA8,PA12-PA14）だが、ESP32-C6版では以下の実装:
- **現在**: 静的配列（RAM、電源OFF時消失）
- **将来**: I2C FRAM（例: MB85RC256V、Fujitsu）

## 保存項目

### 基本情報
- **PORT 番号** (1-9)
  - AXONボードの接続ポート番号
  - MUX切替に使用

- **PORT-FLG** (有効/無効)
  - ポートの有効/無効状態
  - 0: 無効（未接続または0面）
  - 1: 有効（稼働中）

### 設定情報
- **FACE 番号** (0-255)
  - カプセル面番号
  - 0: 無効化状態
  - 1-255: 通常運用

- **タイムアウト値** (秒)
  - ソレノイドON時のタイムアウト
  - デフォルト: 5秒
  - 範囲: 1-255秒

- **設定金額** (100円単位)
  - カプセル1個の価格
  - 例: 0x0001 = 100円, 0x0064 = 10000円
  - 範囲: 0-65535 (0-6553500円)

### 運用情報
- **カウンタ値（売上回数）**
  - 累積販売個数
  - 32bit unsigned integer
  - オーバーフロー時は0に戻る

## データ構造

```c
typedef struct {
    uint8_t  port_num;      // PORT番号 (1-9)
    uint8_t  port_flg;      // 有効/無効フラグ
    uint8_t  face_num;      // FACE番号 (0-255)
    uint8_t  timeout;       // タイムアウト値 (秒)
    uint16_t cash_value;    // 設定金額 (100円単位)
    uint32_t counter;       // 売上回数カウンタ
    uint8_t  msn[6];        // シリアル番号 (参照用)
    uint8_t  fw_ver;        // FWバージョン (参照用)
    uint8_t  reserved[4];   // 予約領域
} FRAM_AXON_Entry;
```

## API仕様

### fram_update_entry()
```c
/**
 * @brief FRAM内の指定ポートエントリを更新
 * @param port ポート番号 (1-9)
 * @param entry 更新するエントリデータ
 * @return ESP_OK: 成功, ESP_FAIL: 失敗
 */
esp_err_t fram_update_entry(uint8_t port, const FRAM_AXON_Entry *entry);
```

### fram_load_entry()
```c
/**
 * @brief FRAM内の指定ポートエントリを読み出し
 * @param port ポート番号 (1-9)
 * @param entry 読み出したデータの格納先
 * @return ESP_OK: 成功, ESP_FAIL: 失敗
 */
esp_err_t fram_load_entry(uint8_t port, FRAM_AXON_Entry *entry);
```

### fram_increment_counter()
```c
/**
 * @brief 指定ポートの売上カウンタを+1
 * @param port ポート番号 (1-9)
 * @return ESP_OK: 成功, ESP_FAIL: 失敗
 */
esp_err_t fram_increment_counter(uint8_t port);
```

### fram_init_all_entries()
```c
/**
 * @brief 全ポートのFRAMエントリを初期化
 * @return ESP_OK: 成功, ESP_FAIL: 失敗
 */
esp_err_t fram_init_all_entries(void);
```

### fram_check_face_duplicate()
```c
/**
 * @brief FACE番号の重複チェック
 * @param face_num チェックするFACE番号
 * @param exclude_port 除外するポート番号 (自分自身)
 * @return true: 重複あり, false: 重複なし
 */
bool fram_check_face_duplicate(uint8_t face_num, uint8_t exclude_port);
```

## メモリマップ

| アドレス範囲 | サイズ | 用途 |
|------------|------|------|
| 0x0000-0x001F | 32B | Port 1 設定 |
| 0x0020-0x003F | 32B | Port 2 設定 |
| 0x0040-0x005F | 32B | Port 3 設定 |
| 0x0060-0x007F | 32B | Port 4 設定 |
| 0x0080-0x009F | 32B | Port 5 設定 |
| 0x00A0-0x00BF | 32B | Port 6 設定 |
| 0x00C0-0x00DF | 32B | Port 7 設定 |
| 0x00E0-0x00FF | 32B | Port 8 設定 |
| 0x0100-0x011F | 32B | Port 9 設定 |
| 0x0120-0x01FF | 224B | 予約領域 |

## ESP32-C6実装の差異

### 元仕様（TI MSPM0）
- **接続**: SPI（PA8=CS, PA12=SCK, PA13=SO, PA14=SI）
- **容量**: 32KB（MB85RS256B等）
- **アクセス**: SPIマスターモード

### ESP32-C6実装
- **現在**: 静的配列（RAM）
  - 電源OFF時消失
  - テスト・開発用
- **将来**: I2C FRAM
  - 容量: 32KB（MB85RC256V等）
  - I2Cアドレス: 0x50-0x57（A0-A2設定）
  - 速度: 400kHz

## 関連ファイル（ESP32-C6版）
- `src/peripheral/fram_utils.h` - FRAM API定義
- `src/peripheral/fram_utils.c` - FRAM API実装（静的配列）
- `src/peripheral/soma_uart.c` - FRAM読み書き使用例
- `src/protocol/src/types.h` - axon_config_t構造体定義

## 実装状況（ESP32-C6版）
- [x] **基本データ構造定義** - axon_config_t
- [x] **静的配列による実装** - soma_fram_data[9]
- [x] **基本読み書きAPI** - soma_fram_read/write_axon_config()
- [x] **ポートスキャン連携** - scan_axon_ports()でFRAM更新
- [ ] **fram_update_entry()** - 標準API（完全版）
- [ ] **fram_load_entry()** - 標準API（完全版）
- [ ] **fram_increment_counter()** - カウンタ+1
- [ ] **fram_check_face_duplicate()** - 重複チェック
- [ ] **I2C FRAM実装** - ハードウェア永続化
- [ ] **エラーハンドリング** - リトライ・ログ

## I2C FRAM移行計画

### 推奨デバイス
- **MB85RC256V** (Fujitsu)
  - 容量: 32KB (256Kbit)
  - I2C: Fast Mode (400kHz)
  - 電圧: 2.7-3.6V
  - 書き込み回数: 無制限
  - データ保持: 10年

### 配線例
```
ESP32-C6         MB85RC256V
GPIO19 (SDA) <-> SDA
GPIO18 (SCL) <-> SCL
3.3V         <-> VDD
GND          <-> VSS, A0, A1, A2 (I2Cアドレス: 0x50)
```

### 移行手順
1. I2Cドライバ初期化（I2C_NUM_0）
2. FRAMデバイス検出（アドレススキャン）
3. 読み書きAPI実装（I2C版）
4. 静的配列からの移行コード
5. 電源OFF試験

## 備考（ESP32-C6版）
- **現在**: 静的配列（開発・テスト用）
- **将来**: I2C FRAM（本番用）
- **電源OFF保持**: I2C FRAM実装後に対応
- **書き込み回数**: FRAM特性により無制限
- **データ整合性**: CRC追加を検討中
- **バックアップ**: NVS（Non-Volatile Storage）併用も検討
