#ifndef __DIPSW_UTILS_H__
#define __DIPSW_UTILS_H__

#include "msp_peripheral_config.h"
#include "peripheral_typedef.h"

/**
 * @brief DIPスイッチの現在の状態を取得する関数
 *
 * DIPSW_1_PIN～DIPSW_4_PINに対応するGPIOピンの状態を読み取り、
 * dipsw_t構造体に格納して返します。
 *
 * @return dipsw_t DIPスイッチの各ビットの状態を格納した構造体
 */
static inline dipsw_t get_dipsw_state(void) {
    dipsw_t  dipsw_state;
    uint32_t dipsw_value = DL_GPIO_readPins(DIPSW_PORT, (DIPSW_1_PIN | DIPSW_2_PIN | DIPSW_3_PIN | DIPSW_4_PIN));

    dipsw_state.dipsw_1 = (dipsw_value & DIPSW_1_PIN) ? 1 : 0;
    dipsw_state.dipsw_2 = (dipsw_value & DIPSW_2_PIN) ? 1 : 0;
    dipsw_state.dipsw_3 = (dipsw_value & DIPSW_3_PIN) ? 1 : 0;
    dipsw_state.dipsw_4 = (dipsw_value & DIPSW_4_PIN) ? 1 : 0;

    return dipsw_state;
}

#endif  // __DIPSW_UTILS_H__