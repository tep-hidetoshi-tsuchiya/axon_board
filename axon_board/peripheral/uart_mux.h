#ifndef __UART_MUX_H__
#define __UART_MUX_H__

#ifdef SOMA_BOARD

#include <stdint.h>
#include "msp_peripheral_config.h"

#define UART_MUX_CHANNEL_MIN (1U)
#define UART_MUX_CHANNEL_MAX (9U)
#define UART_MUX_CHANNEL_CNT (UART_MUX_CHANNEL_MAX - UART_MUX_CHANNEL_MIN + 1U)

#define UART_MUX_GPIO_PORT  (MUX_PORT)
#define UART_MUX_ENABLE_PIN (MUX_ENABLE_PIN)
#define UART_MUX_BIT_0_PIN  (MUX_S0_PIN)
#define UART_MUX_BIT_1_PIN  (MUX_S1_PIN)
#define UART_MUX_BIT_2_PIN  (MUX_S2_PIN)
#define UART_MUX_BIT_3_PIN  (MUX_S3_PIN)
#define UART_MUX_BIT_SHIFT  (13U)  // MUX_S0_PIN is on bit 13 of GPIOB
#define UART_MUX_PORT_1     (1U)
#define UART_MUX_PORT_2     (2U)
#define UART_MUX_PORT_3     (3U)
#define UART_MUX_PORT_4     (4U)
#define UART_MUX_PORT_5     (5U)
#define UART_MUX_PORT_6     (6U)
#define UART_MUX_PORT_7     (7U)
#define UART_MUX_PORT_8     (8U)
#define UART_MUX_PORT_9     (9U)

static inline void select_uart_mux(uint32_t channel) {
    if (channel < UART_MUX_CHANNEL_MIN || channel > UART_MUX_CHANNEL_MAX) {
        // Handle invalid channel selection
        return;
    }

    // Select the appropriate UART multiplexer channel
    DL_GPIO_writePinsVal(UART_MUX_GPIO_PORT,
                         UART_MUX_BIT_0_PIN | UART_MUX_BIT_1_PIN | UART_MUX_BIT_2_PIN | UART_MUX_BIT_3_PIN,
                         (channel << UART_MUX_BIT_SHIFT) &
                             (UART_MUX_BIT_0_PIN | UART_MUX_BIT_1_PIN | UART_MUX_BIT_2_PIN | UART_MUX_BIT_3_PIN));
}

static inline void clear_uart_mux(void) {
    // Disable the UART multiplexer by clearing the enable pin
    DL_GPIO_writePinsVal(UART_MUX_GPIO_PORT,
                         UART_MUX_BIT_0_PIN | UART_MUX_BIT_1_PIN | UART_MUX_BIT_2_PIN | UART_MUX_BIT_3_PIN, 0);
}

static inline void enable_uart_mux(void) {
    // Enable the UART multiplexer by setting the enable pin (low)
    DL_GPIO_writePinsVal(UART_MUX_GPIO_PORT, UART_MUX_ENABLE_PIN, 0);
}

static inline void disable_uart_mux(void) {
    // Disable the UART multiplexer by clearing the enable pin (high)
    // DL_GPIO_writePinsVal(UART_MUX_GPIO_PORT, UART_MUX_ENABLE_PIN, UART_MUX_ENABLE_PIN);
}

#endif  // SOMA_BOARD
#endif  // __UART_MUX_H__