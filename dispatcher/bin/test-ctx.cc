#include <dispatcher/nanocontext.h>
#include <dispatcher/ucontext-fast.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include <cstddef>
#include <dispatcher/common.hh>
static ncontext_t nctx_main, nctx_nano;
static ucontext_t uctx_main, uctx_micro;

uint64_t before, after;
#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

static void fcall(void) { after = get_cycles(); }
static void nano(void) {
    after = get_cycles();
    setcontext_nano(&nctx_main);
}

static void micro(void) {
    after = get_cycles();
    setcontext_fast(&uctx_main);
}
static void microvf(void) {
    after = get_cycles();
    setcontext_fast_no_rs_fp(&uctx_main);
}

int nmain() {
    char nano_stack[16384];

    int total = 1000000;
    uint64_t sum = 0;

    for (int i = 0; i < total; ++i) {
        if (getcontext_nano(&nctx_nano) == -1) handle_error("getcontext_nano");
        makecontext_nano(&nctx_nano, nano_stack, sizeof(nano_stack), nano, 0);
        before = get_cycles();
        if (swapcontext_nano(&nctx_main, &nctx_nano) == -1)
            handle_error("swapcontext_nano");
        sum += after - before;
    }

    printf("ncontext: exiting, avg :%lu\n", sum / total);
    return 0;
}

int umain() {
    char micro_stack[16384];

    int total = 1000000;
    uint64_t sum = 0;

    for (int i = 0; i < total; ++i) {
        if (getcontext_fast(&uctx_micro) == -1) handle_error("getcontext_fast");
        uctx_micro.uc_stack.ss_sp = micro_stack;
        uctx_micro.uc_stack.ss_size = sizeof(micro_stack);
        makecontext_fast(&uctx_micro, micro, 0);
        before = get_cycles();
        if (swapcontext_fast(&uctx_main, &uctx_micro) == -1)
            handle_error("swapcontext_fast");
        sum += after - before;
    }

    printf("ucontext: exiting, avg :%lu\n", sum / total);
    return 0;
}

int umainvf() {
    char micro_stack[16384];

    int total = 1000000;
    uint64_t sum = 0;

    for (int i = 0; i < total; ++i) {
        if (getcontext_fast(&uctx_micro) == -1) handle_error("getcontext_fast");
        uctx_micro.uc_stack.ss_sp = micro_stack;
        uctx_micro.uc_stack.ss_size = sizeof(micro_stack);
        makecontext_fast(&uctx_micro, microvf, 0);
        before = get_cycles();
        if (swapcontext_very_fast(&uctx_main, &uctx_micro) == -1)
            handle_error("swapcontext_fast");
        sum += after - before;
    }

    printf("ucontext vf: exiting, avg :%lu\n", sum / total);
    return 0;
}

int fmain() {
    int total = 1000000;
    uint64_t sum = 0;

    for (int i = 0; i < total; ++i) {
        before = get_cycles();
        fcall();
        sum += after - before;
    }

    printf("fcall: exiting, avg :%lu\n", sum / total);
    return 0;
}

int main() {
    printf("sizeof(ncontext_t): %ld\n", sizeof(ncontext_t));
    printf("sizeof(ucontext_t): %ld\n", sizeof(ucontext_t));
    fmain();
    nmain();
    umain();
    umainvf();
    fmain();
    nmain();
    umain();
    umainvf();
    fmain();
    nmain();
    umain();
    umainvf();

    return 0;
}