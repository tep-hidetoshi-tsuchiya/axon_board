#ifndef __DRIVER_CONFIG_H__
#define __DRIVER_CONFIG_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "driver/UART.h"
#include "driver/dma/DMAMSPM0.h"
#include "driver/systick.h"
#include "driver/uart/UARTMSPM0.h"
#include "peripheral/msp_peripheral_config.h"

#if defined(SOMA_BOARD)
#define CONFIG_UART_COUNT       (2)
#define CONFIG_UART_BUFFER_SIZE (48)
#define S2A_UART_INST           AXON_UART_INST
#define S2A_UART_IRQ            AXON_UART_INT_IRQn
#define S2A_UART_RX_PIN         AXON_UART_RX_IOMUX
#define S2A_UART_RX_PF_FUNC     AXON_UART_RX_PF_FUNC
#define S2A_UART_TX_PIN         AXON_UART_TX_IOMUX
#define S2A_UART_TX_PF_FUNC     AXON_UART_TX_PF_FUNC
#define S2A_UART_TX_TRIG        DMA_UART0_TX_TRIG
#define S2A_UART_RX_TRIG        DMA_UART0_RX_TRIG
#define T2S_UART_INST           TG_UART_INST
#define T2S_UART_IRQ            TG_UART_INT_IRQn
#define T2S_UART_RX_PIN         TG_UART_RX_IOMUX
#define T2S_UART_RX_PF_FUNC     TG_UART_RX_PF_FUNC
#define T2S_UART_TX_PIN         TG_UART_TX_IOMUX
#define T2S_UART_TX_PF_FUNC     TG_UART_TX_PF_FUNC
#define T2S_UART_TX_TRIG        DMA_UART2_TX_TRIG
#define T2S_UART_RX_TRIG        DMA_UART2_RX_TRIG

#define CONFIG_DMA_COUNT         (2)
#define CONFIG_DMA_CHANNEL_COUNT (2)
#define DEFAULT_DMA_PRIORITY     (31)
#define DMA0_TX_TRIG             (S2A_UART_TX_TRIG)
#define DMA0_RX_TRIG             (S2A_UART_RX_TRIG)
#define DMA1_TX_TRIG             (T2S_UART_TX_TRIG)
#define DMA1_RX_TRIG             (T2S_UART_RX_TRIG)
#elif defined(AXON_BOARD)
#define CONFIG_UART_COUNT       (1)
#define CONFIG_UART_BUFFER_SIZE (48)
// ★修正完了: TI標準UART0アドレス使用（MSPM0G3507データシートTable 3-1準拠）
// UART0 Base Address: 0x40108000 (PA10/PA11)
#define S2A_UART_INST           ((UART_Regs *)0x40108000UL)
#define S2A_UART_IRQ            UART0_INT_IRQn
#define S2A_UART_RX_PIN         UART_RX_IOMUX
#define S2A_UART_RX_PF_FUNC     UART_RX_PF_FUNC
#define S2A_UART_TX_PIN         UART_TX_IOMUX
#define S2A_UART_TX_PF_FUNC     UART_TX_PF_FUNC

#define CONFIG_DMA_COUNT         (1)
#define CONFIG_DMA_CHANNEL_COUNT (1)
#define DEFAULT_DMA_PRIORITY     (31)
#define DMA0_TX_TRIG             (DMA_UART0_TX_TRIG)
#define DMA0_RX_TRIG             (DMA_UART0_RX_TRIG)
#endif  // SOMA_BOARD

extern const uint8_t       CONFIG_UART_0;
extern const uint8_t       CONFIG_UART_1;
extern const uint_least8_t UART_count;
extern const UART_Config   UART_config[];

#endif  // __DRIVER_CONFIG_H__