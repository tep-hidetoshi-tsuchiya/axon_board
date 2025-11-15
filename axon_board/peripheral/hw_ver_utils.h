#ifndef __HW_VER_UTILS_H__
#define __HW_VER_UTILS_H__

#include "msp_peripheral_config.h"
#include "peripheral_typedef.h"

/**
 * @brief ハードウェアバージョンを取得する関数
 *
 * HW_VER_1_PIN, HW_VER_2_PIN, HW_VER_3_PIN の各ピンの状態を読み取り、
 * hw_version_t 構造体にバージョン情報を格納して返します。
 *
 * @return hw_version_t ハードウェアバージョン情報
 */
static inline hw_version_t get_hw_version(void) {
    hw_version_t hw_ver;
    uint32_t     hw_ver_value = DL_GPIO_readPins(HW_VER_PORT, (HW_VER_1_PIN | HW_VER_2_PIN | HW_VER_3_PIN));

    hw_ver.version_h = (hw_ver_value & HW_VER_1_PIN) ? 1 : 0;
    hw_ver.version_m = (hw_ver_value & HW_VER_2_PIN) ? 1 : 0;
    hw_ver.version_l = (hw_ver_value & HW_VER_3_PIN) ? 1 : 0;

    return hw_ver;
}

#endif  // __HW_VER_UTILS_H__