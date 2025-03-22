#pragma once

#include <dispatcher/nanocontext.h>
#include <dispatcher/ucontext-fast.h>

#include <config/static-config.hh>
#include <dispatcher/common.hh>
#include <dispatcher/prefetch.hh>
#include <protocol/net.hh>

namespace dispatcher {
#ifdef DILOS_SWAPCONTEXT_NANO
using context_inner_t = ncontext_t;
#else
using context_inner_t = ucontext_t;
#endif

struct context_t {
    inline context_inner_t *uctx() { return &_uctx; }

    static constexpr size_t guard_num = 4;
    inline void set_guard() {
        for (size_t i = 0; i < guard_num; ++i) {
            _guard[i] = 0x12345;
        }
    }
    inline bool check_guard(int id = 0) {
        if (!this) {
            abort();
            return false;
        }
        for (size_t i = 0; i < guard_num; ++i) {
            if (_guard[i] != 0x12345) {
                debug_early_u64("guard id: ", (uint64_t)id);
                abort();
                return false;
            }
        }
        return true;
    }

    inline void *stack() {
        return (void *)((uint64_t)this + sizeof(context_t));
    }

    constexpr static size_t stack_size() {
        return ETH_BUFFER_SIZE - NET_MTU_ASSUME - sizeof(context_t);
    }

    int check_so(int id = 0);

    inline bool check_ptr_in_stk(void *ptr) {
        if ((uintptr_t)ptr < (uintptr_t)stack()) {
            return false;
        }
        if ((uintptr_t)ptr >= (uintptr_t)this + stack_size()) {
            return false;
        }
        return true;
    }

   private:
    context_inner_t _uctx;

   public:
    void *cls;

   private:
    uint64_t _guard[guard_num];
};

static_assert(sizeof(context_t) % 8 == 0);

static_assert(context_t::stack_size() % 8 == 0);
static_assert(context_t::stack_size() > 0);
static_assert(context_t::stack_size() <= ETH_BUFFER_SIZE);
}  // namespace dispatcher