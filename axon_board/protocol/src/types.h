#ifndef __CAPSULETOY_COMMUNICATION_PROTOCOL_TYPES_H__
#define __CAPSULETOY_COMMUNICATION_PROTOCOL_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(USE_MULTITHREADING)
#if defined(FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
typedef SemaphoreHandle_t ct_sem;
#elif defined(USE_WINDOWS_API)
#include <windows.h>
typedef HANDLE ct_sem;
#elif defined(THREADX)
#include "tx_api.h"
typedef TX_SEMAPHORE ct_sem;
#else
#warning "Unsupported multithreading library"
#endif

int ct_sem_init(ct_sem* sem);
int ct_sem_free(ct_sem* sem);
int ct_sem_lock(ct_sem* sem);
int ct_sem_unlock(ct_sem* sem);
#endif  // defined(USE_MULTITHREADING)

enum ResponseCode {
    RESPONSE_OK                  = 0,
    RESPONSE_ERROR_BAD_ARGS      = -1,
    RESPONSE_ERROR_OUT_OF_MEMORY = -2,
    RESPONSE_ERROR_PACKET_ID     = -3,
    RESPONSE_ERROR_PACKET_SIZE   = -4,
    RESPONSE_ERROR_PACKET_CRC    = -5,
};

#ifndef __INLINE
#ifndef __NO_INLINE
#if defined(__GNUC__) || defined(__MINGW32__) || defined(__IAR_SYSTEMS_ICC__)
#define __INLINE inline
#elif defined(_MSC_VER)
#define __INLINE __inline
#elif defined(THREADX)
#define __INLINE _Inline
#else
#define __INLINE
#endif
#else
#define __INLINE
#endif  // NO_INLINE
#endif  // INLINE

#ifndef __PACKED
#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#define __PACKED __attribute__((packed))
#else
#define __PACKED
#endif
#endif  // __PACKED

#if defined(USE_PRINT_DEBUG)
#ifndef LINE_END
#define LINE_END "\n"
#endif  // LINE_END
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(USE_MULTITHREADING)
#if defined(USE_WINDOWS_API)
#define PRINT_DEBUG(fmt, ...)                                        \
    do {                                                             \
        char buffer[256];                                            \
        snprintf(buffer, sizeof(buffer), fmt LINE_END, __VA_ARGS__); \
        OutputDebugStringA(buffer);                                  \
    } while (0)
#elif defined(FREERTOS)
#define PRINT_DEBUG(fmt, ...)                                        \
    do {                                                             \
        char buffer[256];                                            \
        snprintf(buffer, sizeof(buffer), fmt LINE_END, __VA_ARGS__); \
        vPrintString(buffer);                                        \
    } while (0)
#elif defined(THREADX)
#define PRINT_DEBUG(fmt, ...)                                        \
    do {                                                             \
        char buffer[256];                                            \
        snprintf(buffer, sizeof(buffer), fmt LINE_END, __VA_ARGS__); \
        tx_trace_string(buffer);                                     \
    } while (0)
#else
#error "Unsupported multithreading library for debug printing"
#endif
#else  // defined(USE_MULTITHREADING)
#define PRINT_DEBUG(fmt, ...)                                        \
    do {                                                             \
        char buffer[256];                                            \
        snprintf(buffer, sizeof(buffer), fmt LINE_END, __VA_ARGS__); \
        printf("%s", buffer);                                        \
    } while (0)
#endif
#else
#define PRINT_DEBUG(fmt, ...)
#endif  // defined(USE_PRINT_DEBUG)

#ifdef __cplusplus
}
#endif
#endif  // __CAPSULETOY_COMMUNICATION_PROTOCOL_TYPES_H__