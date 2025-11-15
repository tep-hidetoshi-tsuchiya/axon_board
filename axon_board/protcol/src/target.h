#ifndef __TARGET_H__
#define __TARGET_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t    id;
    const char* name;
} Target;

Target* create_target(uint32_t id, const char* name);

#ifdef __cplusplus
}
#endif
#endif  // __TARGET_H__
