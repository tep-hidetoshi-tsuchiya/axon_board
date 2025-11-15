#include <stdbool.h>
#include <stdint.h>

#include "driver_config.h"
#include "isr.h"
#include "peripheral/fram_utils.h"
#include "event.h"

systick_t     g_systick_count = 0U;
uart_status_t g_uart_status   = {0};

void SysTick_Handler(void) {
    g_systick_count++;
}

systick_t get_systick_count_ms() {
    return g_systick_count;
}

void GROUP0_IRQHandler(void) {
    // do nothing
}

void GROUP1_IRQHandler(void) {
    // TODO: ここは関数でわける
#if defined(SOMA_BOARD)
    systick_t current_time = get_systick_count_ms();
    uint32_t  pin_val      = 0;

    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        uint8_t status = 0;
        case DL_INTERRUPT_GROUP1_IIDX_GPIOA:
            switch (DL_GPIO_getPendingInterrupt(GPIOA)) {
                case UART_IRQ_1_IIDX:
                    // Handle UART IRQ 1
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_1_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART1_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART1_MASK);
                    }
                    break;
                case UART_IRQ_2_IIDX:
                    // Handle UART IRQ 2
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_2_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART2_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART2_MASK);
                    }
                    break;
                case UART_IRQ_3_IIDX:
                    // Handle UART IRQ 3
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_3_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART3_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART3_MASK);
                    }
                    break;
                case UART_IRQ_4_IIDX:
                    // Handle UART IRQ 4
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_4_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART4_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART4_MASK);
                    }
                    break;
                case UART_IRQ_5_IIDX:
                    // Handle UART IRQ 5
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_5_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART5_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART5_MASK);
                    }
                    break;
                case UART_IRQ_6_IIDX:
                    // Handle UART IRQ 6
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_6_PIN);
                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART6_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART6_MASK);
                    }
                    break;
                case UART_IRQ_7_IIDX:
                    // Handle UART IRQ 7
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_7_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART7_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART7_MASK);
                    }
                    break;
                case UART_IRQ_8_IIDX:
                    // Handle UART IRQ 8
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_8_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART8_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART8_MASK);
                    }
                    break;
                case UART_IRQ_9_IIDX:
                    // Handle UART IRQ 9
                    status = DL_GPIO_readPins(UART_IRQ_PORT, UART_IRQ_9_PIN);

                    if (status == 0) {
                        uart_status_clear(&g_uart_status, UART_IRQ_UART9_MASK);
                    } else {
                        uart_status_set(&g_uart_status, UART_IRQ_UART9_MASK);
                    }
                    break;
                default:
                    break;
            }
            break;
        case DL_INTERRUPT_GROUP1_IIDX_GPIOB:
            switch (DL_GPIO_getPendingInterrupt(GPIOB)) {
                case PUSH_SW_IIDX:
                    pin_val                   = DL_GPIO_readPins(PUSH_SW_PORT, PUSH_SW_PIN);
                    g_rotary_event.pressed    = pin_val ? 1U : 0U;  // demo using by switch
                    g_rotary_event.phase_time = current_time;

                    g_button_1_event.pressed    = pin_val ? 1U : 0U;
                    g_button_1_event.phase_time = current_time;
                default:
                    break;
            }
            break;
        default:
            break;
    }
