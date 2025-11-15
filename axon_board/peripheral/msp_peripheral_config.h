#ifndef __MSP_PERIPHERAL_CONFIG_H__
#define __MSP_PERIPHERAL_CONFIG_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

/* clang-format off */

#ifndef SOMA_BOARD
#ifndef AXON_BOARD
#error "SOMA_BOARD or AXON_BOARD is not defined"
#endif
#endif // SOMA_BOARD or AXON_BOARD

#ifdef MSPM0G_CPU_FREQ_80MHZ
#define BUS_CLK                                                 (40000000U) // 40 MHz
#else
#define BUS_CLK                                                 (32000000U) // 32 MHz
#endif

#define UART_BAUD_RATE                                          (115200U)
// 未使用　#define UART_IBRD_32_MHZ_115200_BAUD                            (17U)   // 32MHzの場合
// 未使用　#define UART_FBRD_32_MHZ_115200_BAUD                            (23U)   // 32MHzの場合
// #define UART_IBRD_40_MHZ_115200_BAUD                            (21U)    // 40MHzの場合
// #define UART_FBRD_40_MHZ_115200_BAUD                            (45U)    // 40MHzの場合
#define UART_RX_BUFFER_SIZE                                     (8U)

#ifdef MSPM0G_CPU_FREQ_80MHZ
#define TIM_LED_CLOCK_SOURCE_BUSCLK                             (DL_TIMER_CLOCK_BUSCLK)
#define TIM_LED_CLOCK_SOURCE_DIVIDE                             (DL_TIMER_CLOCK_DIVIDE_1)
#define TIM_LED_CLOCK_PRESCALE                                  (20U)
#define TIM_LED_PWM_PERIOD_COUNT                                (1000) // 4KHz

#define TIM_DEBOUNCE_CLOCK_SOURCE_BUSCLK                        (DL_TIMER_CLOCK_BUSCLK)
#define TIM_DEBOUNCE_CLOCK_SOURCE_DIVIDE                        (DL_TIMER_CLOCK_DIVIDE_1)
#define TIM_DEBOUNCE_CLOCK_PRESCALE                             (79U) // 1us per count

#define TIM_DEBOUNCE_TIMER_PERIOD_COUNT                         (99U) // 100us (x - 1)
#define TIM_DEBOUNCE_TIMER_MODE                                 (DL_TIMER_TIMER_MODE_ONE_SHOT)
#define TIM_DEBOUNCE_TIMER_START                                (DL_TIMER_STOP)
#else
#define TIM_LED_CLOCK_SOURCE_BUSCLK                             (DL_TIMER_CLOCK_BUSCLK)
#define TIM_LED_CLOCK_SOURCE_DIVIDE                             (DL_TIMER_CLOCK_DIVIDE_1)
#define TIM_LED_CLOCK_PRESCALE                                  (31U)
#define TIM_LED_PWM_PERIOD_COUNT                                (250) // 4KHz

#define TIM_DEBOUNCE_CLOCK_SOURCE_BUSCLK                        (DL_TIMER_CLOCK_BUSCLK)
#define TIM_DEBOUNCE_CLOCK_SOURCE_DIVIDE                        (DL_TIMER_CLOCK_DIVIDE_1)
#define TIM_DEBOUNCE_CLOCK_PRESCALE                             (31U) // 1us per count

#define TIM_DEBOUNCE_TIMER_PERIOD_COUNT                         (99U) // 100us (x - 1)
#define TIM_DEBOUNCE_TIMER_MODE                                 (DL_TIMER_TIMER_MODE_ONE_SHOT)
#define TIM_DEBOUNCE_TIMER_START                                (DL_TIMER_STOP)
#endif

// 共通ピン定義
// RGB_LED

