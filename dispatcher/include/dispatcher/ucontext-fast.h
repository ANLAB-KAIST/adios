#pragma once

#include <ucontext.h>

#ifdef	__cplusplus
extern "C" {
#endif

void makecontext_fast(ucontext_t *ucp, void (*func)(void), int argc,...);
int setcontext_fast(ucontext_t *ucp);
int setcontext_fast_no_rs_fp(ucontext_t *ucp);
int getcontext_fast(ucontext_t *ucp);

int swapcontext_fast(ucontext_t *ouctx, ucontext_t *uctx);
int swapcontext_fast_no_rs_fp(ucontext_t *ouctx, ucontext_t *uctx); // do not restore fp context
int swapcontext_fast_no_sv_fp(ucontext_t *ouctx, ucontext_t *uctx); // do not save fp context
int swapcontext_very_fast(ucontext_t *ouctx, ucontext_t *uctx); // do not save & restore fp context



#ifdef	__cplusplus
}
#endif
