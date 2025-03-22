#include <config/static-config.hh>

#ifdef DILOS_DO_TRACE
#include <osv/mempool.hh>
#include <osv/trace.hh>

TRACEPOINT(trace_pusnow_enqueue, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_dequeue, "%lu to %u", uintptr_t, uint8_t);
TRACEPOINT(trace_pusnow_asyncpf, "%p %p", uintptr_t, void *);
TRACEPOINT(trace_pusnow_yield, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_exit, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_asyncpf_fj, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_exit_fj, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_yield_fj, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_worker_st, "%d:%d", uint8_t, uint8_t);
TRACEPOINT(trace_pusnow_reenqueue, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_newjob, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_current_worker, "%p", void *);
TRACEPOINT(trace_pusnow_resume, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_start, "%lu", uintptr_t);
TRACEPOINT(trace_pusnow_general, "[%d]%lu", int, uintptr_t);

TRACEPOINT(trace_pusnow_alloc_page, "%p", void *);
TRACEPOINT(trace_pusnow_free_page, "%p", void *);
TRACEPOINT(trace_pusnow_fetch, "[%d] %p %p %p %lx (%d)", int, void *, void *,
           void *, size_t, int);
TRACEPOINT(trace_pusnow_push, "[%d] %p %p %p %lx (%d)", int, void *, void *,
           void *, size_t, int);
TRACEPOINT(trace_pusnow_polled, "%p", void *);

TRACEPOINT(trace_pusnow_alloc, "[%lx] %lx", uintptr_t, uintptr_t);
TRACEPOINT(trace_pusnow_free, "[%lx] %lx", uintptr_t, uintptr_t);

namespace dispatcher {

void trace_enqueue(uintptr_t jid) { trace_pusnow_enqueue(jid); }
void trace_dequeue(uintptr_t jid, uint8_t wid) {
    trace_pusnow_dequeue(jid, wid);
}
void trace_asyncpf(uintptr_t jid, void *ctx) { trace_pusnow_asyncpf(jid, ctx); }
void trace_yield(uintptr_t jid) { trace_pusnow_yield(jid); }
void trace_exit(uintptr_t jid) { trace_pusnow_exit(jid); }
void trace_asyncpf_fj(uintptr_t jid) { trace_pusnow_asyncpf_fj(jid); }
void trace_exit_fj(uintptr_t jid) { trace_pusnow_exit_fj(jid); }
void trace_yield_fj(uintptr_t jid) { trace_pusnow_yield_fj(jid); }
void trace_worker_st(uint8_t a, uint8_t b) { trace_pusnow_worker_st(a, b); }
void trace_reenqueue(uintptr_t jid) { trace_pusnow_reenqueue(jid); }
void trace_newjob(uintptr_t jid) { trace_pusnow_newjob(jid); }
void trace_current_worker(void *p) { trace_pusnow_current_worker(p); }
void trace_resume(uintptr_t jid) { trace_pusnow_resume(jid); }
void trace_start(uintptr_t jid) { trace_pusnow_start(jid); }
void trace_general(int id, uintptr_t token) { trace_pusnow_general(id, token); }

void trace_fetch(int qp_id, void *token, void *paddr, void *roffset,
                 size_t size, int ret) {
    trace_pusnow_fetch(qp_id, token, paddr, roffset, size, ret);
}
void trace_push(int qp_id, void *token, void *paddr, void *roffset, size_t size,
                int ret) {
    trace_pusnow_push(qp_id, token, paddr, roffset, size, ret);
}
void trace_polled(void *token) { trace_pusnow_polled(token); }

void *alloc_page_trace() {
    void *p = ::memory::alloc_page();
    trace_pusnow_alloc_page(p);
    return p;
}

void free_page_trace(void *v) {
    trace_pusnow_free_page(v);
    ::memory::free_page(v);
}

void trace_alloc(uintptr_t tag, uintptr_t addr) {
    trace_pusnow_alloc(tag, addr);
}
void trace_free(uintptr_t tag, uintptr_t addr) { trace_pusnow_free(tag, addr); }

}  // namespace dispatcher

#endif