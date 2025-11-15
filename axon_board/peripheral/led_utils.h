#ifndef __MSP_LED_UTILS_H__
#define __MSP_LED_UTILS_H__

#include "msp_peripheral_config.h"

// ==========================================
// LED フェード処理定義
// ==========================================

// フェード方向定義
#define FADE_DIR_IN  1
#define FADE_DIR_OUT 0

// ==========================================
// RGB色フェードシーケンス定義
// ==========================================

/**
 * @brief RGB色定義構造体
 */
typedef struct {
    uint16_t r;         // 赤色PWM値
    uint16_t g;         // 緑色PWM値
    uint16_t b;         // 青色PWM値
} rgb_color_t;

/**
 * @brief プリセットRGB色定義（8色）
 * 
 * この配列は標準的な8色のRGB値を定義します。
 */
static const rgb_color_t RGB_COLOR[] = {
    {   0,   0,   0},  // 0: Off
    { 250,   0,   0},  // 1: Red
    {   0, 250,   0},  // 2: Green
    {   0,   0, 250},  // 3: Blue
    {   0, 250, 250},  // 4: Cyan
    { 150,   0, 150},  // 5: Magenta
    { 250, 250,   0},  // 6: Yellow
    { 250, 250, 250}   // 7: White
};

/**
 * @brief RGB_COLORの要素数
 */
#define RGB_COLOR_COUNT (sizeof(RGB_COLOR) / sizeof(RGB_COLOR[0]))

/**
 * @brief RGB色フェードイン・フェードアウト処理用構造体（シンプル版）
 */
typedef struct {
    uint16_t r_duty;           // 赤色のPWM値（0～250）
    uint16_t g_duty;           // 緑色のPWM値（0～250）
    uint16_t b_duty;           // 青色のPWM値（0～250）
    uint16_t r_target;         // 赤色の目標値
    uint16_t g_target;         // 緑色の目標値
    uint16_t b_target;         // 青色の目標値
    uint8_t r_dir;             // 赤色の方向 (1=UP, 0=DOWN)
    uint8_t g_dir;             // 緑色の方向 (1=UP, 0=DOWN)
    uint8_t b_dir;             // 青色の方向 (1=UP, 0=DOWN)
    uint8_t fade_complete;     // フェード完了フラグ (1=完了, 0=実行中)
    uint16_t step_interval_ms; // PWM更新間隔（1ms or 2ms）
    systick_t last_update;     // 最終更新時刻（SysTick）
} rgb_fade_state_t;

/**
 * @brief 赤色LEDのPWM値を設定します。
 * @param value PWM値 (0～TIM_LED_PWM_PERIOD_COUNT)
 *
 * ボード種別に応じて適切なタイマインスタンスとチャネルを選択します。
 *
 * デューティ比は value / TIM_LED_PWM_PERIOD_COUNT で決まります。
 */
static inline void set_led_r_pwm(uint16_t value) {
    if (value > TIM_LED_PWM_PERIOD_COUNT) {
        value = TIM_LED_PWM_PERIOD_COUNT;
    }

    DL_TimerA_setCaptureCompareValue(LED_RG_TIM_INST, value, DL_TIMER_CC_0_INDEX);
}

/**
 * @brief 緑色LEDのPWM値を設定します。
 * @param value PWM値 (0～TIM_LED_PWM_PERIOD_COUNT)
 *
 * ボード種別に応じて適切なタイマインスタンスとチャネルを選択します。
 *
 * デューティ比は value / TIM_LED_PWM_PERIOD_COUNT で決まります。
 */
static inline void set_led_g_pwm(uint16_t value) {
    if (value > TIM_LED_PWM_PERIOD_COUNT) {
        value = TIM_LED_PWM_PERIOD_COUNT;
    }

    DL_TimerA_setCaptureCompareValue(LED_RG_TIM_INST, value, DL_TIMER_CC_1_INDEX);
}

/**
 * @brief 青色LEDのPWM値を設定します。
 * @param value PWM値 (0～TIM_LED_PWM_PERIOD_COUNT)
 *
 * ボード種別に応じて適切なタイマインスタンスとチャネルを選択します。
 *
 * デューティ比は value / TIM_LED_PWM_PERIOD_COUNT で決まります。
 */
static inline void set_led_b_pwm(uint16_t value) {
    if (value > TIM_LED_PWM_PERIOD_COUNT) {
        value = TIM_LED_PWM_PERIOD_COUNT;
    }

    DL_TimerG_setCaptureCompareValue(LED_B_TIM_INST, value, DL_TIMER_CC_0_INDEX);
}

