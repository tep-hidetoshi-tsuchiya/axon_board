#include "target.h"

static Target _target;

Target* create_target(uint32_t id, const char* name) {
    _target.id   = id;
    _target.name = name;
    return &_target;
}