#define LED_RGB_PORT                                            (GPIOB)
#define LED_RG_TIM_INST                                         (TIMA1)
#define LED_RG_TIM_IRQ_INST                                     (TIMA1_INT_IRQn)
#define LED_RG_TIM_IRQ_HANDLER                                  (TIMA1_IRQHandler)
#define LED_R_IOMUX                                             (IOMUX_PINCM12)
#define LED_R_PWM                                               (IOMUX_PINCM12_PF_TIMA1_CCP0)
#define LED_R_PIN                                               (DL_GPIO_PIN_0)
#define LED_G_IOMUX                                             (IOMUX_PINCM13)
#define LED_G_PWM                                               (IOMUX_PINCM13_PF_TIMA1_CCP1)
#define LED_G_PIN                                               (DL_GPIO_PIN_1)
#define LED_B_TIM_INST                                          (TIMG6)
#define LED_B_TIM_IRQ_INST                                      (TIMG6_INT_IRQn)
#define LED_B_TIM_IRQ_HANDLER                                   (TIMG6_IRQHandler)
#define LED_B_IOMUX                                             (IOMUX_PINCM15)
#define LED_B_PWM                                               (IOMUX_PINCM15_PF_TIMG6_CCP0)
#define LED_B_PIN                                               (DL_GPIO_PIN_2)

// デバウンスタイマレジスタ
#define DEBOUNCE_TIM_INST                                       (TIMG0)

// SOMA基板用ピン定義
#ifdef SOMA_BOARD

// UART IRQ
#define UART_IRQ_PORT                                           (GPIOA)
#define UART_INT_IRQn                                           (GPIOA_INT_IRQn)
#define UART_IRQ_1_IOMUX                                        (IOMUX_PINCM1)
#define UART_IRQ_1_PIN                                          (DL_GPIO_PIN_0)
#define UART_IRQ_1_IIDX                                         (DL_GPIO_IIDX_DIO0)
#define UART_IRQ_1_EDGE_RISE_FALL                               (DL_GPIO_PIN_0_EDGE_RISE_FALL)
#define UART_IRQ_2_IOMUX                                        (IOMUX_PINCM2)
#define UART_IRQ_2_PIN                                          (DL_GPIO_PIN_1)
#define UART_IRQ_2_IIDX                                         (DL_GPIO_IIDX_DIO1)
#define UART_IRQ_2_EDGE_RISE_FALL                               (DL_GPIO_PIN_1_EDGE_RISE_FALL)
#define UART_IRQ_3_IOMUX                                        (IOMUX_PINCM20)
#define UART_IRQ_3_PIN                                          (DL_GPIO_PIN_9)
#define UART_IRQ_3_IIDX                                         (DL_GPIO_IIDX_DIO9)
#define UART_IRQ_3_EDGE_RISE_FALL                               (DL_GPIO_PIN_9_EDGE_RISE_FALL)
#define UART_IRQ_4_IOMUX                                        (IOMUX_PINCM37)
#define UART_IRQ_4_PIN                                          (DL_GPIO_PIN_15)
#define UART_IRQ_4_IIDX                                         (DL_GPIO_IIDX_DIO15)
#define UART_IRQ_4_EDGE_RISE_FALL                               (DL_GPIO_PIN_15_EDGE_RISE_FALL)
#define UART_IRQ_5_IOMUX                                        (IOMUX_PINCM38)
#define UART_IRQ_5_PIN                                          (DL_GPIO_PIN_16)
#define UART_IRQ_5_IIDX                                         (DL_GPIO_IIDX_DIO16)
#define UART_IRQ_5_EDGE_RISE_FALL                               (DL_GPIO_PIN_16_EDGE_RISE_FALL)
#define UART_IRQ_6_IOMUX                                        (IOMUX_PINCM60)
#define UART_IRQ_6_PIN                                          (DL_GPIO_PIN_27)
#define UART_IRQ_6_IIDX                                         (DL_GPIO_IIDX_DIO27)
#define UART_IRQ_6_EDGE_RISE_FALL                               (DL_GPIO_PIN_27_EDGE_RISE_FALL)
#define UART_IRQ_7_IOMUX                                        (IOMUX_PINCM3)
#define UART_IRQ_7_PIN                                          (DL_GPIO_PIN_28)
#define UART_IRQ_7_IIDX                                         (DL_GPIO_IIDX_DIO28)
#define UART_IRQ_7_EDGE_RISE_FALL                               (DL_GPIO_PIN_28_EDGE_RISE_FALL)
#define UART_IRQ_8_IOMUX                                        (IOMUX_PINCM4)
#define UART_IRQ_8_PIN                                          (DL_GPIO_PIN_29)
#define UART_IRQ_8_IIDX                                         (DL_GPIO_IIDX_DIO29)
#define UART_IRQ_8_EDGE_RISE_FALL                               (DL_GPIO_PIN_29_EDGE_RISE_FALL)
#define UART_IRQ_9_IOMUX                                        (IOMUX_PINCM5)
#define UART_IRQ_9_PIN                                          (DL_GPIO_PIN_30)
#define UART_IRQ_9_IIDX                                         (DL_GPIO_IIDX_DIO30)
#define UART_IRQ_9_EDGE_RISE_FALL                               (DL_GPIO_PIN_30_EDGE_RISE_FALL)

