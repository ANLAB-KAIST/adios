#pragma once

#include <atomic>
namespace dispatcher {

class spinlock_t {
    std::atomic<bool> lock_ = {false};

   public:
    inline void lock() {
        for (;;) {
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                break;
            }
            while (lock_.load(std::memory_order_relaxed))
                ;
        }
    }

    inline void unlock() { lock_.store(false, std::memory_order_release); }
};
}  // namespace dispatcher