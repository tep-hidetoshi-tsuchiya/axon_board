# AXON基板 UART0 初期設定・動作ガイド

## 📋 概要

MSPM0G3507（AXON基板）のUART0を使用したSOMA-AXON間通信の完全な設定手順と実装詳細。

**対象ハードウェア:**
- MCU: MSPM0G3507 (Cortex-M0+)
- UART: UART0（物理アドレス 0x40108000）
- TX: PA10 (PINCM21 / GPIO_PIN_10)
- RX: PA11 (PINCM22 / GPIO_PIN_11)

**通信設定:**
- ボーレート: 115200 bps
- クロック: 32MHz SYSOSC
- データ: 8bit, パリティなし, ストップビット1
- フロー制御: なし

---

## ⚠️ 重要な注意事項

### デバイスヘッダの不具合

**問題:** TI SDK 2.06.00.05のデバイスヘッダに不具合があり、UART0/UART2マクロが間違ったアドレスを返す。

```c
// ❌ 間違ったアドレス（デバイスヘッダの不具合）
UART0 マクロ → 0x40108000 ではなく 0x40002800 を返す場合がある
UART2 マクロ → 0x40102000 ではなく別のアドレスを返す場合がある
```

**解決策:** 直接アドレスキャストを使用

```c
// ✅ 正しい実装（driver_config.h）
#define S2A_UART_INST   ((UART_Regs *)0x40108000UL)  // UART0の正しいアドレス
```

**MSPM0G3507 UARTアドレスマップ（データシートTable 3-1準拠）:**
| UART | Base Address | 用途（AXON基板） |
|------|--------------|------------------|
| UART0 | 0x40108000 | SOMA-AXON通信 (PA10/PA11) |
| UART1 | 0x40104000 | （未使用） |
| UART2 | 0x40102000 | （未使用） |
| UART3 | 0x40100000 | （未使用） |

---

## 🔧 1. UART0初期化の完全な実装

### 1.1 初期化関数（msp_peripheral_config.c）

**場所:** `peripheral/msp_peripheral_config.c` の `_msp_peripheral_uart_init()`

**初期化順序（重要）:**
1. GPIO ピン設定（TX/RX）
2. UART パワードメイン有効化
3. クロック設定
4. UART モード設定
5. ボーレート設定
6. FIFO 有効化
7. 割り込み有効化（UART + NVIC）
8. **UART 有効化（最後に実行）**

```c
static void _msp_peripheral_uart_init(void) {
#ifdef AXON_BOARD
    // ★デバッグ: 関数呼び出し確認
    volatile uint32_t debug_marker = 0xDEADBEEF;
    
    // ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
    UART_Regs *uart0 = (UART_Regs *)0x40108000UL;
    
    // ステップ1: GPIO ピン設定（最初に実行）
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);
    
    // ステップ2: パワードメイン有効化
    DL_UART_reset(uart0);
    DL_UART_enablePower(uart0);
    
    // ステップ3: クロック設定（32MHz SYSOSC, 分周なし）
    DL_UART_ClockConfig clk = {
        .clockSel    = DL_UART_CLOCK_BUSCLK,
        .divideRatio = DL_UART_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_setClockConfig(uart0, &clk);
    
    // ステップ4: UARTモード設定
    DL_UART_Config cfg = {
        .mode        = DL_UART_MODE_NORMAL,
        .direction   = DL_UART_DIRECTION_TX_RX,
        .flowControl = DL_UART_FLOW_CONTROL_NONE,
        .parity      = DL_UART_PARITY_NONE,
        .wordLength  = DL_UART_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_STOP_BITS_ONE
    };
    DL_UART_init(uart0, &cfg);
    
    // ステップ5: ボーレート設定（115200bps @ 32MHz）
    // 計算式: IBRD = 32000000 / (16 * 115200) = 17.36...
    // IBRD = 17, FBRD = int(0.36 * 64 + 0.5) = 23
    DL_UART_configBaudRate(uart0, 32000000, 115200);
    
    // ステップ6: FIFO 有効化
    DL_UART_enableFIFOs(uart0);
    DL_UART_setRXFIFOThreshold(uart0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);  // 1バイト受信で割り込み
    DL_UART_setTXFIFOThreshold(uart0, DL_UART_TX_FIFO_LEVEL_EMPTY);
    
    // ステップ7: 割り込み有効化
    DL_UART_enableInterrupt(uart0, DL_UART_INTERRUPT_RX);  // RX割り込みのみ
    NVIC_EnableIRQ(UART0_INT_IRQn);  // IRQn = 15
    
    // ステップ8: UART有効化（最後に実行、これが無いと動作しない）
    DL_UART_enable(uart0);
    
    // ★デバッグ: 初期化完了マーカー
    debug_marker = 0xCAFEBABE;
#endif
}
```