// PUSH SW
#define PUSH_SW_PORT                                            (GPIOB)
#define PUSH_SW_INT_IRQn                                        (GPIOB_INT_IRQn)
#define PUSH_SW_IOMUX                                           (IOMUX_PINCM45)
#define PUSH_SW_PIN                                             (DL_GPIO_PIN_19)
#define PUSH_SW_IIDX                                            (DL_GPIO_IIDX_DIO19)
#define PUSH_SW_EDGE_RISE_FALL                                  (DL_GPIO_PIN_19_EDGE_RISE_FALL)

// LED
#define LED_PORT                                                (GPIOB)
#define LED_PORT_1_IOMUX                                        (IOMUX_PINCM16)
#define LED_PORT_1_PIN                                          (DL_GPIO_PIN_3)
#define LED_PORT_2_IOMUX                                        (IOMUX_PINCM17)
#define LED_PORT_2_PIN                                          (DL_GPIO_PIN_4)
#define LED_PORT_3_IOMUX                                        (IOMUX_PINCM18)
#define LED_PORT_3_PIN                                          (DL_GPIO_PIN_5)
#define LED_PORT_4_IOMUX                                        (IOMUX_PINCM23)
#define LED_PORT_4_PIN                                          (DL_GPIO_PIN_6)
#define LED_PORT_5_IOMUX                                        (IOMUX_PINCM24)
#define LED_PORT_5_PIN                                          (DL_GPIO_PIN_7)
#define LED_PORT_6_IOMUX                                        (IOMUX_PINCM25)
#define LED_PORT_6_PIN                                          (DL_GPIO_PIN_8)
#define LED_PORT_7_IOMUX                                        (IOMUX_PINCM26)
#define LED_PORT_7_PIN                                          (DL_GPIO_PIN_9)
#define LED_PORT_8_IOMUX                                        (IOMUX_PINCM27)
#define LED_PORT_8_PIN                                          (DL_GPIO_PIN_10)
#define LED_PORT_9_IOMUX                                        (IOMUX_PINCM28)
#define LED_PORT_9_PIN                                          (DL_GPIO_PIN_11)

// MUX
#define MUX_PORT                                                (GPIOB)
#define MUX_ENABLE_IOMUX                                        (IOMUX_PINCM29)
#define MUX_ENABLE_PIN                                          (DL_GPIO_PIN_12)
#define MUX_S0_IOMUX                                            (IOMUX_PINCM30)
#define MUX_S0_PIN                                              (DL_GPIO_PIN_13)
#define MUX_S1_IOMUX                                            (IOMUX_PINCM31)
#define MUX_S1_PIN                                              (DL_GPIO_PIN_14)
#define MUX_S2_IOMUX                                            (IOMUX_PINCM32)
#define MUX_S2_PIN                                              (DL_GPIO_PIN_15)
#define MUX_S3_IOMUX                                            (IOMUX_PINCM33)
#define MUX_S3_PIN                                              (DL_GPIO_PIN_16)

