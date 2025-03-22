#pragma once
#include <config/static-config.hh>

#ifdef DILOS_DO_TRACE

namespace dispatcher {
void trace_enqueue(uintptr_t jid);
void trace_dequeue(uintptr_t jid, uint8_t wid);
void trace_yield(uintptr_t jid);
void trace_asyncpf(uintptr_t jid, void *ctx);
void trace_exit(uintptr_t jid);
void trace_asyncpf_fj(uintptr_t jid);
void trace_yield_fj(uintptr_t jid);
void trace_exit_fj(uintptr_t jid);
void trace_worker_st(uint8_t a, uint8_t b);
void trace_reenqueue(uintptr_t jid);
void trace_newjob(uintptr_t jid);
void trace_resume(uintptr_t jid);
void trace_start(uintptr_t jid);
void trace_general(int id, uintptr_t token);

void trace_current_worker(void *p);
void trace_fetch(int qp_id, void *token, void *paddr, void *roffset,
                 size_t size, int ret);
void trace_push(int qp_id, void *token, void *paddr, void *roffset, size_t size,
                int ret);
void trace_polled(void *token);

void trace_alloc(uintptr_t tag, uintptr_t addr);
void trace_free(uintptr_t tag, uintptr_t addr);
}  // namespace dispatcher

#define DP_TRACE(NAME, ...) NAME(__VA_ARGS__)

#else

#define DP_TRACE(NAME, ...)

#endif
