#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct req_get_t {
    uint64_t klen;  // includes NULL
    char key[];
};

struct resp_get_t {
    uint64_t vlen;  // includes NULL
    char value[];
    // currently only last value
};

#ifdef __cplusplus
}
#endif