// UART
#define TG_UART_INST                                            (UART2)
#define TG_UART_INT_IRQn                                        (UART2_INT_IRQn)
#define TG_UART_IRQ_HANDLER                                     (UART2_IRQHandler)
#define TG_UART_TX_IOMUX                                        (IOMUX_PINCM43)
#define TG_UART_TX_PF_FUNC                                      (IOMUX_PINCM43_PF_UART2_TX)
#define TG_UART_TX_PIN                                          (DL_GPIO_PIN_17)
#define TG_UART_RX_PF_FUNC                                      (IOMUX_PINCM44_PF_UART2_RX)
#define TG_UART_RX_IOMUX                                        (IOMUX_PINCM44)
#define TG_UART_RX_PIN                                          (DL_GPIO_PIN_18)
#define TG_UART_RX_DMA_CHANNEL                                  (0)
#define TG_UART_RX_DMA_TRIG                                     (DMA_UART2_RX_TRIG)
#define AXON_UART_INST                                          (UART0)
#define AXON_UART_INT_IRQn                                      (UART0_INT_IRQn)
#define AXON_UART_IRQ_HANDLER                                   (UART0_IRQHandler)
#define AXON_UART_TX_IOMUX                                      (IOMUX_PINCM21)
#define AXON_UART_TX_PF_FUNC                                    (IOMUX_PINCM21_PF_UART0_TX)
#define AXON_UART_TX_PIN                                        (DL_GPIO_PIN_10)
#define AXON_UART_RX_IOMUX                                      (IOMUX_PINCM22)
#define AXON_UART_RX_PF_FUNC                                    (IOMUX_PINCM22_PF_UART0_RX)
#define AXON_UART_RX_PIN                                        (DL_GPIO_PIN_11)
#define AXON_UART_RX_DMA_CHANNEL                                (1)
#define AXON_UART_RX_DMA_TRIG                                   (DMA_UART0_RX_TRIG)

#define ESP32_UART_PORT                                         (GPIOA)
#define ESP32_UART_TX_IOMUX                                     (IOMUX_PINCM59)
#define ESP32_UART_TX_PF_FUNC                                   (IOMUX_PINCM59_PF_UART3_TX)
#define ESP32_UART_TX_PIN                                       (DL_GPIO_PIN_26)
#define ESP32_UART_RX_IOMUX                                     (IOMUX_PINCM55)
#define ESP32_UART_RX_PF_FUNC                                   (IOMUX_PINCM55_PF_UART3_RX)
#define ESP32_UART_RX_PIN                                       (DL_GPIO_PIN_25)

// FRAM
#define FRAM_SPI_INST                                           (SPI0)
#define FRAM_SPI_PORT                                           (GPIOA)
#define FRAM_SPI_INT_IRQn                                       (SPI0_INT_IRQn)
#define FRAM_SPI_IRQ_HANDLER                                    (SPI0_IRQHandler)
#define FRAM_SPI_PORT                                           (GPIOA)
#define FRAM_SPI_INT_IRQn                                       (SPI0_INT_IRQn)
#define FRAM_SPI_CS_IOMUX                                       (IOMUX_PINCM19)
#define FRAM_SPI_CS_PF_FUNC                                     (IOMUX_PINCM19_PF_SPI0_CS0)
#define FRAM_SPI_CS_PIN                                         (DL_GPIO_PIN_8)
#define FRAM_SPI_SCLK_IOMUX                                     (IOMUX_PINCM34)
#define FRAM_SPI_SCLK_PF_FUNC                                   (IOMUX_PINCM34_PF_SPI0_SCLK)
#define FRAM_SPI_SCLK_PIN                                       (DL_GPIO_PIN_12)
#define FRAM_SPI_SO_IOMUX                                       (IOMUX_PINCM35)
#define FRAM_SPI_SO_PF_FUNC                                     (IOMUX_PINCM35_PF_SPI0_POCI)
#define FRAM_SPI_SO_PIN                                         (DL_GPIO_PIN_13)
#define FRAM_SPI_SI_IOMUX                                       (IOMUX_PINCM36)
#define FRAM_SPI_SI_PF_FUNC                                     (IOMUX_PINCM36_PF_SPI0_PICO)
#define FRAM_SPI_SI_PIN                                         (DL_GPIO_PIN_14)

