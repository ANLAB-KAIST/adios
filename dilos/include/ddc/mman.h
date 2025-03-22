#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/mman.h>
#include <stdint.h>
#include <sys/mman.h>

#ifdef DISABLE_MAP_DDC
#define MAP_DDC 0
#else
#define MAP_DDC 0x400000
#endif

#define MAP_DDC_LOCAL 0x800000
#define MAP_FIX_FIRST_TWO 0x1000000
#define MAP_DDC_PHY 0x2000000

#define MAP_ALIGNED(n) ((n) << MAP_ALIGNMENT_SHIFT)
#define MAP_ALIGNMENT_SHIFT 28

#define ADDR_SG (0x400000000000ULL)

#define MADV_DDC_CLEAN 0x100
#define MADV_DDC_PAGEOUT 0x101
#define MADV_DDC_PAGEOUT_SG 0x102
#define MADV_DDC_PRINT_STAT 0x103

#define MADV_DDC_MASK (0x10000 - 1)

#ifdef __cplusplus
}
#endif