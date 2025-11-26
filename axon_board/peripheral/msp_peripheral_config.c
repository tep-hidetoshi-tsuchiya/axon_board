#define DEFINE_GLOBALS
#include "msp_peripheral_config.h"

#include "ti_msp_dl_config.h"
#define POWER_STARTUP_DELAY (16)

#ifdef SOMA_BOARD
/*
 * SPI clock
 * Source Clock(32MHz) / 1 = 32MHz
 */
static const DL_SPI_ClockConfig g_FRAM_CLOCK_CONFIG = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,             // 32MHz
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1,     // 分周なし
};

static const DL_SPI_Config g_FRAM_SPI_CONFIG = {
    .mode          = DL_SPI_MODE_CONTROLLER,
    .frameFormat   = DL_SPI_FRAME_FORMAT_MOTO4_POL0_PHA0,
    .parity        = DL_SPI_PARITY_NONE,
    .dataSize      = DL_SPI_DATA_SIZE_8,
    .bitOrder      = DL_SPI_BIT_ORDER_MSB_FIRST,
    .chipSelectPin = DL_SPI_CHIP_SELECT_NONE,  // CS手動制御設定
};
#endif

/*
 * UART clock
 * Source Clock(40MHz) / 1 = 40MHz
 * target baud rate = 115200bps
 * 115200bps = 32MHz / (16 * 115200) = 17.36111111
 * IBRD = 17
 * FBRD = 0.36111111 * 64 = 23.111 => 23
 */
static const DL_UART_Main_ClockConfig g_UART_CLOCK_CONFIG = {.clockSel    = DL_UART_CLOCK_BUSCLK,
                                                             .divideRatio = DL_UART_CLOCK_DIVIDE_RATIO_1};

static const DL_UART_Config g_UART_CONFIG = {
    .mode        = DL_UART_MODE_NORMAL,
    .direction   = DL_UART_DIRECTION_TX_RX,
    .flowControl = DL_UART_FLOW_CONTROL_NONE,
    .parity      = DL_UART_PARITY_NONE,
    .wordLength  = DL_UART_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_STOP_BITS_ONE,
};

/*
 * TimerA clock
 * BUSCLK(32MHz) / 32 = 1MHz (MSPM0G_CPU_FREQ_80MHzが未定義の場合)
 * BUSCLK(40MHz) / 20 = 2MHz (MSPM0G_CPU_FREQ_80MHzが定義されている場合)
 */
static const DL_TimerA_ClockConfig g_TIMA_LED_CLOCK_CONFIG = {
    .clockSel    = TIM_LED_CLOCK_SOURCE_BUSCLK,
    .divideRatio = TIM_LED_CLOCK_SOURCE_DIVIDE,
    .prescale    = TIM_LED_CLOCK_PRESCALE,
};

/*
 * TimerA PWM
 */
static const DL_TimerA_PWMConfig g_TIMA_LED_PWM_CONFIG = {
    .isTimerWithFourCC = false,
    .period            = TIM_LED_PWM_PERIOD_COUNT,
    .pwmMode           = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .startTimer        = DL_TIMER_START,
};

/*
 * TimerG clock
 * BUSCLK(32MHz) / 32 = 1MHz (MSPM0G_CPU_FREQ_80MHzが未定義の場合)
 * BUSCLK(40MHz) / 20 = 2MHz (MSPM0G_CPU_FREQ_80MHzが定義されている場合)
 */
static const DL_TimerG_ClockConfig g_TIMG_LED_CLOCK_CONFIG = {
    .clockSel    = TIM_LED_CLOCK_SOURCE_BUSCLK,
    .divideRatio = TIM_LED_CLOCK_SOURCE_DIVIDE,
    .prescale    = TIM_LED_CLOCK_PRESCALE,
};

/*
 * TimerG PWM
 */