### 1.2 初期化順序の重要性

**SYSCFG_DL_init() の呼び出し順序:**

```c
void SYSCFG_DL_init(void) {
    SYSCFG_DL_SYSCTL_init();     // 1. システムクロック初期化
    SYSCFG_DL_initPower();       // 2. パワードメイン初期化
    SYSCFG_DL_GPIO_init();       // 3. GPIO初期化（UART以外）
    SYSCFG_DL_PWM_init();        // 4. PWM初期化
    SYSCFG_DL_SPI_init();        // 5. SPI初期化
    
    // ★重要: UART初期化は最後に実行
    // GPIO初期化でピン設定が上書きされるのを防ぐため
    _msp_peripheral_uart_init(); // 6. UART初期化（GPIO設定含む）
    
    SYSCFG_DL_AES_init();        // 7. その他ペリフェラル
    SYSCFG_DL_CRC_init();
    SYSCFG_DL_TRNG_init();
    SYSCFG_DL_SYSTICK_init();
}
```

**理由:**
- UART初期化を先に実行すると、後続のGPIO初期化でピン設定が上書きされる可能性がある
- UART初期化内でGPIOピン設定も行うため、GPIO初期化後に実行する必要がある

### 1.3 パワードメイン初期化（SYSCFG_DL_initPower）

**場所:** `peripheral/msp_peripheral_config.c` の `SYSCFG_DL_initPower()`

```c
void SYSCFG_DL_initPower(void) {
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(LED_RG_TIM_INST);
    DL_TimerG_reset(LED_B_TIM_INST);

#ifdef AXON_BOARD
    // ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
    DL_UART_reset((UART_Regs *)0x40108000UL);
#endif

    DL_AES_reset(AES);
    DL_CRC_reset(CRC);
    DL_TRNG_reset(TRNG);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(LED_RG_TIM_INST);
    DL_TimerG_enablePower(LED_B_TIM_INST);

#ifdef AXON_BOARD
    // ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
    DL_UART_enablePower((UART_Regs *)0x40108000UL);
#endif

    DL_AES_enablePower(AES);
    DL_CRC_enablePower(CRC);
    DL_TRNG_enablePower(TRNG);

    delay_cycles(POWER_STARTUP_DELAY);
}
```

### 1.4 GPIO初期化の注意点

**UART0ピンの重複設定を回避:**

`SYSCFG_DL_GPIO_init()` 内では **UART0ピン（PA10/PA11）を設定しない**。
UART初期化関数内で設定するため、重複を避ける。

```c
void SYSCFG_DL_GPIO_init(void) {
#ifdef AXON_BOARD
    // ... 他のGPIO設定 ...
    
    // ★重要: UART0ピン（PA10/PA11）はここで設定しない
    // _msp_peripheral_uart_init() 内で設定済み
    // 以前はここで設定していたが、初期化順序の問題で削除
    
    // ... 他のGPIO設定 ...
#endif
}
```

---

## 🎯 2. UART0 割り込み処理（ISR）

### 2.1 割り込みハンドラ（isr.c）

**場所:** `isr.c` の `UART0_IRQHandler()`