// DIPSW
#define DIPSW_PORT                                              (GPIOB)
#define DIPSW_1_IOMUX                                           (IOMUX_PINCM52)
#define DIPSW_1_PIN                                             (DL_GPIO_PIN_24)
#define DIPSW_2_IOMUX                                           (IOMUX_PINCM56)
#define DIPSW_2_PIN                                             (DL_GPIO_PIN_25)
#define DIPSW_3_IOMUX                                           (IOMUX_PINCM57)
#define DIPSW_3_PIN                                             (DL_GPIO_PIN_26)
#define DIPSW_4_IOMUX                                           (IOMUX_PINCM58)
#define DIPSW_4_PIN                                             (DL_GPIO_PIN_27)

// HW_VER
#define HW_VER_PORT                                             (GPIOB)
#define HW_VER_1_IOMUX                                          (IOMUX_PINCM49)
#define HW_VER_1_PIN                                            (DL_GPIO_PIN_21)
#define HW_VER_2_IOMUX                                          (IOMUX_PINCM50)
#define HW_VER_2_PIN                                            (DL_GPIO_PIN_22)
#define HW_VER_3_IOMUX                                          (IOMUX_PINCM51)
#define HW_VER_3_PIN                                            (DL_GPIO_PIN_23)
#endif // SOMA_BOARD

// AXON基板用ピン定義
#ifdef AXON_BOARD

// 7セグメントLED 1
#define SEG1_PORT                                               (GPIOB)
#define SEG1_A_IOMUX                                            (IOMUX_PINCM16)
#define SEG1_A_PIN                                              (DL_GPIO_PIN_3)
#define SEG1_B_IOMUX                                            (IOMUX_PINCM17)
#define SEG1_B_PIN                                              (DL_GPIO_PIN_4)
#define SEG1_C_IOMUX                                            (IOMUX_PINCM18)
#define SEG1_C_PIN                                              (DL_GPIO_PIN_5)
#define SEG1_D_IOMUX                                            (IOMUX_PINCM23)
#define SEG1_D_PIN                                              (DL_GPIO_PIN_6)
#define SEG1_E_IOMUX                                            (IOMUX_PINCM24)
#define SEG1_E_PIN                                              (DL_GPIO_PIN_7)
#define SEG1_F_IOMUX                                            (IOMUX_PINCM25)
#define SEG1_F_PIN                                              (DL_GPIO_PIN_8)
#define SEG1_G_IOMUX                                            (IOMUX_PINCM26)
#define SEG1_G_PIN                                              (DL_GPIO_PIN_9)
#define SEG1_PINS_MASK                                          (DL_GPIO_PIN_3 | \
                                                                 DL_GPIO_PIN_4 | \
                                                                 DL_GPIO_PIN_5 | \
                                                                 DL_GPIO_PIN_6 | \
                                                                 DL_GPIO_PIN_7 | \
                                                                 DL_GPIO_PIN_8 | \
                                                                 DL_GPIO_PIN_9)
#define SEG1_BIT_OFFSET                                         (3U)

// 7セグメントLED 2
#define SEG2_PORT                                               (GPIOB)
#define SEG2_A_IOMUX                                            (IOMUX_PINCM27)
#define SEG2_A_PIN                                              (DL_GPIO_PIN_10)
#define SEG2_B_IOMUX                                            (IOMUX_PINCM28)
#define SEG2_B_PIN                                              (DL_GPIO_PIN_11)
#define SEG2_C_IOMUX                                            (IOMUX_PINCM29)
#define SEG2_C_PIN                                              (DL_GPIO_PIN_12)
#define SEG2_D_IOMUX                                            (IOMUX_PINCM30)
#define SEG2_D_PIN                                              (DL_GPIO_PIN_13)
#define SEG2_E_IOMUX                                            (IOMUX_PINCM31)
#define SEG2_E_PIN                                              (DL_GPIO_PIN_14)
#define SEG2_F_IOMUX                                            (IOMUX_PINCM32)
#define SEG2_F_PIN                                              (DL_GPIO_PIN_15)
#define SEG2_G_IOMUX                                            (IOMUX_PINCM33)
#define SEG2_G_PIN                                              (DL_GPIO_PIN_16)
#define SEG2_PINS_MASK                                          (DL_GPIO_PIN_10 | \
                                                                 DL_GPIO_PIN_11 | \
                                                                 DL_GPIO_PIN_12 | \
                                                                 DL_GPIO_PIN_13 | \
                                                                 DL_GPIO_PIN_14 | \
                                                                 DL_GPIO_PIN_15 | \
                                                                 DL_GPIO_PIN_16)
