#ifndef __PERIPHERAL_TYPEDEF_H__
#define __PERIPHERAL_TYPEDEF_H__

#include <stdint.h>

/**
 * @brief ハードウェアバージョン情報を格納するビットフィールド構造体
 *
 * version_h: バージョンの上位ビット
 * version_m: バージョンの中位ビット
 * version_l: バージョンの下位ビット
 * reserved : 予約ビット（未使用、将来拡張用）
 */
typedef struct {
    uint8_t version_h : 1;
    uint8_t version_m : 1;
    uint8_t version_l : 1;
    uint8_t reserved : 5;  // Reserved bits
} hw_version_t;

/**
 * @brief DIPスイッチの状態を格納するビットフィールド構造体
 *
 * dipsw_1: DIPスイッチ1の状態
 * dipsw_2: DIPスイッチ2の状態
 * dipsw_3: DIPスイッチ3の状態
 * dipsw_4: DIPスイッチ4の状態
 * reserved: 予約ビット（未使用、将来拡張用）
 */
typedef struct {
    uint8_t dipsw_1 : 1;
    uint8_t dipsw_2 : 1;
    uint8_t dipsw_3 : 1;
    uint8_t dipsw_4 : 1;
    uint8_t reserved : 4;  // Reserved bits
} dipsw_t;

#endif  // __PERIPHERAL_TYPEDEF_H__