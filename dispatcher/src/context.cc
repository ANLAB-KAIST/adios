#include <dispatcher/ucontext-fast.h>

#include <dispatcher/context.hh>

extern "C" void debug_early(const char *msg);
extern "C" void debug_early_u64(const char *msg, unsigned long long val);
namespace dispatcher {

int context_t::check_so(int id) {
    int sp = 0;
    if (!this) {
        abort();
        return false;
    }

    if ((uintptr_t)&sp < (uintptr_t)stack()) {
        debug_early_u64("id: ", id);
        debug_early_u64("so1: ", (uintptr_t)&sp);
        debug_early_u64("so11: ", (uintptr_t)stack());
        abort();
        return false;
    }
    if ((uintptr_t)&sp > (uintptr_t)this + stack_size()) {
        debug_early_u64("id: ", id);
        debug_early_u64("so2: ", (uintptr_t)&sp);
        debug_early_u64("so22: ", (uintptr_t)this + stack_size());
        abort();
        return false;
    }

#ifdef DILOS_CHECK_GUARD
    if (!check_guard()) {
        debug_early("so0\n");
        abort();
        return false;
    }
#endif
    return true;
}
}  // namespace dispatcher