**36バイトフレーム受信処理:**
- ヘッダー: 0x14 0x20（2バイト）
- データ部: 32バイト
- CRC16: 2バイト（リトルエンディアン）

```c
void UART0_IRQHandler(void) {
#ifdef AXON_BOARD
    // RX割り込み処理
    while (DL_UART_isRXFIFOEmpty(S2A_UART_INST) == false) {
        uint8_t b = DL_UART_receiveData(S2A_UART_INST);
        
        // デバッグカウンタ
        debug_rx_count++;
        debug_last_byte = b;
        
        // フレーム同期: 2バイトヘッダー(0x14 0x20)を確実に検出
        if (rx_index == 0) {
            // 1バイト目: 0x14でなければ破棄
            if (b == 0x14) {
                rx_frame[rx_index++] = b;
                debug_rx_index = rx_index;
            }
        } else if (rx_index == 1) {
            // 2バイト目: 0x20確認
            debug_byte1 = b;
            
            if (b == 0x20) {
                // 正しいヘッダー2バイト目
                rx_frame[rx_index++] = b;
                debug_rx_index = rx_index;
            } else {
                // ヘッダー不一致 → リセット
                debug_byte1_ng_count++;
                debug_sync_reset_count++;
                rx_index = 0;
                debug_rx_index = rx_index;
                
                // ★重要: 受信したバイトが0x14なら次のフレームの先頭として保存
                if (b == 0x14) {
                    rx_frame[rx_index++] = b;
                    debug_rx_index = rx_index;
                }
            }
        } else {
            // 3バイト目以降: 通常受信
            rx_frame[rx_index++] = b;
            debug_rx_index = rx_index;
            
            // 36バイト受信完了
            if (rx_index >= AXON_FRAME_SIZE) {
                debug_complete_count++;
                
                // ★重要: 次の受信で上書きされる前に完成フレームを退避
                memcpy(rx_complete_frame, rx_frame, AXON_FRAME_SIZE);
                rx_complete_ready = 1;
                
                frame_received = 1;
                rx_index = 0;
                debug_rx_index = rx_index;
            }
        }
    }
#else
    // SOMA_BOARD用の既存UART処理
    UARTMSP_interruptHandler((UART_Handle)&UART_config[0]);
#endif
}
```

### 2.2 ダブルバッファリング実装

**問題:** ISRで受信中にメイン側が処理すると、データが破損する

**解決策:** ISR→メイン間でダブルバッファリング

```c
// isr.c（グローバルスコープ）
uint8_t rx_complete_frame[AXON_FRAME_SIZE];       // ISRが退避するバッファ
volatile uint8_t rx_complete_ready = 0;           // データ準備完了フラグ

// soma_uart_test.h（extern宣言）
extern uint8_t rx_complete_frame[AXON_FRAME_SIZE];
extern volatile uint8_t rx_complete_ready;
```

**メイン側での使用方法（soma_uart_test.c）:**

```c
void soma_check_frame(void) {
    if (!frame_received)
        return;

    frame_received = 0;
    
    // ★重要: ISRが退避した完成フレームをコピー（競合回避）
    if (!rx_complete_ready) {
        return;  // データ準備できていない
    }
    
    memcpy(rx_frame, rx_complete_frame, AXON_FRAME_SIZE);
    rx_complete_ready = 0;  // クリア
    
    // フレーム処理開始...
}
```

### 2.3 デバッグ用変数

**宣言場所:** `soma_uart_test.c`（定義） / `soma_uart_test.h`（extern宣言）

```c
// デバッグカウンタ（デバッガのExpressionsウィンドウで確認）
volatile uint32_t debug_rx_count = 0;          // 受信バイト総数
volatile uint32_t debug_frame_count = 0;       // メイン処理完了フレーム数
volatile uint32_t debug_sync_reset_count = 0;  // ヘッダー同期リセット回数
volatile uint32_t debug_complete_count = 0;    // ISRで36バイト完了回数
volatile uint8_t debug_last_byte = 0;          // 最後に受信したバイト
volatile uint8_t debug_rx_index = 0;           // 現在のrx_index
volatile uint8_t debug_byte1 = 0;              // 2バイト目の値
volatile uint32_t debug_byte1_ng_count = 0;    // 2バイト目!=0x20の回数
```

