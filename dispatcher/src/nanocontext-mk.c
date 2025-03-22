#include <dispatcher/nanocontext.h>

void makecontext_nano(ncontext_t *ncp, void *ss_sp, size_t ss_size,
                      void (*func)(void), void *arg) {
    uint64_t *sp;
    int i;

    /* Generate room on stack for parameter if needed and uc_link. */
    sp = (uint64_t *)((uintptr_t)ss_sp + ss_size);
    /* Align stack and make space for trampoline address. */
    sp = (uint64_t *)((((uintptr_t)sp) & -16L) - 8);

    /* Setup context ncp. */
    /* Address to jump to. */
    ncp->rip = (uintptr_t)func;
    /* Setup rbx.*/
    ncp->rsp = (uintptr_t)sp;

    /* Handle argument */
    ncp->rdi = (uint64_t)arg;
}