#elif defined(AXON_BOARD)
    systick_t current_time = get_systick_count_ms();
    uint32_t  pin_val      = 0;

    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case DL_INTERRUPT_GROUP1_IIDX_GPIOA:
            switch (DL_GPIO_getPendingInterrupt(GPIOA)) {
                case ESCROW_SW_IIDX:
                    // Handle the escrow switch interrupt
                    g_escrow_event.pressed    = DL_GPIO_readPins(ESCROW_SW_PORT, ESCROW_SW_PIN) ? 0U : 1U;  // pullup
                    g_escrow_event.phase_time = current_time;
                    break;
                case COIN_SENSOR_IIDX:
                    // Handle the coin sensor interrupt
                    g_coindet_event.pressed    = DL_GPIO_readPins(COIN_SENSOR_PORT, COIN_SENSOR_PIN) ? 0U : 1U;  // pullup
                    g_coindet_event.phase_time = current_time;
                    break;
                case CONNECTOR_DET_IIDX:
                    // Handle the connector detect interrupt
                    g_connector_event.pressed    = DL_GPIO_readPins(CONNECTOR_DET_PORT, CONNECTOR_DET_PIN) ? 0U : 1U;  // pullup
                    g_connector_event.phase_time = current_time;
                    break;
                case DIAL_SW_IIDX:
                    // Handle the dial switch interrupt

                    // pullup
                    // 1 = pressed
                    // 0 = not pressed
                    g_rotary_event.pressed    = DL_GPIO_readPins(DIAL_SW_PORT, DIAL_SW_PIN) ? 0U : 1U;
                    g_rotary_event.phase_time = current_time;
                    break;
                case DOOR_OC_DET_IIDX:
                    // Handle the door open/close detect interrupt
                    g_door_event.pressed    = DL_GPIO_readPins(DOOR_OC_DET_PORT, DOOR_OC_DET_PIN) ? 0U : 1U;  // pullup
                    g_door_event.phase_time = current_time;
                    break;
                case SOLDOUT_SW_IIDX:
                    // Handle the sold-out switch interrupt
                    g_soldout_event.pressed    = DL_GPIO_readPins(SOLDOUT_SW_PORT, SOLDOUT_SW_PIN) ? 0U : 1U;  // pullup
                    g_soldout_event.phase_time = current_time;
                    break;
                default:
                    break;
            }
            break;
        case DL_INTERRUPT_GROUP1_IIDX_GPIOB:
            switch (DL_GPIO_getPendingInterrupt(GPIOB)) {
                case PUSH_SW1_IIDX:
                    // Handle the push switch 1 interrupt

                    // for debug
                    // pullup
                    // 1 = pressed
                    // 0 = not pressed
                    // g_rotary_event.pressed    = DL_GPIO_readPins(PUSH_SW_PORT, PUSH_SW1_PIN) ? 1U : 0U;

                    pin_val                   = DL_GPIO_readPins(PUSH_SW_PORT, PUSH_SW1_PIN);
                    // g_rotary_event.pressed    = pin_val ? 1U : 0U;  // demo using by switch
                    // g_rotary_event.phase_time = current_time;

                    g_button_1_event.pressed    = pin_val ? 1U : 0U;
                    g_button_1_event.phase_time = current_time;
                    break;
                case PUSH_SW2_IIDX:
                    // Handle the push switch 2 interrupt
                    pin_val                     = DL_GPIO_readPins(PUSH_SW_PORT, PUSH_SW2_PIN);
                    g_button_2_event.pressed    = pin_val ? 1U : 0U;
                    g_button_2_event.phase_time = current_time;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
#endif  // SOMA_BOARD
}

void UART0_IRQHandler(void) {
    UARTMSP_interruptHandler((UART_Handle)&UART_config[0]);
}

void UART2_IRQHandler(void) {
#if CONFIG_UART_COUNT > 1
    UARTMSP_interruptHandler((UART_Handle)&UART_config[1]);
#endif  // CONFIG_UART_COUNT > 1
}

#ifdef SOMA_BOARD
void FRAM_SPI_IRQ_HANDLER(void)
{
    // Handle SPI Receive interrupt
    switch (DL_SPI_getPendingInterrupt(FRAM_SPI_INST)) {
        case DL_SPI_IIDX_RX:    // データ受信完了割り込み
            /* Read RX FIFO, then increment data to be transmitted */
            gRxData = DL_SPI_receiveData8(FRAM_SPI_INST);
            gSpiRxCompFlg = true;    // 受信完了フラグセット

            break;
        default:
            break;
    }
}
#endif // SOMA_BOARD