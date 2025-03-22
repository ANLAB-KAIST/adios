#pragma once

#define gcc_likely(x) __builtin_expect(!!(x), 1)
#define gcc_unlikely(x) __builtin_expect(!!(x), 0)
