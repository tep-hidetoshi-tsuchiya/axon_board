#include "main.h"

#include "peripheral/msp_peripheral_config.h"
#include "ti_msp_dl_config.h"

int main(void) {
    SYSCFG_DL_init();

    DL_SYSCTL_disableSleepOnExit();

    DL_FlashCTL_executeClearStatus(FLASHCTL_GEN_OFS);

    NVIC_ClearPendingIRQ(SysTick_IRQn);
    NVIC_ClearPendingIRQ(GPIOA_INT_IRQn);
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);

    NVIC_EnableIRQ(SysTick_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

#ifdef SOMA_BOARD
    soma_routine_main(NULL);
#else  /*SOMA_BOARD*/
    axon_routine_main(NULL);
#endif /*SOMA_BOARD*/
}