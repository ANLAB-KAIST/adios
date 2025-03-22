#pragma once
#include <stddef.h>
#include <stdint.h>

// smaller ucontext_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __ncontext_t {
    /* preserved registers */
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    /* argument registers */
    uint64_t rdi;
    /* other int registers */
    uint64_t rip;
    uint64_t rsp;
    /* mxcsr */
    uint32_t mxcsr;
    /* fpu cw. fregs are caller saved */
    uint16_t fpucw;
    uint16_t __padding;
} __attribute__((packed)) ncontext_t;

void makecontext_nano(ncontext_t *ucp, void *ss_sp, size_t ss_size,
                      void (*func)(void), void *arg);
int setcontext_nano(ncontext_t *ucp);
int getcontext_nano(ncontext_t *ucp);
int swapcontext_nano(ncontext_t *ouctx, ncontext_t *uctx);

#ifdef __cplusplus
}
#endif