/**
 * @brief RGB各色のLED PWM値をまとめて設定します。
 * @param r 赤色LEDのPWM値
 * @param g 緑色LEDのPWM値
 * @param b 青色LEDのPWM値
 *
 * 各色のPWM設定関数を順に呼び出します。
 *
 * 各色ともデューティ比は設定値 / TIM_LED_PWM_PERIOD_COUNT で決まります。
 */
static inline void set_led_rgb_pwm(uint16_t r, uint16_t g, uint16_t b) {
    set_led_r_pwm(r);
    set_led_g_pwm(g);
    set_led_b_pwm(b);
}

/**
 * @brief 赤色LEDをGPIOモードでOFF（完全消灯）します。
 */
static inline void turn_off_led_r_gpio(void) {
    // PWM機能を無効化してGPIOモードに切り替えて消灯
    DL_GPIO_initDigitalOutput(LED_R_IOMUX);
    DL_GPIO_setPins(LED_RGB_PORT, LED_R_PIN);
}

/**
 * @brief 緑色LEDをGPIOモードでOFF（完全消灯）します。
 */
static inline void turn_off_led_g_gpio(void) {
    // PWM機能を無効化してGPIOモードに切り替えて消灯
    DL_GPIO_initDigitalOutput(LED_G_IOMUX);
    DL_GPIO_setPins(LED_RGB_PORT, LED_G_PIN);
}

/**
 * @brief 青色LEDをGPIOモードでOFF（完全消灯）します。
 */
static inline void turn_off_led_b_gpio(void) {
    // PWM機能を無効化してGPIOモードに切り替えて消灯
    DL_GPIO_initDigitalOutput(LED_B_IOMUX);
    DL_GPIO_setPins(LED_RGB_PORT, LED_B_PIN);
}

/**
 * @brief RGB全色をGPIOモードでOFF（完全消灯）します。
 * 
 * PWM値を0にしても完全に消灯しない場合に使用します。
 * 各色のピン機能をPWMからGPIOに切り替え、HIGHレベルを出力します。
 * （アノードコモンLEDのため、HIGHで消灯）
 */
static inline void turn_off_led_rgb_gpio(void) {
    turn_off_led_r_gpio();
    turn_off_led_g_gpio();
    turn_off_led_b_gpio();
}

/**
 * @brief 赤色LEDをPWMモードに復帰させます。
 * 
 * turn_off_led_r_gpio()でGPIOモードに切り替えた後、
 * 再びPWM制御を行いたい場合に使用します。
 */
static inline void restore_led_r_pwm(void) {
    DL_GPIO_initPeripheralOutputFunction(LED_R_IOMUX, LED_R_PWM);
}

/**
 * @brief 緑色LEDをPWMモードに復帰させます。
 * 
 * turn_off_led_g_gpio()でGPIOモードに切り替えた後、
 * 再びPWM制御を行いたい場合に使用します。
 */
static inline void restore_led_g_pwm(void) {
    DL_GPIO_initPeripheralOutputFunction(LED_G_IOMUX, LED_G_PWM);
}

/**
 * @brief 青色LEDをPWMモードに復帰させます。
 * 
 * turn_off_led_b_gpio()でGPIOモードに切り替えた後、
 * 再びPWM制御を行いたい場合に使用します。
 */
static inline void restore_led_b_pwm(void) {
    DL_GPIO_initPeripheralOutputFunction(LED_B_IOMUX, LED_B_PWM);
}

/**
 * @brief RGB全色をPWMモードに復帰させます。
 * 
 * turn_off_led_rgb_gpio()でGPIOモードに切り替えた後、
 * 再びPWM制御を行いたい場合に使用します。
 */
static inline void restore_led_rgb_pwm(void) {
    restore_led_r_pwm();
    restore_led_g_pwm();
    restore_led_b_pwm();
}

/**
 * @brief RGB色フェード状態を初期化
 * @param state フェード状態構造体
 * @param r 赤色の目標値
 * @param g 緑色の目標値
 * @param b 青色の目標値
 * @param is_fast_blink true: 高速(1ms間隔), false: 低速(2ms間隔)
 */
