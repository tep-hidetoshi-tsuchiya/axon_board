#include "driver_config.h"

/*
 *  ======== DMA ========
 */
const uint8_t CONFIG_DMA_0 = 0;
const uint8_t CONFIG_DMA_1 = 1;
const uint8_t DMA_count    = CONFIG_DMA_COUNT;

DMAMSPM0_Object DMAObject[CONFIG_DMA_CHANNEL_COUNT] = {
    {.dmaTransfer =
         {
             .txTrigger              = DMA0_TX_TRIG,
             .txTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
             .rxTrigger              = DMA0_RX_TRIG,
             .rxTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
             .transferMode           = DL_DMA_SINGLE_TRANSFER_MODE,
             .extendedMode           = DL_DMA_NORMAL_MODE,
             .destWidth              = DL_DMA_WIDTH_BYTE,
             .srcWidth               = DL_DMA_WIDTH_BYTE,
             .destIncrement          = DL_DMA_ADDR_INCREMENT,
             .dmaChannel             = 0,
             .dmaTransferSource      = NULL,
             .dmaTransferDestination = NULL,
             .enableDMAISR           = false,
         }},
#if CONFIG_DMA_COUNT > 1
    {.dmaTransfer =
         {
             .txTrigger              = DMA1_TX_TRIG,
             .txTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
             .rxTrigger              = DMA1_RX_TRIG,
             .rxTriggerType          = DL_DMA_TRIGGER_TYPE_EXTERNAL,
             .transferMode           = DL_DMA_SINGLE_TRANSFER_MODE,
             .extendedMode           = DL_DMA_NORMAL_MODE,
             .destWidth              = DL_DMA_WIDTH_BYTE,
             .srcWidth               = DL_DMA_WIDTH_BYTE,
             .destIncrement          = DL_DMA_ADDR_INCREMENT,
             .dmaChannel             = 1,
             .dmaTransferSource      = NULL,
             .dmaTransferDestination = NULL,
             .enableDMAISR           = false,
         }},
#endif  // CONFIG_DMA_COUNT > 1
};

static const DMAMSPM0_HWAttrs DMAMSP0HWAttrs[CONFIG_DMA_COUNT] = {
    {
        .dmaIsrFxn          = NULL,
        .intPriority        = DEFAULT_DMA_PRIORITY,
        .roundRobinPriority = 0,
    },
#if CONFIG_DMA_COUNT > 1
    {
        .dmaIsrFxn          = NULL,
        .intPriority        = DEFAULT_DMA_PRIORITY,
        .roundRobinPriority = 0,
    },
#endif  // CONFIG_DMA_COUNT > 1
};
const DMAMSPM0_Cfg DMAMSPM0_Config[CONFIG_DMA_COUNT] = {
    {
        &DMAMSP0HWAttrs[CONFIG_DMA_0],
        &DMAObject[CONFIG_DMA_0],
    },
#if CONFIG_DMA_COUNT > 1
    {
        &DMAMSP0HWAttrs[CONFIG_DMA_1],
        &DMAObject[CONFIG_DMA_1],
    },
#endif  // CONFIG_DMA_COUNT > 1
};

/*
 *  ======== UART ========
 */
const uint8_t       CONFIG_UART_0 = 0;
const uint8_t       CONFIG_UART_1 = 1;
const uint_least8_t UART_count    = CONFIG_UART_COUNT;

typedef struct {
    uint8_t rxBufPtr[CONFIG_UART_BUFFER_SIZE];
    uint8_t txBufPtr[CONFIG_UART_BUFFER_SIZE];
} UART_Buffer;

UART_Buffer g_uart_buffers[CONFIG_UART_COUNT] = {
    {
        .rxBufPtr = {0},
        .txBufPtr = {0},
    },
#if CONFIG_UART_COUNT == 2
    {
        .rxBufPtr = {0},
        .txBufPtr = {0},
    },
#endif  // CONFIG_UART_COUNT == 2
#if CONFIG_UART_COUNT > 3
#error "CONFIG_UART_COUNT is set to an unsupported value"
#endif  // CONFIG_UART_COUNT > 3
};