#define SEG2_BIT_OFFSET                                         (10U)

// ESCROW SW
#define ESCROW_SW_PORT                                          (GPIOA)
#define ESCROW_SW_INT_IRQn                                      (GPIOA_INT_IRQn)
#define ESCROW_SW_IOMUX                                         (IOMUX_PINCM1)
#define ESCROW_SW_PIN                                           (DL_GPIO_PIN_0)
#define ESCROW_SW_IIDX                                          (DL_GPIO_IIDX_DIO0)
#define ESCROW_SW_EDGE_RISE_FALL                                (DL_GPIO_PIN_0_EDGE_RISE_FALL)

// COIN SENSOR
#define COIN_SENSOR_PORT                                        (GPIOA)
#define COIN_SENSOR_INT_IRQn                                    (GPIOA_INT_IRQn)
#define COIN_SENSOR_IOMUX                                       (IOMUX_PINCM2)
#define COIN_SENSOR_PIN                                         (DL_GPIO_PIN_1)
#define COIN_SENSOR_IIDX                                        (DL_GPIO_IIDX_DIO1)
#define COIN_SENSOR_EDGE_RISE_FALL                              (DL_GPIO_PIN_1_EDGE_RISE_FALL)

// BLOCK SOLENOID
#define BLOCK_SOL_PORT                                          (GPIOA)
#define BLOCK_SOL_IOMUX                                         (IOMUX_PINCM14)
#define BLOCK_SOL_PIN                                           (DL_GPIO_PIN_7)

// CONNECTOR DETECT
#define CONNECTOR_DET_PORT                                      (GPIOA)
#define CONNECTOR_DET_INT_IRQn                                  (GPIOA_INT_IRQn)
#define CONNECTOR_DET_IOMUX                                     (IOMUX_PINCM20)
#define CONNECTOR_DET_PIN                                       (DL_GPIO_PIN_9)
#define CONNECTOR_DET_IIDX                                      (DL_GPIO_IIDX_DIO9)
#define CONNECTOR_DET_EDGE_RISE_FALL                            (DL_GPIO_PIN_9_EDGE_RISE_FALL)

// UART
#define UART_PORT                                               (GPIOA)
#define UART_INST                                               (UART0)
#define UART_INST_IRQn                                          (UART0_INT_IRQn)
#define UART_INST_IRQ_HANDLER                                   (UART0_IRQHandler)
#define UART_TX_IOMUX                                           (IOMUX_PINCM21)
#define UART_TX_PF_FUNC                                         (IOMUX_PINCM21_PF_UART0_TX)
#define UART_TX_PIN                                             (DL_GPIO_PIN_10)
#define UART_RX_IOMUX                                           (IOMUX_PINCM22)
#define UART_RX_PF_FUNC                                         (IOMUX_PINCM22_PF_UART0_RX)
#define UART_RX_PIN                                             (DL_GPIO_PIN_11)
#define UART_IRQ_OUT_IOMUX                                      (IOMUX_PINCM54)
#define UART_IRQ_OUT_PIN                                        (DL_GPIO_PIN_24)

// DIAL SW
#define DIAL_SW_PORT                                            (GPIOA)
#define DIAL_SW_INT_IRQn                                        (GPIOA_INT_IRQn)
#define DIAL_SW_IOMUX                                           (IOMUX_PINCM37)
#define DIAL_SW_PIN                                             (DL_GPIO_PIN_15)
#define DIAL_SW_IIDX                                            (DL_GPIO_IIDX_DIO15)
#define DIAL_SW_EDGE_RISE_FALL                                  (DL_GPIO_PIN_15_EDGE_RISE_FALL)

