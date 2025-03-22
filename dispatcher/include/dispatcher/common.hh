#pragma once

#include <dispatcher/cycles.h>
#include <dispatcher/likely.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <sys/mman.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using worker_id_t = uint8_t;

extern "C" void debug_early(const char *msg);
extern "C" void debug_early_u64(const char *msg, unsigned long long val);