static inline void rgb_fade_init(rgb_fade_state_t* state, uint16_t r, uint16_t g, uint16_t b, bool is_fast_blink) {
    state->r_duty = 0;
    state->g_duty = 0;
    state->b_duty = 0;
    state->r_target = r;
    state->g_target = g;
    state->b_target = b;
    state->r_dir = FADE_DIR_IN;
    state->g_dir = FADE_DIR_IN;
    state->b_dir = FADE_DIR_IN;
    state->fade_complete = 0;
    
    // 高速: 1ms間隔、低速: 2ms間隔
    // state->step_interval_ms = is_fast_blink ? 1 : 2;
    state->step_interval_ms = is_fast_blink ? 1 : 4;
    
    // 初期化時の時刻を記録
    state->last_update = get_systick_count_ms();
    
    if (r > 0 || g > 0 || b > 0) {
        restore_led_rgb_pwm();
    }
}

/**
 * @brief RGB色フェードステップ更新
 * @param state フェード状態構造体
 * @param current_time 現在時刻（SysTick）
 * @return true: PWM更新あり, false: 更新なし（まだタイミングではない）
 */
static inline bool rgb_fade_step(rgb_fade_state_t* state, systick_t current_time) {
    // 更新間隔チェック
    if ((current_time - state->last_update) < state->step_interval_ms) {
        return false;  // まだ更新タイミングではない
    }
    
    // 最終更新時刻を更新
    state->last_update = current_time;
    
    // PWM値を±1更新
    bool updated = false;
    
    // 赤色の更新
    if (state->r_dir == FADE_DIR_IN && state->r_duty < state->r_target) {
        state->r_duty++;
        updated = true;
    } else if (state->r_dir == FADE_DIR_OUT && state->r_duty > 5) {
        state->r_duty--;
        updated = true;
    }
    
    // 緑色の更新
    if (state->g_dir == FADE_DIR_IN && state->g_duty < state->g_target) {
        state->g_duty++;
        updated = true;
    } else if (state->g_dir == FADE_DIR_OUT && state->g_duty > 5) {
        state->g_duty--;
        updated = true;
    }
    
    // 青色の更新
    if (state->b_dir == FADE_DIR_IN && state->b_duty < state->b_target) {
        state->b_duty++;
        updated = true;
    } else if (state->b_dir == FADE_DIR_OUT && state->b_duty > 5) {
        state->b_duty--;
        updated = true;
    }
    
    // PWM値を設定
    if (updated) {
        set_led_rgb_pwm(state->r_duty, state->g_duty, state->b_duty);
    }
    
    // フェードイン完了チェック → フェードアウトへ移行
    if (state->r_dir == FADE_DIR_IN && 
        state->r_duty >= state->r_target &&
        state->g_duty >= state->g_target &&
        state->b_duty >= state->b_target) {
        state->r_dir = FADE_DIR_OUT;
        state->g_dir = FADE_DIR_OUT;
        state->b_dir = FADE_DIR_OUT;
    }
    
    // フェードアウト完了チェック
    if (state->r_dir == FADE_DIR_OUT && 
        state->r_duty == 5 && state->g_duty == 5 && state->b_duty == 5) {
        state->fade_complete = 1;
        turn_off_led_rgb_gpio();
    }
    
    return updated;
}

// ==========================================
// RGB色フェード管理（状態保持）
// ==========================================

/**
 * @brief RGB色フェード管理用の静的変数
 */
static rgb_fade_state_t g_rgb_fade_state;

/**
 * @brief RGB色フェード処理を更新（メインループから呼び出し）
 * @param color_index 色インデックス（0～RGB_COLOR_COUNT-1）
 *                    0: Off, 1: Red, 2: Green, 3: Blue, 4: Cyan, 5: Magenta, 6: Yellow, 7: White
 * @param is_fast_blink true: 高速点滅 (250ms), false: 低速点滅 (500ms)
 * @param current_time 現在時刻（SysTick）
 * 
 * メインループから高頻度に呼び出してください。
 * 内部で時間管理しているので、更新タイミング以外は即座にreturnします。
 */
static inline void rgb_led_fade_update(uint8_t color_index, bool is_fast_blink, systick_t current_time) {
    // インデックス範囲チェック
    if (color_index >= RGB_COLOR_COUNT) {
        color_index = 0;
    }
    
    const rgb_color_t* target_color = &RGB_COLOR[color_index];
    
    // フェード完了時は再初期化
    if (g_rgb_fade_state.fade_complete) {
        rgb_fade_init(&g_rgb_fade_state, 
                      target_color->r, 
                      target_color->g, 
                      target_color->b,
                      is_fast_blink);
    }
    
    // フェードステップ更新
    rgb_fade_step(&g_rgb_fade_state, current_time);
}

#endif /* __MSP_LED_UTILS_H__ */