#pragma once

#include <atomic>
#include <config/cmake-config.hh>
#include <cstdlib>

constexpr size_t max_req_per_type = 128;
constexpr size_t max_req_init = max_req_per_type;
constexpr size_t max_req_page = max_req_per_type;
constexpr size_t max_req_subpage = max_req_per_type;
constexpr size_t max_syscall = max_req_per_type;
constexpr size_t max_evict = max_req_per_type;

static_assert(max_req_init <= 128);

constexpr size_t ETH_RECV_SIZE = 4096;
// constexpr size_t ETH_MIN_STACK_SIZE = 4096;
constexpr size_t ETH_MIN_STACK_SIZE = 4096 * 5;

static_assert(ETH_RECV_SIZE % 4096 == 0);
static_assert(ETH_MIN_STACK_SIZE % 4096 == 0);

constexpr size_t ETH_BUFFER_SIZE = ETH_RECV_SIZE + ETH_MIN_STACK_SIZE;
constexpr size_t ETH_BUFFER_BTCH_SIZE = (ETH_BUFFER_SIZE / 8) - 1;
static_assert(ETH_BUFFER_SIZE % 4096 == 0);

constexpr size_t NUM_ETH_RX_DESC = 32 * 1024;
constexpr size_t NUM_ETH_PKT_PAGES = NUM_ETH_RX_DESC * 4;
constexpr size_t PKT_POOL_SIZE_GB =
    ETH_BUFFER_SIZE * NUM_ETH_PKT_PAGES / 1024 / 1024 / 1024;

constexpr size_t MAX_WORKERS = 32;
constexpr size_t MAX_REQS_PER_WORKER = 4;

// constexpr size_t DDC_PHYMEM_ADDITIONAL_SIZE = 0;
constexpr size_t DDC_PHYMEM_ADDITIONAL_SIZE = 1ULL << 30;
// constexpr size_t DDC_PHYMEM_ADDITIONAL_SIZE = 2ULL << 30;
// constexpr size_t DDC_PHYMEM_ADDITIONAL_SIZE = 64ULL << 20;

constexpr std::memory_order atomic_on_store = std::memory_order_release;
constexpr std::memory_order atomic_on_load = std::memory_order_acquire;
// constexpr std::memory_order atomic_on_store = std::memory_order_seq_cst;
// constexpr std::memory_order atomic_on_load = std::memory_order_seq_cst;

// #define DILOS_SWAPCONTEXT_FASTER

#define DILOS_SWAPCONTEXT_NANO

#define DILOS_DO_TRACE

// #define DILOS_ROCKSDB_CHECK_PREEMPT

// #define DILOS_RDMA_PERF

#define DILOS_DROP_OK

// #define DILOS_ND_ETH_SYNC

// #define DILOS_ND_ETH_ASYNC

#if defined(DILOS_ND_ETH_SYNC) && defined(DILOS_ND_ETH_ASYNC)
#error DILOS_ND_ETH_SYNC && DILOS_ND_ETH_ASYNC
#endif

#define DILOS_SYNC_ALLOC

#define PROTOCOL_BREAKDOWN

// #define DILOS_CHECK_GUARD