**確認方法:**
1. CCS Debuggerの `Expressions` ウィンドウに追加
2. プログラム実行中にリアルタイム確認
3. 期待値との比較

**期待される値（正常動作時）:**
```
debug_rx_count = 36 * N          （Nフレーム受信）
debug_complete_count = N         （N回完了）
debug_sync_reset_count = 0       （同期エラーなし）
debug_byte1_ng_count = 0         （Lengthエラーなし）
frame_received = 1 → 0 → 1...    （受信→処理→受信）
rx_complete_ready = 1 → 0 → 1... （準備→消費→準備）
```

---

## 📦 3. フレームプロトコル仕様

### 3.1 36バイトフレーム構造

```
Byte[0]    : 0x14           （ヘッダー固定値）
Byte[1]    : 0x20           （Length = 32バイト）
Byte[2-33] : データ部 32バイト（AES-128-CBC暗号化済み）
Byte[34-35]: CRC16 リトルエンディアン
```

### 3.2 CRC16計算仕様

**アルゴリズム:** CRC16-CCITT (ISO/IEC 13239) LSB-first

**パラメータ:**
- 多項式（通常形式）: 0x1021
- 多項式（反転形式）: 0x8408
- 初期値: 0xFFFF
- 最終XOR: なし
- 入出力: LSB-first (reflected)

**計算対象:** Byte[0-33]（ヘッダー + データ部、CRC自体は含まない）

**実装:**

```c
uint16_t crc16_tep(const uint8_t* data, int len) {
    uint16_t crc = 0xFFFF;  // 初期値
    
    for (int i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];  // データバイトをXOR
        
        // 8ビット分処理（LSB-first）
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x8408;  // LSB=1: 多項式でXOR
            } else {
                crc >>= 1;                   // LSB=0: 単純に右シフト
            }
        }
    }
    
    return crc;  // 最終XORなし
}
```

**検証方法:**

```c
uint8_t frame[36];
// ... フレーム作成 ...

// CRC計算（Byte[0-33]）
uint16_t crc_calc = crc16_tep(frame, 34);
frame[34] = crc_calc & 0xFF;         // CRC LSB
frame[35] = (crc_calc >> 8) & 0xFF;  // CRC MSB

// 受信側での検証
uint16_t crc_recv = frame[34] | (frame[35] << 8);
uint16_t crc_check = crc16_tep(frame, 34);
if (crc_recv == crc_check) {
    // CRC正常
}
```

### 3.3 ACK/NACKフレーム（6バイト）

**送信フレーム構造:**
```
Byte[0]  : 0x14  （ヘッダー）
Byte[1]  : 0x04  （Length = 4バイト）
Byte[2]  : 0x01  （コマンド種別: 応答）
Byte[3]  : 0x01 (ACK) or 0x02 (NACK)
Byte[4-5]: CRC16（Byte[2-3]に対して計算）
```

**実装例:**

```c
bool send_ack_frame(void) {
    uint8_t tx_frame[6] = {
        0x14,  // Header
        0x04,  // Length
        0x01,  // Response type
        0x01,  // ACK
        0x00, 0x00  // CRC（後で計算）
    };
    
    // CRC計算（Byte[2-3]）
    uint16_t crc = crc16_tep(&tx_frame[2], 2);
    tx_frame[4] = crc & 0xFF;
    tx_frame[5] = (crc >> 8) & 0xFF;
    
    // UART送信
    for (int i = 0; i < 6; i++) {
        while (!DL_UART_isTXFIFOEmpty(S2A_UART_INST));
        DL_UART_transmitData(S2A_UART_INST, tx_frame[i]);
        while (DL_UART_isBusy(S2A_UART_INST));
    }
    
    return true;
}
```