static const DL_TimerG_PWMConfig g_TIMG_LED_PWM_CONFIG = {
    .isTimerWithFourCC = false,
    .period            = TIM_LED_PWM_PERIOD_COUNT,
    .pwmMode           = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .startTimer        = DL_TIMER_START,
};

static const DL_TimerG_ClockConfig g_TIMG_DEBOUNCE_CLOCK_CONFIG = {
    .clockSel    = TIM_DEBOUNCE_CLOCK_SOURCE_BUSCLK,
    .divideRatio = TIM_DEBOUNCE_CLOCK_SOURCE_DIVIDE,
    .prescale    = TIM_DEBOUNCE_CLOCK_PRESCALE,
};

static const DL_TimerG_TimerConfig g_TIMG_DEBOUNCE_TIMER_CONFIG = {
    .timerMode  = TIM_DEBOUNCE_TIMER_MODE,
    .period     = TIM_DEBOUNCE_TIMER_PERIOD_COUNT,
    .startTimer = TIM_DEBOUNCE_TIMER_START,
};

#ifdef SOMA_BOARD
static const DL_DMA_Config g_DMA_CH0_CONFIG = {
    .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
    .extendedMode  = DL_DMA_NORMAL_MODE,
    .srcIncrement  = DL_DMA_ADDR_UNCHANGED,
    .destIncrement = DL_DMA_ADDR_INCREMENT,
    .destWidth     = DL_DMA_WIDTH_BYTE,
    .srcWidth      = DL_DMA_WIDTH_BYTE,
    .trigger       = DL_UART_DMA_IIDX_RX_TRIGGER,
    .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

static const DL_DMA_Config g_DMA_CH1_CONFIG = {
    .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
    .extendedMode  = DL_DMA_NORMAL_MODE,
    .srcIncrement  = DL_DMA_ADDR_UNCHANGED,
    .destIncrement = DL_DMA_ADDR_INCREMENT,
    .destWidth     = DL_DMA_WIDTH_BYTE,
    .srcWidth      = DL_DMA_WIDTH_BYTE,
    .trigger       = DL_UART_DMA_IIDX_RX_TRIGGER,
    .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};
#endif

// prototypes
static void _msp_peripheral_gpio_init_for_dipsw(void);
static void _msp_peripheral_gpio_init_for_hw_ver(void);
static void _msp_peripheral_pwm_init(void);
static void _msp_peripheral_tim_init(void);
static void _msp_peripheral_uart_init(void);
static void _msp_peripheral_spi_init(void);

static void _msp_peripheral_gpio_init_for_dipsw(void) {
    DL_GPIO_initDigitalInputFeatures(DIPSW_1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(DIPSW_2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(DIPSW_3_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(DIPSW_4_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearInterruptStatus(DIPSW_PORT, DIPSW_1_PIN | DIPSW_2_PIN | DIPSW_3_PIN | DIPSW_4_PIN);
}

static void _msp_peripheral_gpio_init_for_hw_ver(void) {
    DL_GPIO_initDigitalInputFeatures(HW_VER_1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(HW_VER_2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(HW_VER_3_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearInterruptStatus(HW_VER_PORT, HW_VER_1_PIN | HW_VER_2_PIN | HW_VER_3_PIN);
}

static void _msp_peripheral_pwm_init(void) {
    // LED PWM Configuration
    // TIMA_PWM for RGB LED
    DL_TimerA_setClockConfig(LED_RG_TIM_INST, &g_TIMA_LED_CLOCK_CONFIG);
    DL_TimerA_initPWMMode(LED_RG_TIM_INST, &g_TIMA_LED_PWM_CONFIG);
    DL_TimerA_setCounterControl(LED_RG_TIM_INST, DL_TIMER_CZC_CCCTL0_ZCOND, DL_TIMER_CAC_CCCTL0_ACOND,
                                DL_TIMER_CLC_CCCTL0_LCOND);

    // Capture Compare 0 for Red LED
    DL_TimerA_setCaptureCompareOutCtl(LED_RG_TIM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_ENABLED,
                                      DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptCompUpdateMethod(LED_RG_TIM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
                                      DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(LED_RG_TIM_INST, 0, DL_TIMER_CC_0_INDEX);

    // Capture Compare 1 for Green LED
    DL_TimerA_setCaptureCompareOutCtl(LED_RG_TIM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_ENABLED,
                                      DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptCompUpdateMethod(LED_RG_TIM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
                                      DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptureCompareValue(LED_RG_TIM_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerA_enableClock(LED_RG_TIM_INST);
    DL_TimerA_setCCPDirection(LED_RG_TIM_INST, DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT);

    // TIMG_PWM for RGB LED
    DL_TimerG_setClockConfig(LED_B_TIM_INST, &g_TIMG_LED_CLOCK_CONFIG);
    DL_TimerG_initPWMMode(LED_B_TIM_INST, &g_TIMG_LED_PWM_CONFIG);
    DL_TimerG_setCounterControl(LED_B_TIM_INST, DL_TIMER_CZC_CCCTL0_ZCOND, DL_TIMER_CAC_CCCTL0_ACOND,
                                DL_TIMER_CLC_CCCTL0_LCOND);

    // Capture Compare 0 for Blue LED
    DL_TimerG_setCaptureCompareOutCtl(LED_B_TIM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_ENABLED,
                                      DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptCompUpdateMethod(LED_B_TIM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
                                      DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(LED_B_TIM_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerG_enableClock(LED_B_TIM_INST);
    DL_TimerG_setCCPDirection(LED_B_TIM_INST, DL_TIMER_CC0_OUTPUT);
}

static void _msp_peripheral_tim_init(void) {
    // DEBOUNCE Timer Configuration
    DL_TimerG_setClockConfig(DEBOUNCE_TIM_INST, &g_TIMG_DEBOUNCE_CLOCK_CONFIG);
    DL_TimerG_initTimerMode(DEBOUNCE_TIM_INST, &g_TIMG_DEBOUNCE_TIMER_CONFIG);
    DL_TimerG_enableClock(DEBOUNCE_TIM_INST);
}

static void _msp_peripheral_uart_init(void) {
#ifdef AXON_BOARD
    // ★デバッグ: 関数呼び出し確認
    volatile uint32_t debug_marker = 0xDEADBEEF;
    
    // ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
    UART_Regs *uart0 = (UART_Regs *)0x40108000UL;
    
    // ★最重要: GPIO ピン設定を最初に実行（TIドライバと同じ順序）
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM21, IOMUX_PINCM21_PF_UART0_TX);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM22, IOMUX_PINCM22_PF_UART0_RX);
    
    // Power ON (SYSCFG_DL_initPowerでもやっているが確実に実行)
    DL_UART_reset(uart0);
    DL_UART_enablePower(uart0);
    
    // Clock 32MHz SYSOSC
    DL_UART_ClockConfig clk = {
        .clockSel    = DL_UART_CLOCK_BUSCLK,
        .divideRatio = DL_UART_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_setClockConfig(uart0, &clk);
    
    // UARTモード設定
    DL_UART_Config cfg = {
        .mode        = DL_UART_MODE_NORMAL,
        .direction   = DL_UART_DIRECTION_TX_RX,
        .flowControl = DL_UART_FLOW_CONTROL_NONE,
        .parity      = DL_UART_PARITY_NONE,
        .wordLength  = DL_UART_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_STOP_BITS_ONE
    };
    DL_UART_init(uart0, &cfg);
    
    // Baudrate 115200 @ 32MHz (SYSOSC 32MHz)
    DL_UART_configBaudRate(uart0, 32000000, 115200);
    
    // FIFO enable
    DL_UART_enableFIFOs(uart0);
    DL_UART_setRXFIFOThreshold(uart0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_setTXFIFOThreshold(uart0, DL_UART_TX_FIFO_LEVEL_EMPTY);
    
    // 割り込み
    DL_UART_enableInterrupt(uart0, DL_UART_INTERRUPT_RX);
    NVIC_EnableIRQ(UART0_INT_IRQn);
    
    // ★最重要: UARTを有効化（これが無いと絶対動かない）
    DL_UART_enable(uart0);
    
    // ★デバッグ: 初期化完了マーカー
    debug_marker = 0xCAFEBABE;
#endif
}

static void _msp_peripheral_spi_init(void) {
#ifdef SOMA_BOARD

    // SPI Interrupt Configuration
    DL_SPI_setClockConfig(FRAM_SPI_INST, (DL_SPI_ClockConfig *) &g_FRAM_CLOCK_CONFIG);

    DL_SPI_init(FRAM_SPI_INST, (DL_SPI_Config *) &g_FRAM_SPI_CONFIG);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     16000000 = (32000000)/((1 + 0) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(FRAM_SPI_INST, 0);      // 16MHz (32MHz / ((1 + 0) * 2) = 16MHz)
    
    /* 高速動作(16MHz)用: サンプリング遅延を設定 */
    /* データサンプリングを2クロック遅延させる (FRAMのデータ受信遅延対応) */
    DL_SPI_setDelayedSampling(FRAM_SPI_INST, 1);  // 1クロックサイクル遅延 (推奨値: 1-3)
    
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(FRAM_SPI_INST, DL_SPI_RX_FIFO_LEVEL_ONE_FRAME, DL_SPI_TX_FIFO_LEVEL_ONE_FRAME);
    DL_SPI_enableInterrupt(FRAM_SPI_INST, (DL_SPI_INTERRUPT_RX));

    /* Enable module */
    DL_SPI_enable(FRAM_SPI_INST);
#endif
}

// overwrite
void SYSCFG_DL_init(void) {
    // ★重要: クロック設定を最初に実行(ペリフェラル初期化の前提条件)
    SYSCFG_DL_SYSCTL_init();
    
    // ペリフェラル電源制御
    SYSCFG_DL_initPower();
    
    // GPIO初期化（UART以外のペリフェラル用）
    // 注意: UART0ピン設定はここではなく_msp_peripheral_uart_init()内で実行
    SYSCFG_DL_GPIO_init();
    
    /* Module-Specific Initializations*/
    _msp_peripheral_pwm_init();
    _msp_peripheral_spi_init();
    
    // ★重要: UART初期化はGPIO初期化の後、他のペリフェラルの後に実行
    // GPIO初期化でピン設定が上書きされるのを防ぐため、最後に実行
    _msp_peripheral_uart_init();
    
    // _msp_peripheral_dma_init();
    SYSCFG_DL_AES_init();
    SYSCFG_DL_CRC_init();
    SYSCFG_DL_TRNG_init();
    SYSCFG_DL_SYSTICK_init();
    /* Ensure backup structures have no valid state */
}

// overwrite
void SYSCFG_DL_initPower(void) {
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(LED_RG_TIM_INST);
    DL_TimerG_reset(LED_B_TIM_INST);

#ifdef SOMA_BOARD
    DL_UART_reset(TG_UART_INST);
    DL_UART_reset(AXON_UART_INST);
    DL_SPI_reset(FRAM_SPI_INST);
#endif
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

#ifdef SOMA_BOARD
    DL_UART_enablePower(TG_UART_INST);
    DL_UART_enablePower(AXON_UART_INST);
    DL_SPI_enablePower(FRAM_SPI_INST);
#endif
#ifdef AXON_BOARD
    // ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
    DL_UART_enablePower((UART_Regs *)0x40108000UL);
#endif

    DL_AES_enablePower(AES);
    DL_CRC_enablePower(CRC);
    DL_TRNG_enablePower(TRNG);

    delay_cycles(POWER_STARTUP_DELAY);
}

// overwrite
void SYSCFG_DL_GPIO_init(void) {
    _msp_peripheral_gpio_init_for_dipsw();

    _msp_peripheral_gpio_init_for_hw_ver();
#ifdef SOMA_BOARD
    // 3色LED
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_R_IOMUX, LED_R_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_G_IOMUX, LED_G_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_B_IOMUX, LED_B_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(LED_RGB_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);
    DL_GPIO_clearPins(LED_RGB_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);

    // LEDポート
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_3_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_4_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_5_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_6_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_7_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_8_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(LED_PORT_9_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(LED_PORT, LED_PORT_1_PIN | LED_PORT_2_PIN | LED_PORT_3_PIN | LED_PORT_4_PIN | LED_PORT_5_PIN |
                                       LED_PORT_6_PIN | LED_PORT_7_PIN | LED_PORT_8_PIN | LED_PORT_9_PIN);
    DL_GPIO_clearPins(LED_PORT, LED_PORT_1_PIN | LED_PORT_2_PIN | LED_PORT_3_PIN | LED_PORT_4_PIN | LED_PORT_5_PIN |
                                    LED_PORT_6_PIN | LED_PORT_7_PIN | LED_PORT_8_PIN | LED_PORT_9_PIN);

    // MUX
    DL_GPIO_initDigitalOutputFeatures(MUX_ENABLE_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(MUX_S0_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(MUX_S1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(MUX_S2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(MUX_S3_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(MUX_PORT, MUX_ENABLE_PIN | MUX_S0_PIN | MUX_S1_PIN | MUX_S2_PIN | MUX_S3_PIN);
    DL_GPIO_clearPins(MUX_PORT, MUX_ENABLE_PIN | MUX_S0_PIN | MUX_S1_PIN | MUX_S2_PIN | MUX_S3_PIN);

    // PUSH SW
    DL_GPIO_initDigitalInputFeatures(PUSH_SW_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(PUSH_SW_PORT, PUSH_SW_PIN);
    DL_GPIO_setUpperPinsPolarity(PUSH_SW_PORT, DL_GPIO_PIN_19_EDGE_RISE);
    DL_GPIO_enableInterrupt(PUSH_SW_PORT, PUSH_SW_PIN);

    // FRAM
    // CSは手動制御用にGPIOで設定
    DL_GPIO_initDigitalOutputFeatures(FRAM_SPI_CS_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(FRAM_SPI_PORT, FRAM_SPI_CS_PIN);   // CSピンを出力に設定
    DL_GPIO_setPins(FRAM_SPI_PORT, FRAM_SPI_CS_PIN);        // CS Highに設定（非選択状態）

    DL_GPIO_initPeripheralOutputFunction(FRAM_SPI_SCLK_IOMUX, FRAM_SPI_SCLK_PF_FUNC);   // SCLK = SCK = POCI (コントローラ出力)
    DL_GPIO_initPeripheralInputFunction(FRAM_SPI_SO_IOMUX, FRAM_SPI_SO_PF_FUNC);        // SO = MISO = POCI (コントローラ入力)
    DL_GPIO_initPeripheralOutputFunction(FRAM_SPI_SI_IOMUX, FRAM_SPI_SI_PF_FUNC);       // SI = MOSI = POCI (コントローラ出力)

    // UART for TG
    DL_GPIO_initPeripheralOutputFunction(TG_UART_TX_IOMUX, TG_UART_TX_PF_FUNC);
    DL_GPIO_initPeripheralInputFunction(TG_UART_RX_IOMUX, TG_UART_RX_PF_FUNC);

    // UART for AXON
    DL_GPIO_initPeripheralOutputFunction(AXON_UART_TX_IOMUX, AXON_UART_TX_PF_FUNC);
    DL_GPIO_initPeripheralInputFunction(AXON_UART_RX_IOMUX, AXON_UART_RX_PF_FUNC);

    // UART IRQ
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_3_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_4_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_5_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_6_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_7_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_8_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(UART_IRQ_9_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(UART_IRQ_PORT, UART_IRQ_1_PIN | UART_IRQ_2_PIN | UART_IRQ_3_PIN | UART_IRQ_4_PIN |
                                                    UART_IRQ_5_PIN | UART_IRQ_6_PIN | UART_IRQ_7_PIN | UART_IRQ_8_PIN |
                                                    UART_IRQ_9_PIN);
    DL_GPIO_enableInterrupt(UART_IRQ_PORT, UART_IRQ_1_PIN | UART_IRQ_2_PIN | UART_IRQ_3_PIN | UART_IRQ_4_PIN |
                                               UART_IRQ_5_PIN | UART_IRQ_6_PIN | UART_IRQ_7_PIN | UART_IRQ_8_PIN |
                                               UART_IRQ_9_PIN);

    // set polarity
    DL_GPIO_setLowerPinsPolarity(GPIOA, UART_IRQ_1_EDGE_RISE_FALL | UART_IRQ_2_EDGE_RISE_FALL |
                                            UART_IRQ_3_EDGE_RISE_FALL | UART_IRQ_4_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOA, UART_IRQ_5_EDGE_RISE_FALL | UART_IRQ_6_EDGE_RISE_FALL |
                                            UART_IRQ_7_EDGE_RISE_FALL | UART_IRQ_8_EDGE_RISE_FALL |
                                            UART_IRQ_9_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOB, PUSH_SW_EDGE_RISE_FALL);
#endif /*SOMA_BOARD*/

#ifdef AXON_BOARD
    // 3色LED (R,G,B: PWM)
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_R_IOMUX, LED_R_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_G_IOMUX, LED_G_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initPeripheralOutputFunctionFeatures(LED_B_IOMUX, LED_B_PWM, DL_GPIO_INVERSION_DISABLE,
                                                 DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_LOW,
                                                 DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(LED_RGB_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);
    DL_GPIO_clearPins(LED_RGB_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN);

    // 7セグメントLED 1
    DL_GPIO_initDigitalOutputFeatures(SEG1_A_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_B_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_C_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_D_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_E_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_F_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG1_G_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(SEG1_PORT,
                         SEG1_A_PIN | SEG1_B_PIN | SEG1_C_PIN | SEG1_D_PIN | SEG1_E_PIN | SEG1_F_PIN | SEG1_G_PIN);
    DL_GPIO_clearPins(SEG1_PORT,
                      SEG1_A_PIN | SEG1_B_PIN | SEG1_C_PIN | SEG1_D_PIN | SEG1_E_PIN | SEG1_F_PIN | SEG1_G_PIN);

    // 7セグメントLED 2
    DL_GPIO_initDigitalOutputFeatures(SEG2_A_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_B_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_C_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_D_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_E_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_F_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(SEG2_G_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(SEG2_PORT,
                         SEG2_A_PIN | SEG2_B_PIN | SEG2_C_PIN | SEG2_D_PIN | SEG2_E_PIN | SEG2_F_PIN | SEG2_G_PIN);
    DL_GPIO_clearPins(SEG2_PORT,
                      SEG2_A_PIN | SEG2_B_PIN | SEG2_C_PIN | SEG2_D_PIN | SEG2_E_PIN | SEG2_F_PIN | SEG2_G_PIN);

    // escrow sw
    DL_GPIO_initDigitalInputFeatures(ESCROW_SW_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(ESCROW_SW_PORT, ESCROW_SW_PIN);
    DL_GPIO_enableInterrupt(ESCROW_SW_PORT, ESCROW_SW_PIN);

    // coin sensor
    DL_GPIO_initDigitalInputFeatures(COIN_SENSOR_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(COIN_SENSOR_PORT, COIN_SENSOR_PIN);
    DL_GPIO_enableInterrupt(COIN_SENSOR_PORT, COIN_SENSOR_PIN);

    // dial switch
    DL_GPIO_initDigitalInputFeatures(DIAL_SW_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(DIAL_SW_PORT, DIAL_SW_PIN);
    DL_GPIO_enableInterrupt(DIAL_SW_PORT, DIAL_SW_PIN);

    // soldout sw
    DL_GPIO_initDigitalInputFeatures(SOLDOUT_SW_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(SOLDOUT_SW_PORT, SOLDOUT_SW_PIN);
    DL_GPIO_enableInterrupt(SOLDOUT_SW_PORT, SOLDOUT_SW_PIN);

    // connector detect
    DL_GPIO_initDigitalInputFeatures(CONNECTOR_DET_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(CONNECTOR_DET_PORT, CONNECTOR_DET_PIN);
    DL_GPIO_enableInterrupt(CONNECTOR_DET_PORT, CONNECTOR_DET_PIN);

    // block solenoid
    DL_GPIO_initDigitalOutputFeatures(BLOCK_SOL_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(BLOCK_SOL_PORT, BLOCK_SOL_PIN);
    DL_GPIO_clearPins(BLOCK_SOL_PORT, BLOCK_SOL_PIN);

    // dial lock solenoid
    DL_GPIO_initDigitalOutputFeatures(DIAL_LOCK_SOL_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                      DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(DIAL_LOCK_SOL_PORT, DIAL_LOCK_SOL_PIN);
    DL_GPIO_clearPins(DIAL_LOCK_SOL_PORT, DIAL_LOCK_SOL_PIN);

    // door open close detect
    DL_GPIO_initDigitalInputFeatures(DOOR_OC_DET_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(DOOR_OC_DET_PORT, DOOR_OC_DET_PIN);
    DL_GPIO_enableInterrupt(DOOR_OC_DET_PORT, DOOR_OC_DET_PIN);

    // push sw
    DL_GPIO_initDigitalInputFeatures(PUSH_SW1_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(PUSH_SW2_IOMUX, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearInterruptStatus(PUSH_SW_PORT, PUSH_SW1_PIN | PUSH_SW2_PIN);
    DL_GPIO_enableInterrupt(PUSH_SW_PORT, PUSH_SW1_PIN | PUSH_SW2_PIN);

    // 注意: UART0 (PA10/PA11) のピン設定は _msp_peripheral_uart_init() 内で実施
    // ここでは設定しない（UART初期化の直前に設定することで上書き防止）

    // UART (追加のUART - UART2相当)
    DL_GPIO_initPeripheralOutputFunction(UART_TX_IOMUX, UART_TX_PF_FUNC);
    DL_GPIO_initPeripheralInputFunction(UART_RX_IOMUX, UART_RX_PF_FUNC);

    DL_GPIO_initDigitalInput(UART_IRQ_OUT_IOMUX);
    DL_GPIO_enableOutput(UART_PORT, UART_IRQ_OUT_PIN);

    // set polarity
    DL_GPIO_setLowerPinsPolarity(GPIOA, ESCROW_SW_EDGE_RISE_FALL | COIN_SENSOR_EDGE_RISE_FALL |
                                            CONNECTOR_DET_EDGE_RISE_FALL | DIAL_SW_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOA, SOLDOUT_SW_EDGE_RISE_FALL | DOOR_OC_DET_EDGE_RISE_FALL);

    DL_GPIO_setUpperPinsPolarity(GPIOB, PUSH_SW1_EDGE_RISE_FALL | PUSH_SW2_EDGE_RISE_FALL);
#endif
}

// overwrite
void SYSCFG_DL_SYSTICK_init(void) {
    /* Initialize the period to 1.00 ms */
    DL_SYSTICK_init(SYSTICK_PERIOD);
    DL_SYSTICK_enableInterrupt();
    DL_SYSTICK_enable();
}
