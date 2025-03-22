#pragma once

#include <dispatcher/common.hh>
#include <dispatcher/logger.hh>

namespace dispatcher {

// global memory management
// 1. manage RDMA/raw_eth mrs
// 2. and their keys
class mm_multi_t : public logger_t {
   public:
    static constexpr size_t MAX_MR = 16;
    struct mr_t {
        uintptr_t start;
        size_t end;
        uint32_t eth_lkey;
        uint32_t ib_lkey;
        struct ibv_mr *eth_mr;
        struct ibv_mr *ib_mr;
    };
    struct init_t {
        std::vector<std::pair<void *, size_t>> addrs;
    };

    int init(const init_t &config, struct ibv_pd *eth_pd, struct ibv_pd *ib_pd);
    int reg_mr(struct ibv_pd *eth_pd, struct ibv_pd *ib_pd, void *ptr,
               size_t len);

    inline struct mr_t &find_mr(void *paddr, size_t size) {
        auto it = mrs.begin();
        while (it->start != 0) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(paddr);
            if (addr >= it->start && addr + size <= it->end) {
                return *it;
            }
            ++it;
        }
        PRINT_LOG("unable to find mr: %p size: %ld\n", paddr, size);
        abort();
    }

   private:
    std::array<struct mr_t, MAX_MR> mrs;
};

class mm_single_t : public logger_t {
   public:
    struct mr_t {
        uintptr_t start;
        size_t end;
        uint32_t eth_lkey;
        uint32_t ib_lkey;
        struct ibv_mr *eth_mr;
        struct ibv_mr *ib_mr;
    };
    struct init_t {
        std::vector<std::pair<void *, size_t>> addrs;
    };

    int init(const init_t &config, struct ibv_pd *eth_pd, struct ibv_pd *ib_pd);
    int reg_mr(struct ibv_pd *eth_pd, struct ibv_pd *ib_pd, void *ptr,
               size_t len);

    inline struct mr_t &find_mr(void *paddr, size_t size) { return mr; }

   private:
    struct mr_t mr;
};

using mm_t = mm_single_t;
// using mm_t = mm_multi_t;
}  // namespace dispatcher