---

## 🧪 4. テスト方法

### 4.1 基本ループバックテスト

**ハードウェア接続:**
- PA10(TX) と PA11(RX) をジャンパー線で短絡

**テスト内容:**
```c
bool axon_uart_loopback_test(void) {
    // 1. 36バイトテストパターン作成
    // 2. UART0経由で送信
    // 3. ISRで受信
    // 4. 送受信データ比較
}
```

**期待結果:**
```
✓ LOOPBACK TEST PASSED
debug_rx_count = 36
debug_complete_count = 1
All 36 bytes matched
```

### 4.2 36バイトフレーム完全検証テスト

**テスト内容:**
```c
bool axon_36byte_frame_test(void) {
    // 1. ヘッダー(0x14 0x20) + データ32バイト + CRC16
    // 2. CRC16計算して付加
    // 3. UART0ループバック送信
    // 4. ISR受信 → ダブルバッファ → メイン処理
    // 5. ヘッダー検証
    // 6. データ部検証
    // 7. CRC16検証
}
```

**期待結果:**
```
✓ 36-BYTE FRAME TEST PASSED
✓ Header: 0x14 0x20
✓ Data: 32 bytes correct
✓ CRC16: valid
✓ Frame sync: working
✓ ISR buffering: working
```

### 4.3 実機SOMA通信テスト

**準備:**
1. PA10/PA11のジャンパー線を外す
2. SOMA基板と接続（UART0 ↔ SOMA UART）
3. 両方の基板に電源投入

**テスト手順:**
1. SOMA側から36バイトフレーム送信
2. AXON側で受信・CRC検証
3. ACK応答送信
4. デバッグカウンタ確認

**確認項目:**
- `debug_rx_count` = 36
- `debug_sync_reset_count` = 0（同期エラーなし）
- `frame_received` = 1
- CRC検証 PASSED

---

## 📊 5. レジスタ確認チェックリスト

### 5.1 初期化後のUART0レジスタ

**期待値:**
```
アドレス: 0x40108000

CTL0  = 0x00020039 または 0x00000301
        bit[0]  EN  = 1  （UART有効）
        bit[8]  TXE = 1  （TX有効）
        bit[9]  RXE = 1  （RX有効）

STAT  = 0x00000044 または 0x00000020
        bit[4]  TXFE = 1  （TX FIFO Empty）
        bit[6]  BUSY = 0  （アイドル）

IBRD  = 0x00000011  （17 decimal）
FBRD  = 0x00000017  （23 decimal）

IFLS  = 0x00000075 または類似
        RX FIFO threshold = 1 entry

CPU_INT.IMASK = 0x00000400 または類似
        RX interrupt enabled
```

### 5.2 GPIOピン設定

**IOMUX設定:**
```
PINCM21 (PA10/TX) = 0x00000082
        bit[3:0] PF    = 0x2  （UART0機能）
        bit[7]   PIPU  = 1    （プルアップ有効）
        bit[8]   INENA = 0    （入力無効、出力のみ）

PINCM22 (PA11/RX) = 0x00040082
        bit[3:0] PF    = 0x2  （UART0機能）
        bit[7]   PIPU  = 1    （プルアップ有効）
        bit[8]   INENA = 1    （入力有効）
        bit[18]  IOMUX_INENA_Override = 1
```

### 5.3 NVIC設定

**期待値:**
```
UART0_IRQn = 15

NVIC_GetEnableIRQ(15) = 1     （割り込み有効）
NVIC_GetPriority(15)  = 0     （最高優先度）
NVIC_GetPendingIRQ(15) = 0    （ペンディングなし）
```

---

## 🔍 6. トラブルシューティング

### 問題1: 受信割り込みが発生しない

**症状:**
- `debug_rx_count` が増えない
- `frame_received` が常に0

**原因と対策:**