// SOLDOUT SW
#define SOLDOUT_SW_PORT                                         (GPIOA)
#define SOLDOUT_SW_INT_IRQn                                     (GPIOA_INT_IRQn)
#define SOLDOUT_SW_IOMUX                                        (IOMUX_PINCM38)
#define SOLDOUT_SW_PIN                                          (DL_GPIO_PIN_16)
#define SOLDOUT_SW_IIDX                                         (DL_GPIO_IIDX_DIO16)
#define SOLDOUT_SW_EDGE_RISE_FALL                               (DL_GPIO_PIN_16_EDGE_RISE_FALL)

// DIAL LOCK SOLENOID(仮想現金)
#define DIAL_LOCK_SOL_PORT                                      (GPIOA)
#define DIAL_LOCK_SOL_IOMUX                                     (IOMUX_PINCM39)
#define DIAL_LOCK_SOL_PIN                                       (DL_GPIO_PIN_17)

// GPIO RESERVE
#define DOOR_OC_DET_PORT                                        (GPIOA)
#define DOOR_OC_DET_IOMUX                                       (IOMUX_PINCM47)
#define DOOR_OC_DET_PIN                                         (DL_GPIO_PIN_22)
#define DOOR_OC_DET_IIDX                                        (DL_GPIO_IIDX_DIO22)
#define DOOR_OC_DET_EDGE_RISE_FALL                              (DL_GPIO_PIN_22_EDGE_RISE_FALL)

// PUSH SW
#define PUSH_SW_PORT                                            (GPIOB)
#define PUSH_SW_INT_IRQn                                        (GPIOB_INT_IRQn)
#define PUSH_SW1_IOMUX                                          (IOMUX_PINCM45)
#define PUSH_SW1_PIN                                            (DL_GPIO_PIN_19)
#define PUSH_SW1_IIDX                                           (DL_GPIO_IIDX_DIO19)
#define PUSH_SW1_EDGE_RISE_FALL                                 (DL_GPIO_PIN_19_EDGE_RISE_FALL)
#define PUSH_SW2_IOMUX                                          (IOMUX_PINCM48)
#define PUSH_SW2_PIN                                            (DL_GPIO_PIN_20)
#define PUSH_SW2_IIDX                                           (DL_GPIO_IIDX_DIO20)
#define PUSH_SW2_EDGE_RISE_FALL                                 (DL_GPIO_PIN_20_EDGE_RISE_FALL)

// DIPSW
#define DIPSW_PORT                                              (GPIOB)
#define DIPSW_1_IOMUX                                           (IOMUX_PINCM52)
#define DIPSW_1_PIN                                             (DL_GPIO_PIN_24)
#define DIPSW_2_IOMUX                                           (IOMUX_PINCM56)
#define DIPSW_2_PIN                                             (DL_GPIO_PIN_25)
#define DIPSW_3_IOMUX                                           (IOMUX_PINCM57)
#define DIPSW_3_PIN                                             (DL_GPIO_PIN_26)
#define DIPSW_4_IOMUX                                           (IOMUX_PINCM58)
#define DIPSW_4_PIN                                             (DL_GPIO_PIN_27)

// HW_VER
#define HW_VER_PORT                                             (GPIOB)
#define HW_VER_1_IOMUX                                          (IOMUX_PINCM49)
#define HW_VER_1_PIN                                            (DL_GPIO_PIN_21)
#define HW_VER_2_IOMUX                                          (IOMUX_PINCM50)
#define HW_VER_2_PIN                                            (DL_GPIO_PIN_22)
#define HW_VER_3_IOMUX                                          (IOMUX_PINCM51)
#define HW_VER_3_PIN                                            (DL_GPIO_PIN_23)
#endif // AXON_BOARD

/* clang-format on */
#endif // __MSP_PERIPHERAL_CONFIG_H__