static const UARTMSP_HWAttrs UARTMSPHWAttrs[CONFIG_UART_COUNT] = {
    {
        .regs          = S2A_UART_INST,
        .irq           = S2A_UART_IRQ,
        .rxPin         = S2A_UART_RX_PIN,
        .rxPinFunction = S2A_UART_RX_PF_FUNC,
        .txPin         = S2A_UART_TX_PIN,
        .txPinFunction = S2A_UART_TX_PF_FUNC,
        .mode          = DL_UART_MODE_NORMAL,
        .direction     = DL_UART_DIRECTION_TX_RX,
        .flowControl   = DL_UART_FLOW_CONTROL_NONE,
        .clockSource   = DL_UART_CLOCK_BUSCLK,
        .clockDivider  = DL_UART_CLOCK_DIVIDE_RATIO_1,
        .rxIntFifoThr  = DL_UART_RX_FIFO_LEVEL_ONE_ENTRY,
        .txIntFifoThr  = DL_UART_TX_FIFO_LEVEL_EMPTY,
    },
#if CONFIG_UART_COUNT > 1
    {
        .regs          = T2S_UART_INST,
        .irq           = T2S_UART_IRQ,
        .rxPin         = T2S_UART_RX_PIN,
        .rxPinFunction = T2S_UART_RX_PF_FUNC,
        .txPin         = T2S_UART_TX_PIN,
        .txPinFunction = T2S_UART_TX_PF_FUNC,
        .mode          = DL_UART_MODE_NORMAL,
        .direction     = DL_UART_DIRECTION_TX_RX,
        .flowControl   = DL_UART_FLOW_CONTROL_NONE,
        .clockSource   = DL_UART_CLOCK_BUSCLK,
        .clockDivider  = DL_UART_CLOCK_DIVIDE_RATIO_1,
        .rxIntFifoThr  = DL_UART_RX_FIFO_LEVEL_ONE_ENTRY,
        .txIntFifoThr  = DL_UART_TX_FIFO_LEVEL_EMPTY,
    },
#endif  // CONFIG_UART_COUNT > 1
};

UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object = {.supportFxns        = &UARTMSPSupportFxns,
                   .buffersSupported   = true,
                   .eventsSupported    = false,
                   .callbacksSupported = true,
                   .dmaSupported       = true,
                   .noOfDMAChannels    = 1,
                   .rxDmaChannel       = CONFIG_DMA_0,
                   .DMA_Handle         = (DMAMSPM0_Handle)&DMAMSPM0_Config[CONFIG_DMA_0]},
        .buffersObject =
            {
                .rxBufPtr  = (uint8_t *)g_uart_buffers[CONFIG_UART_0].rxBufPtr,
                .txBufPtr  = (uint8_t *)g_uart_buffers[CONFIG_UART_0].txBufPtr,
                .rxBufSize = CONFIG_UART_BUFFER_SIZE,
                .txBufSize = CONFIG_UART_BUFFER_SIZE,
            },
    },
#if CONFIG_UART_COUNT > 1
    {
        .object =
            {
                .supportFxns        = &UARTMSPSupportFxns,
                .buffersSupported   = true,
                .eventsSupported    = false,
                .callbacksSupported = true,
                .dmaSupported       = true,
                .noOfDMAChannels    = 2,
                .rxDmaChannel       = CONFIG_DMA_1,
                .DMA_Handle         = (DMAMSPM0_Handle)&DMAMSPM0_Config[CONFIG_DMA_1],
            },
        .buffersObject =
            {
                .rxBufPtr  = (uint8_t *)g_uart_buffers[CONFIG_UART_1].rxBufPtr,
                .txBufPtr  = (uint8_t *)g_uart_buffers[CONFIG_UART_1].txBufPtr,
                .rxBufSize = CONFIG_UART_BUFFER_SIZE,
                .txBufSize = CONFIG_UART_BUFFER_SIZE,
            },
    },
#endif  // CONFIG_UART_COUNT > 1
};

const UART_Config UART_config[CONFIG_UART_COUNT] = {
    {
        &UARTObject[CONFIG_UART_0],
        &UARTMSPHWAttrs[CONFIG_UART_0],
    },
#if CONFIG_UART_COUNT > 1
    {
        &UARTObject[CONFIG_UART_1],
        &UARTMSPHWAttrs[CONFIG_UART_1],
    },
#endif  // CONFIG_UART_COUNT > 1
};

void UARTMSP_eventCallback(UART_Handle handle, uint32_t event, uint32_t data, void *userArg) {
}