1. **UART0アドレス間違い**
   ```c
   // ❌ 間違い
   UART_Regs *uart0 = UART0;  // デバイスヘッダの不具合
   
   // ✅ 正しい
   UART_Regs *uart0 = (UART_Regs *)0x40108000UL;
   ```

2. **UART有効化忘れ**
   ```c
   // ★最後に必ず実行
   DL_UART_enable(uart0);
   ```

3. **NVIC未有効**
   ```c
   // 割り込み有効化（両方必要）
   DL_UART_enableInterrupt(uart0, DL_UART_INTERRUPT_RX);
   NVIC_EnableIRQ(UART0_INT_IRQn);
   ```

4. **GPIOピン設定不足**
   ```c
   // TX/RX両方必要
   DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
   DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);
   ```

### 問題2: データが破損する

**症状:**
- `debug_rx_count` = 36 だが、データ不一致
- CRC検証失敗

**原因と対策:**

1. **ISR・メイン間の競合**
   ```c
   // ❌ 間違い: rx_frame[]を直接参照
   memcpy(data, rx_frame, 36);
   
   // ✅ 正しい: ISRが退避したバッファを使用
   if (rx_complete_ready) {
       memcpy(data, rx_complete_frame, 36);
       rx_complete_ready = 0;
   }
   ```

2. **ボーレート設定ミス**
   ```c
   // 32MHz SYSOSC時のIBRD/FBRD
   DL_UART_configBaudRate(uart0, 32000000, 115200);
   // → IBRD=17, FBRD=23
   ```

### 問題3: フレーム同期がずれる

**症状:**
- `debug_sync_reset_count` が増加
- ヘッダー検出失敗

**原因と対策:**

1. **ヘッダー値の確認**
   ```c
   // 正しいヘッダー
   frame[0] = 0x14;
   frame[1] = 0x20;
   ```

2. **バイト順序の確認**
   ```c
   // リトルエンディアン（CRC）
   frame[34] = crc & 0xFF;         // LSB
   frame[35] = (crc >> 8) & 0xFF;  // MSB
   ```

### 問題4: CTL0 = 0x00000000（UART無効）

**症状:**
- レジスタが全て0
- 初期化が効いていない

**原因と対策:**

1. **ビルドキャッシュ問題**
   ```powershell
   # CCS: Project → Clean Project
   # または
   Remove-Item -Recurse -Force Debug\*.o, Debug\*.d
   ```

2. **初期化関数が呼ばれていない**
   ```c
   // デバッグマーカーで確認
   volatile uint32_t debug_marker = 0xDEADBEEF;
   // ... 初期化処理 ...
   debug_marker = 0xCAFEBABE;
   
   // デバッガで debug_marker の値を確認
   // 0xDEADBEEF のままなら関数未実行
   ```

3. **#ifdef条件不一致**
   ```c
   // コンパイラフラグで -DAXON_BOARD が定義されているか確認
   #ifdef AXON_BOARD
   _msp_peripheral_uart_init();
   #endif
   ```

---

## 📚 7. 参考資料

### 7.1 関連ファイル一覧

| ファイル | 役割 | 重要な関数/定義 |
|---------|------|----------------|
| `driver_config.h` | UART0マクロ定義 | `S2A_UART_INST = 0x40108000` |
| `peripheral/msp_peripheral_config.c` | UART0初期化 | `_msp_peripheral_uart_init()` |
| `isr.c` | 割り込みハンドラ | `UART0_IRQHandler()` |
| `soma_uart_test.c` | フレーム処理・テスト | `soma_check_frame()`, `axon_36byte_frame_test()` |
| `soma_uart_test.h` | 定数・extern宣言 | `AXON_FRAME_SIZE`, デバッグ変数 |

### 7.2 TI DriverLib API

