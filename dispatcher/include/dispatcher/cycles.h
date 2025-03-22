#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
static inline uint64_t get_cycles() {
    unsigned low, high;
    unsigned long long val;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    val = high;
    val = (val << 32) | low;
    return val;
}
#ifdef __cplusplus
}
#endif