#ifndef __MSPM0_SYSTICK_H__
#define __MSPM0_SYSTICK_H__

#include <stdint.h>

#ifndef systick_t
typedef volatile uint32_t systick_t;
#endif  // systick_t

systick_t get_systick_count_ms(void);

#endif  // __MSPM0_SYSTICK_H__