**主要API:**
```c
// 初期化
DL_UART_reset(uart0);
DL_UART_enablePower(uart0);
DL_UART_setClockConfig(uart0, &clk);
DL_UART_init(uart0, &cfg);
DL_UART_configBaudRate(uart0, clk_freq, baud);
DL_UART_enableFIFOs(uart0);
DL_UART_setRXFIFOThreshold(uart0, threshold);
DL_UART_enableInterrupt(uart0, mask);
DL_UART_enable(uart0);

// GPIO
DL_GPIO_initPeripheralOutputFunction(pincm, pf);
DL_GPIO_initPeripheralInputFunction(pincm, pf);

// 送受信
DL_UART_transmitData(uart0, data);
uint8_t data = DL_UART_receiveData(uart0);

// ステータス
bool empty = DL_UART_isRXFIFOEmpty(uart0);
bool empty = DL_UART_isTXFIFOEmpty(uart0);
bool busy = DL_UART_isBusy(uart0);
```

### 7.3 データシート参照

**MSPM0G3507 Technical Reference Manual:**
- Section: UART Module
- Table 3-1: Peripheral Base Addresses
- Section: GPIO / IOMUX Configuration

---

## ✅ 8. チェックリスト

### 初期設定完了確認

- [ ] `driver_config.h` で `S2A_UART_INST = 0x40108000` 定義
- [ ] `msp_peripheral_config.c` に `_msp_peripheral_uart_init()` 実装
- [ ] `SYSCFG_DL_init()` で UART初期化を最後に実行
- [ ] GPIO ピン設定（TX/RX）を UART初期化内で実行
- [ ] UART有効化（`DL_UART_enable`）を最後に実行
- [ ] NVIC有効化（`NVIC_EnableIRQ`）

### ISR実装確認

- [ ] `isr.c` に `UART0_IRQHandler()` 実装
- [ ] ヘッダー同期処理（0x14 0x20）実装
- [ ] ダブルバッファリング実装（`rx_complete_frame`）
- [ ] デバッグカウンタ実装
- [ ] `#include <string.h>` 追加（memcpy用）

### フレーム処理確認

- [ ] CRC16計算関数実装（`crc16_tep`）
- [ ] フレーム検証処理（ヘッダー・CRC）
- [ ] ACK/NACK送信関数実装
- [ ] テスト関数実装（ループバック・フレーム）

### ビルド設定確認

- [ ] コンパイラフラグ `-DAXON_BOARD` 設定
- [ ] Clean Build 実行
- [ ] リンクエラーなし
- [ ] 警告確認（重要な警告のみ対応）

### 動作確認

- [ ] レジスタ値確認（CTL0, IBRD, FBRD, STAT）
- [ ] GPIO ピン設定確認（PINCM21, PINCM22）
- [ ] NVIC 設定確認
- [ ] ループバックテスト成功
- [ ] 36バイトフレームテスト成功
- [ ] デバッグカウンタ正常値

---

## 📝 変更履歴

| 日付 | 変更内容 | 理由 |
|------|---------|------|
| 2025-11-26 | UART0アドレス 0x40002800 → 0x40108000 | デバイスヘッダの不具合対応 |
| 2025-11-26 | ダブルバッファリング実装 | ISR・メイン間の競合回避 |
| 2025-11-26 | UART初期化順序変更（最後に実行） | GPIO上書き問題の回避 |
| 2025-11-26 | ループバック待機条件修正 | `rx_index` → `frame_received` |
| 2025-11-26 | `static` 削除・`extern` 宣言追加 | リンクエラー解消 |

---

## 🎯 まとめ

### 最重要ポイント

1. **UART0アドレスは 0x40108000 を直接指定**
   - デバイスヘッダマクロは使用しない

2. **初期化順序厳守**
   - UART初期化は GPIO初期化の後、最後に実行

3. **ダブルバッファリング必須**
   - ISRとメイン間でデータ競合を回避

4. **DL_UART_enable() は最後**
   - 全設定完了後にUARTを有効化

5. **フレーム同期の実装**
   - 0x14 0x20 ヘッダー検出ロジック

これらの設定を正しく実装すれば、UART0は安定して動作します。
実機SOMA通信でも同じ設定で対応可能です。
