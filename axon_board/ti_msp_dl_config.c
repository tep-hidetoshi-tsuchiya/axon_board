/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_AES_backupConfig  gAESBackup;
DL_TRNG_backupConfig gTRNGBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void) {
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_AES_init();
    SYSCFG_DL_CRC_init();
    SYSCFG_DL_TRNG_init();
    SYSCFG_DL_SYSTICK_init();
    /* Ensure backup structures have no valid state */
    gAESBackup.backupRdy  = false;
    gTRNGBackup.backupRdy = false;
}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void) {
    bool retStatus = true;

    retStatus &= DL_AES_saveConfiguration(AES, &gAESBackup);
    retStatus &= DL_TRNG_saveConfiguration(TRNG, &gTRNGBackup);

    return retStatus;
}

SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void) {
    bool retStatus = true;

    retStatus &= DL_AES_restoreConfiguration(AES, &gAESBackup);
    retStatus &= DL_TRNG_restoreConfiguration(TRNG, &gTRNGBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void) {
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_AES_reset(AES);
    DL_CRC_reset(CRC);
    DL_TRNG_reset(TRNG);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_AES_enablePower(AES);
    DL_CRC_enablePower(CRC);
    DL_TRNG_enablePower(TRNG);

    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void) {
}

#ifdef MSPM0G_CPU_FREQ_80MHZ
static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {.inputFreq   = DL_SYSCTL_SYSPLL_INPUT_FREQ_16_32_MHZ,
                                                     .rDivClk2x   = 1,
                                                     .rDivClk1    = 0,
                                                     .rDivClk0    = 0,
                                                     .enableCLK2x = DL_SYSCTL_SYSPLL_CLK2X_DISABLE,
                                                     .enableCLK1  = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
                                                     .enableCLK0  = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
                                                     .sysPLLMCLK  = DL_SYSCTL_SYSPLL_MCLK_CLK0,
                                                     .sysPLLRef   = DL_SYSCTL_SYSPLL_REF_SYSOSC,
                                                     .qDiv        = 9,
                                                     .pDiv        = DL_SYSCTL_SYSPLL_PDIV_2};
#endif

SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void) {
#ifdef MSPM0G_CPU_FREQ_80MHZ
    // Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&gSYSPLLConfig);
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
#else
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
#endif
}

SYSCONFIG_WEAK void SYSCFG_DL_AES_init(void) {
    DL_AES_init(AES, DL_AES_MODE_ENCRYPT_ECB_MODE, DL_AES_KEY_LENGTH_128);
}

SYSCONFIG_WEAK void SYSCFG_DL_CRC_init(void) {
    DL_CRC_init(CRC, DL_CRC_16_POLYNOMIAL, DL_CRC_BIT_NOT_REVERSED, DL_CRC_INPUT_ENDIANESS_LITTLE_ENDIAN,
                DL_CRC_OUTPUT_BYTESWAP_DISABLED);

    DL_CRC_setSeed16(CRC, CRC_SEED);
}

SYSCONFIG_WEAK void SYSCFG_DL_TRNG_init(void) {
    DL_TRNG_setClockDivider(TRNG, TRNG_CLOCK_DIVIDER);

    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
    while (!DL_TRNG_isCommandDone(TRNG));
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

    DL_TRNG_setDecimationRate(TRNG, TRNG_DECIMATION_RATE);
}

SYSCONFIG_WEAK void SYSCFG_DL_SYSTICK_init(void) {
    /* Initialize the period to 1.00 ms */
    DL_SYSTICK_init(SYSTICK_PERIOD);
}
