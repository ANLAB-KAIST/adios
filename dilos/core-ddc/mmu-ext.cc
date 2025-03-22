#include "../core/mmu.cc"
// MMU

#include <ddc/mman.h>
#include <ddc/prefetch.h>
#include <dlfcn.h>
#include <osv/mutex.h>

#include <cstddef>
#include <ddc/elf.hh>
#include <ddc/memory.hh>
#include <ddc/mmu.hh>
#include <ddc/stat.hh>
#include <osv/irqlock.hh>
#include <osv/preempt-lock.hh>
#include <osv/sched.hh>

#include "apic.hh"

//

#include <boost/circular_buffer.hpp>

#include "init.hh"
#include "prefetcher/general.hh"
#include "pte.hh"
// DDC PT Operations

#include <dispatcher/ctx.h>
#include <x86intrin.h>

#include <config/static-config.hh>
#include <dispatcher/dispatcher.hh>
// #include <osv/migration-lock.hh>

// #define NORETURN_UNTIL_PREFETCH
// preempt_lock2_t preempt_lock2;

STAT_COUNTER(ddc_page_fault);
STAT_COUNTER(ddc_page_fault_remote);
STAT_COUNTER(ddc_page_fault_protected_other);
STAT_AVG(ddc_page_fault_protected_num);
STAT_ELAPSED(ddc_page_fault_remote);
STAT_ELAPSED(ddc_page_fault_remote_real);
STAT_ELAPSED(ddc_page_prefetching);

namespace ddc {

constexpr unsigned ddc_perm = mmu::perm_rw;

constexpr size_t prefetch_window = max_req_page;

TRACEPOINT(trace_ddc_mmu_mmap, "addr=%p, length=%lx, prot=%d, flags=%d", void *,
           size_t, int, int);
TRACEPOINT(trace_ddc_mmu_madvise, "addr=%p, length=%lx, advise=%d", void *,
           size_t, int);
TRACEPOINT(trace_ddc_mmu_mprotect, "addr=%p, length=%lx, prot=%d", void *,
           size_t, int);
TRACEPOINT(trace_ddc_mmu_munmap, "addr=%p, length=%lx", void *, size_t);
TRACEPOINT(trace_ddc_mmu_msync, "addr=%p, length=%lx, flags=%d", void *, size_t,
           int);
TRACEPOINT(trace_ddc_mmu_mincore, "addr=%p, length=%lx", void *, size_t);
TRACEPOINT(trace_ddc_mmu_mlock, "addr=%p, length=%lx", const void *, size_t);
TRACEPOINT(trace_ddc_mmu_munlock, "addr=%p, length=%lx", const void *, size_t);
TRACEPOINT(trace_ddc_mmu_fault, "addr=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_fault_first, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_fault_remote, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_fault_protected, "protector=%d", unsigned);
TRACEPOINT(trace_ddc_mmu_fault_remote_offset, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_fault_remote_init_done, "va=%p bytes=%lx", uintptr_t,
           uint64_t);
TRACEPOINT(trace_ddc_mmu_fault_remote_page_done, "va=%p bytes=%lx", uintptr_t,
           uint64_t);
TRACEPOINT(trace_ddc_mmu_polled_size, "size: %ld", size_t);
TRACEPOINT(trace_ddc_mmu_polled, "tag: %d, offset: %lx", int, uintptr_t);
TRACEPOINT(trace_ddc_mmu_polled_page, "offset: %lx size: %ld", uintptr_t,
           size_t);
TRACEPOINT(trace_ddc_mmu_polled_page_poped, "offset: %lx", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_do_prefetch, "addr=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_first, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_remote, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_remote_offset, "va=%p paddr=%p", uintptr_t,
           void *);
TRACEPOINT(trace_ddc_mmu_prefetch_remote_offset_page, "va=%p paddr=%p",
           uintptr_t, void *);
TRACEPOINT(trace_ddc_mmu_prefetch_remote_page, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_remote_sub_page, "va=%p", uintptr_t);
TRACEPOINT(trace_ddc_mmu_prefetch_remote_result, "result=%d", int);
TRACEPOINT(trace_pusnow_initial_protected, "%p", void *);
TRACEPOINT(trace_pusnow_before_yield, "%p", void *);
TRACEPOINT(trace_pusnow_after_yield, "%p", void *);
TRACEPOINT(trace_pusnow_before_asyncpf, "%p", void *);
TRACEPOINT(trace_pusnow_after_asyncpf, "%p", void *);
TRACEPOINT(trace_pusnow_recvipi, "%d", int);
TRACEPOINT(trace_pusnow_recvipi2, "%p vs %p", void *, void *);
TRACEPOINT(trace_pusnow_recvipi_job, "%lu", uint64_t);
TRACEPOINT(trace_pusnow_pf_general, "%d", int);
TRACEPOINT(trace_pusnow_pf_general2, "%d (%u) %lx", int, unsigned, uintptr_t);
TRACEPOINT(trace_pusnow_rip, "%lx", uintptr_t);

class mock_vma : public vma {
   public:
    mock_vma(addr_range range) : vma(range, 0, 0, false, nullptr) {}
    virtual void fault(uintptr_t addr, exception_frame *ef) override {
        abort();
    }
    virtual void split(uintptr_t edge) override { abort(); }
    virtual error sync(uintptr_t start, uintptr_t end) override { abort(); }
    virtual page_allocator *page_ops() override { abort(); }
};

struct pf_request_sjk_t {
    uintptr_t va;
    int *remain;
    pt_element<base_page_level> pte_prev;
    hw_ptep<base_page_level> ptep;
    void *paddr;
    pf_request_sjk_t(uintptr_t va, int *remain,
                     pt_element<base_page_level> pte_prev,
                     hw_ptep<base_page_level> ptep, void *paddr)
        : va(va),
          remain(remain),
          pte_prev(pte_prev),
          ptep(ptep),
          paddr(paddr) {}

    pf_request_sjk_t &operator=(const pf_request_sjk_t &other) {
        va = other.va;
        remain = other.remain;
        pte_prev = other.pte_prev;
        ptep = other.ptep;
        paddr = other.paddr;
        return *this;
    }
};

struct percpu_ctx_t {
    int pf_mode;
    size_t issued;
    dispatcher::worker_t *worker;
    dispatcher::worker_core_t *worker_core;
    boost::circular_buffer<pf_request_sjk_t> req_sjk;
    uintptr_t tokens_sjk[max_req_init];
    percpu_ctx_t()
        : pf_mode(dispatcher::dispatcher_t::PF_MODE_SYNC),
          issued(0),
          req_sjk(max_req_init) {}
};

static std::array<percpu_ctx_t, max_cpu> percpu_ctx;

enum class fetch_type : uint8_t {
    INIT = 1,
    PAGE = 2,
    SUBPAGE = 3,
};

static inline uintptr_t embed_tag(uintptr_t offset, fetch_type tag) {
    static_assert(sizeof(uintptr_t) == 8);
    static_assert(sizeof(fetch_type) == 1);
    assert(!(offset >> 56));
    return offset | (static_cast<uintptr_t>(tag) << 56);
}
static inline fetch_type get_tag(uintptr_t token) {
    return static_cast<fetch_type>(token >> 56);
}
static inline uintptr_t get_offset(uintptr_t token) {
    return token & 0x00FFFFFFFFFFFFFFULL;
}

/* Event handler */
// static bool do_hit_track = false;
// static int nil_ev_handler(const struct ddc_event_t *event) { return 0; }
// static ddc_handler_t ev_handler = nil_ev_handler;

struct ddc_event_impl_t {
    ddc_event_t inner;
    int *remain;
};
static_assert(offsetof(ddc_event_impl_t, inner) == 0);

/* Event handler End */

static void handle_init(unsigned my_cpu_id, uintptr_t va,
                        pt_element<base_page_level> pte_prev,
                        hw_ptep<base_page_level> ptep, void *paddr) {
    auto after = make_leaf_pte(ptep, mmu::virt_to_phys(paddr), ddc_perm);
    after.set_accessed(true);
    after.set_dirty(false);
    trace_ddc_mmu_fault_remote_init_done(va, *(uint64_t *)paddr);

    bool ret = ptep.compare_exchange(pte_prev, after);
    assert(ret);
    auto &ctx = percpu_ctx[my_cpu_id];
    ctx.issued -= 1;
    ctx.worker_core->page_mgmt_local.add_active_bufferd(paddr, va,
                                                        ptep.release());
}

static int do_polling(unsigned my_cpu_id, int &remain,
                      pt_element<base_page_level> pte_prev,
                      hw_ptep<base_page_level> ptep, void *paddr) {
    assert(remain >= 0);

    auto &ctx = percpu_ctx[my_cpu_id];
    size_t to_poll = ctx.issued;
    uintptr_t tokens[to_poll];
    int polled = 0;

    dispatcher::worker_core_t *worker_core = percpu_ctx[my_cpu_id].worker_core;
    polled = worker_core->poll_rdma(tokens, to_poll);

    for (int i = 0; i < polled; ++i) {
        auto tag = get_tag(tokens[i]);
        auto offset = get_offset(tokens[i]);
        (void)offset;
        switch (tag) {
            case fetch_type::INIT:
                handle_init(my_cpu_id, get_offset(tokens[i]), pte_prev, ptep,
                            paddr);
                --remain;
                break;
            // case fetch_type::PAGE:
            //     handle_page(my_cpu_id, get_offset(tokens[i]));
            //     break;
            // case fetch_type::SUBPAGE:
            //     handle_subpage(my_cpu_id, get_offset(tokens[i]), remain);
            //     --remain;
            //     break;
            default:
                abort();
        }
    }
    return polled;
}
static int do_polling_sjk(unsigned my_cpu_id) {
    assert(!sched::preemptable());
    auto &ctx = percpu_ctx[my_cpu_id];
    size_t to_poll = ctx.issued;

    int polled = 0;
    dispatcher::worker_core_t *worker_core = ctx.worker_core;
    polled = worker_core->poll_rdma(ctx.tokens_sjk, to_poll);

    for (int i = 0; i < polled; ++i) {
        auto tag = get_tag(ctx.tokens_sjk[i]);
        auto offset = get_offset(ctx.tokens_sjk[i]);

        switch (tag) {
            case fetch_type::INIT: {
                auto &req = ctx.req_sjk.front();
                assert(offset == req.va);
                handle_init(my_cpu_id, req.va, req.pte_prev, req.ptep,
                            req.paddr);
                *req.remain = *req.remain - 1;
                ctx.req_sjk.pop_front();
            } break;
            default:
                abort();
        }
    }
    return polled;
}
class first_fault_handling {
   protected:
    bool do_page_small_first(mmu::pt_element<0> pte, mmu::hw_ptep<0> ptep,
                             uintptr_t va, void *paddr) {
        assert(paddr != NULL);
        memset(paddr, 0, mmu::page_size);  // TODO: no zerofill

        auto pte_next = make_leaf_pte(ptep, mmu::virt_to_phys(paddr), ddc_perm);
        pte_next.set_accessed(true);

        unsigned my_cpu_id = sched::cpu::current()->id;
        while (true) {
            if (is_protected(pte)) {
                abort(
                    "UNIMPLEMENTED: wait protected one (fault handling first "
                    "access)");
            } else if (is_remote(pte)) {
                abort("unknwon remote fault");
            } else if (pte.valid()) {
                // This fault is handled by other thread.
                percpu_ctx[my_cpu_id].worker_core->pp_local.free_page(paddr);

                return true;
            }

            if (ptep.compare_exchange(pte, pte_next)) {
                // update success
                break;
            }

            pte = ptep.read();
        }
        percpu_ctx[my_cpu_id].worker_core->page_mgmt_local.add_active_bufferd(
            paddr, va, ptep.release());
        // WITH_LOCK(preempt_lock) { insert_page_buffered(page); }
        return true;
    }
};

class fault_handling
    : public mmu::vma_operation<mmu::allocate_intermediate_opt::yes,
                                mmu::skip_empty_opt::no, mmu::account_opt::no>,
      first_fault_handling {
   public:
    fault_handling(uintptr_t fault_addr) : _fault_addr(fault_addr) {}
    bool page(mmu::hw_ptep<0> ptep, uintptr_t va) {
        auto pte = ptep.read();
        if (pte.valid()) {
            return true;
        }
        bool ret = false;
        unsigned my_cpu_id = sched::cpu::current()->id;

        if (is_first(pte)) {
            assert(sched::preemptable());
            SCOPE_LOCK(preempt_lock);
            trace_ddc_mmu_fault_first(va);
            void *page =
                percpu_ctx[my_cpu_id].worker_core->pp_local.alloc_page();

            ret = do_page_small_first(pte, ptep, va, page);
        } else {
            trace_ddc_mmu_fault_remote(va);

            switch (percpu_ctx[my_cpu_id].pf_mode) {
                case dispatcher::dispatcher_t::PF_MODE_SYNC:
                    ret = do_page_small_remote(pte, ptep, va);
                    break;
                case dispatcher::dispatcher_t::PF_MODE_ASYNC:
                    ret = do_page_small_remote_async(pte, ptep, va);
                    break;
                case dispatcher::dispatcher_t::PF_MODE_ASYNC_LOCAL:
                    ret = do_page_small_remote_async_local(pte, ptep, va);
                    break;
                case dispatcher::dispatcher_t::PF_MODE_HYBRID:
                    ret = do_page_small_remote_hybrid(pte, ptep, va);
                    break;
                case dispatcher::dispatcher_t::PF_MODE_SYNC_SJK:
                    ret = do_page_small_remote_sjk(pte, ptep, va);
                    break;
                case dispatcher::dispatcher_t::PF_MODE_SYNC_SJK2:
                    ret = do_page_small_remote_sjk2(pte, ptep, va);
                    break;
            }
            STAT_ELAPSED_TO(ddc_page_fault_remote, __debug);
        }
        return ret;
    }
    bool page(mmu::hw_ptep<1> ptep, uintptr_t va) {
        abort("not implemented: fault handling do_page_1\n");
        return true;
    }

   private:
    bool do_page_small_remote(mmu::pt_element<0> pte, mmu::hw_ptep<0> ptep,
                              uintptr_t va) {
        SCOPE_LOCK(preempt_lock);
        unsigned my_cpu_id = sched::cpu::current()->id;

        int remain = 0;

        auto protect_pte = protected_pte(ptep, my_cpu_id);  // protect pte
        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
                _mm_pause();

            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }

        dispatcher::worker_core_t *worker_core =
            percpu_ctx[my_cpu_id]
                .worker_core;  // safe due to migration lock or
                               // my_cpu_id is updated after yield

        // 2. alloc page to fetch
        void *paddr = worker_core->pp_local.alloc_page();
        assert(paddr != NULL);

        int ret =
            worker_core->fetch_rdma(dispatcher::worker_core_t::RDMA_SYNC_QP,
                                    embed_tag(va, fetch_type::INIT), paddr,
                                    va_to_offset(va), mmu::page_size);

        assert(!ret);

        auto &ctx = percpu_ctx[my_cpu_id];
        remain += 1;
        ctx.issued += 1;

#ifdef PROTOCOL_BREAKDOWN
        uint64_t polling_start_cycle = get_cycles();
#endif

        while (remain > 0) {
            do_polling(my_cpu_id, remain, protect_pte, ptep, paddr);
        }

#ifdef PROTOCOL_BREAKDOWN
        // maybe okay (at least sync pf)
        uint64_t polling_end_cycle = get_cycles();
        if (percpu_ctx[my_cpu_id].worker) {
            percpu_ctx[my_cpu_id].worker->major_pf += 1;
            percpu_ctx[my_cpu_id].worker->polling_pf +=
                polling_end_cycle - polling_start_cycle;
        }
#endif

        return true;
    }
    bool do_page_small_remote_async(mmu::pt_element<0> pte,
                                    mmu::hw_ptep<0> ptep, uintptr_t va) {
        // DILOS++ Plan
        // 1. Replace data structure for initial pf -> Done
        // 2. Impl new sync PF
        // 3. Impl async PF -> Done
        // 4. Impl new protect
        // 5. Impl prefetch

        // New PTE
        // Protect + Job ->

        unsigned my_cpu_id = sched::cpu::current()->id;
        dispatcher::job_t *job = percpu_ctx[my_cpu_id].worker->job;
        job->next = nullptr;

        auto protect_pte =
            protected_pte_with_addr(ptep,
                                    (uintptr_t)job);  // protect pte

        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
                // it is very hard to link slist concurrently
                // percpu_ctx[my_cpu_id].worker->yield_to_worker();
                adios_ctx_yield();
                // at this point worker can be changed, update my_cpu_id
                my_cpu_id = sched::cpu::current()->id;
            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                // protect PTE with current running job
                // this will prevent multiple pg fetch

                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }

        // 2. alloc page to fetch
        void *paddr = percpu_ctx[my_cpu_id].worker->pp_local.alloc_page();

        assert(paddr != NULL);

        // !IMPORTANT DO NOT ISSUE RDMA IN JOB CONTEXT (DATA RACE)
#ifdef PROTOCOL_BREAKDOWN
        uint64_t polling_start_cycle = get_cycles();
#endif

        percpu_ctx[my_cpu_id].worker->pf_async_to_worker(va_to_offset(va),
                                                         paddr);

        // at this point worker can be changed, update my_cpu_id
        my_cpu_id = sched::cpu::current()->id;

#ifdef PROTOCOL_BREAKDOWN
        // maybe okay (at least sync pf)
        uint64_t polling_end_cycle = get_cycles();
        percpu_ctx[my_cpu_id].worker->major_pf += 1;
        percpu_ctx[my_cpu_id].worker->polling_pf +=
            polling_end_cycle - polling_start_cycle;
#endif
        //  handle mapping
        // todo: refactor this

        auto after = make_leaf_pte(ptep, mmu::virt_to_phys(paddr), ddc_perm);
        after.set_accessed(true);
        after.set_dirty(false);
        bool ret = ptep.compare_exchange(protect_pte, after);
        assert(ret);

        percpu_ctx[my_cpu_id].worker->page_mgmt_local.add_active_bufferd(
            paddr, va, ptep.release());

        return true;
    }

    bool do_page_small_remote_async_local(mmu::pt_element<0> pte,
                                          mmu::hw_ptep<0> ptep, uintptr_t va) {
        unsigned my_cpu_id = sched::cpu::current()->id;
        dispatcher::job_t *job = percpu_ctx[my_cpu_id].worker->job;
        job->next = nullptr;

        auto protect_pte =
            protected_pte_with_addr(ptep,
                                    (uintptr_t)job);  // protect pte

        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
#if 0
                job->next = (dispatcher::job_t *)get_protect_addr(pte);
                // due to concurrency cas should be done in worker
                percpu_ctx[my_cpu_id].worker->wait_to_worker(ptep.release());
                // Handled by other coroutine
                return true;
#else
                // percpu_ctx[my_cpu_id].worker->yield_to_worker();
                adios_ctx_yield();
                my_cpu_id = sched::cpu::current()->id;
#endif
            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                // protect PTE with current running job
                // this will prevent multiple pg fetch

                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }

        // 2. alloc page to fetch
#ifndef DILOS_SYNC_ALLOC

        void *paddr =
            percpu_ctx[my_cpu_id].worker->pp_local.alloc_page_non_polling();

        while (paddr == nullptr) {
            // upcall
            percpu_ctx[my_cpu_id].worker->page_alloc_async_to_worker();
            paddr =
                percpu_ctx[my_cpu_id].worker->pp_local.alloc_page_non_polling();
        }
        assert(paddr != NULL);
#else

        void *paddr = percpu_ctx[my_cpu_id].worker->pp_local.alloc_page();
        assert(paddr != NULL);
#endif

        // !IMPORTANT DO NOT ISSUE RDMA IN JOB CONTEXT (DATA RACE)
#ifdef PROTOCOL_BREAKDOWN
        uint64_t polling_start_cycle = get_cycles();
#endif

        percpu_ctx[my_cpu_id].worker->pf_async_to_worker(va_to_offset(va),
                                                         paddr);

        // at this point worker can be changed, update my_cpu_id
        my_cpu_id = sched::cpu::current()->id;

#ifdef PROTOCOL_BREAKDOWN
        // maybe okay (at least sync pf)
        uint64_t polling_end_cycle = get_cycles();
        percpu_ctx[my_cpu_id].worker->major_pf += 1;
        percpu_ctx[my_cpu_id].worker->polling_pf +=
            polling_end_cycle - polling_start_cycle;
#endif
        //  handle mapping
        // todo: refactor this

        auto after = make_leaf_pte(ptep, mmu::virt_to_phys(paddr), ddc_perm);
        after.set_accessed(true);
        after.set_dirty(false);

        pte = ptep.read();

        // update until mapping
        while (!ptep.compare_exchange(pte, after)) {
            pte = ptep.read();
        }

        // now pte points ready jobs

        dispatcher::job_t *ready_job =
            (dispatcher::job_t *)get_protect_addr(pte);
        while (ready_job != job) {
            assert(ready_job != nullptr);
            dispatcher::job_t *to_push = ready_job;
            ready_job = ready_job->next;
            percpu_ctx[my_cpu_id].worker->ready_local.push_back(to_push);
        }
        percpu_ctx[my_cpu_id].worker->page_mgmt_local.add_active_bufferd(
            paddr, va, ptep.release());

        return true;
    }

    bool do_page_small_remote_hybrid(mmu::pt_element<0> pte,
                                     mmu::hw_ptep<0> ptep, uintptr_t va) {
        // DILOS++ Plan

        unsigned my_cpu_id = sched::cpu::current()->id;
        dispatcher::job_t *job = percpu_ctx[my_cpu_id].worker->job;

        auto protect_pte =
            protected_pte_with_addr(ptep,
                                    (uintptr_t)job);  // protect pte
        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
                // percpu_ctx[my_cpu_id].worker->yield_to_worker();
                adios_ctx_yield();

                // at this point worker can be changed, update my_cpu_id
                my_cpu_id = sched::cpu::current()->id;

            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                // protect PTE with current running job
                // this will prevent multiple pg fetch

                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }

        // 2. alloc page to fetch
        void *paddr = percpu_ctx[my_cpu_id].worker->pp_local.alloc_page();
        assert(paddr != NULL);

        // uintptr_t fault_page_addr = align_down(_fault_addr,
        // mmu::page_size);

        // hybrid mode
        if (dispatcher::default_dispatcher.num_job_pending.load(
                std::memory_order_acquire) > 0) {
            // async mode

#ifdef PROTOCOL_BREAKDOWN
            uint64_t polling_start_cycle = get_cycles();
#endif

            percpu_ctx[my_cpu_id].worker->pf_async_to_worker(va_to_offset(va),
                                                             paddr);
            // at this point worker can be changed, update my_cpu_id
            my_cpu_id = sched::cpu::current()->id;

            //  handle mapping
            // todo: refactor this

            auto after =
                make_leaf_pte(ptep, mmu::virt_to_phys(paddr), ddc_perm);
            after.set_accessed(true);
            after.set_dirty(false);
            bool ret = ptep.compare_exchange(protect_pte, after);
            assert(ret);
            percpu_ctx[my_cpu_id].worker->page_mgmt_local.add_active_bufferd(
                paddr, va, ptep.release());

#ifdef PROTOCOL_BREAKDOWN
            // maybe okay (at least sync pf)
            uint64_t polling_end_cycle = get_cycles();
            percpu_ctx[my_cpu_id].worker->major_pf += 1;
            percpu_ctx[my_cpu_id].worker->polling_pf +=
                polling_end_cycle - polling_start_cycle;
#endif
        } else {
            // sync mode

            int ret = percpu_ctx[my_cpu_id].worker->fetch_rdma(
                dispatcher::worker_core_t::RDMA_SYNC_QP,
                embed_tag(va, fetch_type::INIT), paddr, va_to_offset(va),
                mmu::page_size);
            assert(!ret);
            int remain = 0;
            auto &ctx = percpu_ctx[my_cpu_id];
            remain += 1;
            ctx.issued += 1;

#ifdef DILOS_PREFETCH_PF
            // Inform prefetcher
            ddc_event_impl_t ev;
            ev.inner.type = ddc_event_t::DDC_EVENT_PREFETCH_START;
            ev.inner.fault_addr = _fault_addr;
            ev.inner.start.hits =
                do_hit_track ? perthread_ctx.prefetch.hits(ctx.prefetch_va) : 0;
            ev.inner.start.pages = ctx.prefetch_va;
            ev.remain = &remain;
            int handler_ret = ev_handler(&ev.inner);
            assert(handler_ret == 0);
#endif

#ifdef PROTOCOL_BREAKDOWN
            uint64_t polling_start_cycle = get_cycles();
#endif

            WITH_LOCK(preempt_lock) {
                while (remain > 0) {
                    // DROP_LOCK(preempt_lock) { sched::cpu::schedule(); }
                    do_polling(my_cpu_id, remain, protect_pte, ptep, paddr);
                }
            }

#ifdef PROTOCOL_BREAKDOWN
            // maybe okay (at least sync pf)
            uint64_t polling_end_cycle = get_cycles();
            percpu_ctx[my_cpu_id].worker->major_pf += 1;
            percpu_ctx[my_cpu_id].worker->polling_pf +=
                polling_end_cycle - polling_start_cycle;
#endif
        }

        return true;
    }

    bool do_page_small_remote_sjk(mmu::pt_element<0> pte, mmu::hw_ptep<0> ptep,
                                  uintptr_t va) {
        // if (perthread_ctx.issued == 0) migration_lock.lock();
        assert(sched::preemptable());

        SCOPE_LOCK(preempt_lock);

        assert(!sched::preemptable());

        unsigned my_cpu_id = sched::cpu::current()->id;

        int remain = 0;

        auto protect_pte = protected_pte(ptep, my_cpu_id);  // protect pte
        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
                // ddc::percpu_ctx[my_cpu_id].worker->yield_to_worker();
                adios_ctx_yield_global();
                my_cpu_id = sched::cpu::current()->id;
            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }
        // trace_pusnow_pf_general(2);

        // 2. alloc page to fetch
        void *paddr = percpu_ctx[my_cpu_id].worker_core->pp_local.alloc_page();
        assert(paddr != NULL);

        // trace_pusnow_pf_general(3);
        // trace_ddc_mmu_fault_remote_offset(va);
        // uintptr_t fault_page_addr = align_down(_fault_addr,
        // mmu::page_size);

        int ret = 0;
        assert(!sched::preemptable());
        // trace_pusnow_pf_general2(1, my_cpu_id, va);
        ret = percpu_ctx[my_cpu_id].worker_core->fetch_rdma(
            dispatcher::worker_core_t::RDMA_SYNC_QP,
            embed_tag(va, fetch_type::INIT), paddr, va_to_offset(va),
            mmu::page_size);

        assert(!ret);

        // trace_pusnow_pf_general(4);
        auto &ctx = percpu_ctx[my_cpu_id];
        remain += 1;
        ctx.issued += 1;

        // trace_pusnow_pf_general2(2, my_cpu_id, va);
        ctx.req_sjk.push_back({va, &remain, protect_pte, ptep, paddr});

        // trace_pusnow_pf_general(5);

        // trace_pusnow_pf_general(6);
#ifdef PROTOCOL_BREAKDOWN
        uint64_t polling_start_cycle = get_cycles();
#endif

        // trace_pusnow_pf_general(7);
        while (remain > 0) {
            do_polling_sjk(my_cpu_id);
            dispatcher::handle_preempt_requested(my_cpu_id);
        }

        // trace_pusnow_pf_general(8);
#ifdef PROTOCOL_BREAKDOWN
        // maybe okay (at least sync pf)
        uint64_t polling_end_cycle = get_cycles();
        if (percpu_ctx[my_cpu_id].worker) {
            percpu_ctx[my_cpu_id].worker->major_pf += 1;
            percpu_ctx[my_cpu_id].worker->polling_pf +=
                polling_end_cycle - polling_start_cycle;
        }
#endif

        // if (perthread_ctx.issued == 0) migration_lock.unlock();

        assert(!sched::preemptable());

        return true;
    }
    bool do_page_small_remote_sjk2(mmu::pt_element<0> pte, mmu::hw_ptep<0> ptep,
                                   uintptr_t va) {
        // if (perthread_ctx.issued == 0) migration_lock.lock();
        assert(sched::preemptable());

        SCOPE_LOCK(preempt_lock);

        assert(!sched::preemptable());

        unsigned my_cpu_id = sched::cpu::current()->id;

        int remain = 0;

        auto protect_pte = protected_pte(ptep, my_cpu_id);  // protect pte
        // 1. Protect PTE to prevent duplicated fetching
        while (true) {
            if (pte.valid()) {
                // Handled by other thread
                return true;
            } else if (is_protected(pte)) {
                DROP_LOCK(preempt_lock) {
                    // ddc::percpu_ctx[my_cpu_id].worker->yield_to_worker();
                    adios_ctx_yield_global();
                }
                my_cpu_id = sched::cpu::current()->id;
            } else if (!is_remote(pte)) {
                abort("unknwon fault 2");
            } else {
                if (ptep.compare_exchange(pte, protect_pte)) {
                    break;
                }
            }
            pte = ptep.read();
        }
        // trace_pusnow_pf_general(2);

        // 2. alloc page to fetch
        void *paddr = percpu_ctx[my_cpu_id].worker_core->pp_local.alloc_page();
        assert(paddr != NULL);

        // trace_pusnow_pf_general(3);
        // trace_ddc_mmu_fault_remote_offset(va);
        // uintptr_t fault_page_addr = align_down(_fault_addr,
        // mmu::page_size);

        int ret = 0;
        assert(!sched::preemptable());
        // trace_pusnow_pf_general2(1, my_cpu_id, va);
        ret = percpu_ctx[my_cpu_id].worker_core->fetch_rdma(
            dispatcher::worker_core_t::RDMA_SYNC_QP,
            embed_tag(va, fetch_type::INIT), paddr, va_to_offset(va),
            mmu::page_size);

        assert(!ret);

        // trace_pusnow_pf_general(4);
        auto &ctx = percpu_ctx[my_cpu_id];
        remain += 1;
        ctx.issued += 1;

        // trace_pusnow_pf_general2(2, my_cpu_id, va);
        ctx.req_sjk.push_back({va, &remain, protect_pte, ptep, paddr});

        // trace_pusnow_pf_general(5);

        // trace_pusnow_pf_general(6);
#ifdef PROTOCOL_BREAKDOWN
        uint64_t polling_start_cycle = get_cycles();
#endif

        // trace_pusnow_pf_general(7);
        while (remain > 0) {
            do_polling_sjk(my_cpu_id);
            dispatcher::handle_preempt_requested(my_cpu_id);
        }

        // trace_pusnow_pf_general(8);
#ifdef PROTOCOL_BREAKDOWN
        // maybe okay (at least sync pf)
        uint64_t polling_end_cycle = get_cycles();
        if (percpu_ctx[my_cpu_id].worker) {
            percpu_ctx[my_cpu_id].worker->major_pf += 1;
            percpu_ctx[my_cpu_id].worker->polling_pf +=
                polling_end_cycle - polling_start_cycle;
        }
#endif

        // if (perthread_ctx.issued == 0) migration_lock.unlock();

        assert(!sched::preemptable());

        return true;
    }
    const uintptr_t _fault_addr;
};

template <typename T>
inline ulong ddc_operate_range(T mapper, void *start, size_t size) {
    return mmu::operate_range(mapper, NULL, reinterpret_cast<void *>(start),
                              size);
}

void vm_fault(uintptr_t addr, exception_frame *ef) {
    trace_ddc_mmu_fault(addr);
#ifdef PROTOCOL_BREAKDOWN
    uint64_t pf_start_cycle = get_cycles();
#endif

    STAT_COUNTING(ddc_page_fault);

    uintptr_t fault_addr = addr;
    addr = align_down(addr, mmu::page_size);

    fault_handling pto(fault_addr);

    ddc_operate_range(pto, reinterpret_cast<void *>(addr), mmu::page_size);

#ifdef PROTOCOL_BREAKDOWN
    uint64_t pf_end_cycle = get_cycles();
    WITH_LOCK(preempt_lock) {
        unsigned my_cpu_id = sched::cpu::current()->id;
        dispatcher::worker_t *worker = percpu_ctx[my_cpu_id].worker;
        if (worker) {
            worker->num_pf += 1;
            worker->sum_pf += pf_end_cycle - pf_start_cycle;
        }
    }

#endif
}

static size_t ddc_in_use = 0;
static size_t ddc_in_use_gb_last = 0;

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset) {
    trace_ddc_mmu_mmap(addr, length, prot, flags);
    assert(!(flags & MAP_FIXED));
    assert(prot == (PROT_WRITE | PROT_READ) || prot == (PROT_READ));
    assert(!(flags & MAP_POPULATE));
    assert(!(flags & MAP_FILE));
    // assert(!(flags & MAP_SHARED));
    assert(flags & MAP_DDC);

    uintptr_t start = reinterpret_cast<uintptr_t>(addr);
    assert(addr == NULL || start == vma_middle);

    if (start == 0) {
        start = vma_start;
    }

    size_t mmap_align_bits = (flags >> MAP_ALIGNMENT_SHIFT) & 0xFF;
    mmap_align_bits = mmap_align_bits < 12 ? 12 : mmap_align_bits;
    size_t mmap_align = 1 << mmap_align_bits;

    // we do not check MAPED or UNMAPED
    // we trust jemalloc

    uintptr_t allocated = 0;
    bool special = false;

    {
        SCOPE_LOCK(mmu::vma_list_mutex.for_write());
        allocated = find_hole(start, length, mmap_align);
        if (flags & MAP_FIX_FIRST_TWO) {
            assert(length == 0x10000000);
            special = true;
            // MiMalloc Handling...
            constexpr size_t first_two_size = 2 * mmu::page_size;
            constexpr size_t segment_size = 0x400000;

            for (size_t i = 0; i < length; i += segment_size) {
                auto *front_vma = new anon_vma(
                    addr_range(allocated + i, allocated + i + first_two_size),
                    mmu::perm_rw,
                    mmu::mmap_fixed | mmu::mmap_small | mmu::mmap_populate);
                front_vma->set(allocated + i, allocated + i + first_two_size);
                vma_list.insert(*front_vma);
                mmu::populate_vma(front_vma, (void *)(allocated + i),
                                  first_two_size);
                auto *vma =
                    new mock_vma(addr_range(allocated + i + first_two_size,
                                            allocated + i + segment_size));
                vma->set(allocated + i + first_two_size,
                         allocated + i + segment_size);
                vma_list.insert(*vma);
            }

        } else {
            auto *vma = new mock_vma(addr_range(allocated, allocated + length));
            vma->set(allocated, allocated + length);
            vma_list.insert(*vma);
        }
        ddc_in_use += length;
        size_t ddc_in_use_gb = ddc_in_use >> 30;

        if (ddc_in_use_gb_last < ddc_in_use_gb) {
            ddc_in_use_gb_last = ddc_in_use_gb;
            printf("alloc: %p (%ld): current %f GB\n", allocated, length,
                   (float)ddc_in_use / (1024 * 1024 * 1024));
        }
    }

    if (special) allocated += 0x100000000000ul;

    assert(is_ddc(allocated));

    if (fd > 0) {
        assert(offset == 0);

        ssize_t rd;
        size_t total_read = 0;
        off_t offset_old = lseek(fd, 0, SEEK_CUR);
        off_t offset_new = lseek(fd, 0, SEEK_SET);
        char *ret_addr = reinterpret_cast<char *>(allocated);
        if (offset_new != 0) {
            printf("strange... offset_new: %d,offset_old:%d\n", offset_new,
                   offset_old);
            abort();
        }

        while (total_read < length) {
            size_t to_this_read = length - total_read > (1ULL << 30)
                                      ? (1ULL << 30)
                                      : length - total_read;  // max 1GB
            rd = read(fd, ret_addr + total_read, to_this_read);
            if (rd <= 0) {
                // An error occurred
                off_t offset_strange = lseek(fd, 0, SEEK_CUR);
                printf("rd: %d offset_strange: %d errno: %d\n", rd,
                       offset_strange, errno);
                abort();
                return MAP_FAILED;
            }
            total_read += rd;
        }
        offset_new = lseek(fd, offset_old, SEEK_SET);
        if (offset_old != offset_new) abort();
    }

    return reinterpret_cast<void *>(allocated);
}

// static remote_queue syscall_queue({}, {max_syscall});

int madvise_dontneed(void *addr, size_t length) {
    // madv_dontneed pto;
    // WITH_LOCK(page_list_lock) { ddc_operate_range(pto, addr, length); }
    abort();
    return 0;
}
int madvise_free(void *addr, size_t length) {
    // do not need to hold syscall lock, no pushing
    // madv_freeing pto;
    // ddc_operate_range(pto, addr, length);
    abort();
    return 0;
}

static mutex_t syscall_mutex;
int madvise_pageout(void *addr, size_t length) {
    SCOPE_LOCK(syscall_mutex);
    abort();
    // pageout pto(syscall_worker_core);
    // ddc_operate_range(pto, addr, length);

    return 0;
}

int madvise_pageout_sg(void *addr, size_t length, uint16_t mask) {
    SCOPE_LOCK(syscall_mutex);
    abort();
    //  FFFF FFFF FFFF FFFF
    // 0x1111111111111111
    // FFFF
    // if (!mask) return -1;
    // uint64_t expanded = _pdep_u64(mask, 0x1111111111111111) * 0xF;
    // pageout_sg pto(expanded);
    // ddc_operate_range(pto, addr, length);
    // return 0;
}
int madvise_print_stat() {
    size_t fetched_total = 0;
    size_t pushed_total = 0;
    size_t fetched = 0;
    size_t pushed = 0;

    for (size_t i = 0; i < sched::cpus.size(); ++i) {
        assert(i < max_cpu);

        fetched = 0;
        pushed = 0;

        // percpu_ctx[i].queues.get_stat(fetched, pushed);
#ifdef PRINT_ALL
        debug_early_u64("CPU: ", i);
        debug_early_u64("  fetched: ", fetched);
        debug_early_u64("  pushed : ", pushed);
#endif

        fetched_total += fetched;
        pushed_total += pushed;
    }
    fetched = 0;
    pushed = 0;
    // syscall_queue.get_stat(fetched, pushed);
#ifdef PRINT_ALL
    debug_early("Syscall:\n");
    debug_early_u64("  fetched: ", fetched);
    debug_early_u64("  pushed : ", pushed);
#endif
    fetched_total += fetched;
    pushed_total += pushed;

    fetched = 0;
    pushed = 0;
    // eviction_get_stat(fetched, pushed);
#ifdef PRINT_ALL
    debug_early("Eviction:\n");
    debug_early_u64("  fetched: ", fetched);
    debug_early_u64("  pushed : ", pushed);
#endif

    fetched_total += fetched;
    pushed_total += pushed;
    printf("fetched: %ld pushed: %ld\n", fetched_total, pushed_total);

    return 0;
}

int madvise(void *addr, size_t length, int advice) {
    trace_ddc_mmu_madvise(addr, length, advice);
    int mask = advice & (~MADV_DDC_MASK);
    int advice2 = advice & (MADV_DDC_MASK);
    switch (advice2) {
        case MADV_DONTNEED:
            return madvise_dontneed(addr, length);
        case MADV_FREE:
            return madvise_free(addr, length);
        case MADV_DDC_PAGEOUT:
            return madvise_pageout(addr, length);
        case MADV_DDC_PAGEOUT_SG:
            return madvise_pageout_sg(addr, length, mask);
        case MADV_DDC_PRINT_STAT:
            return madvise_print_stat();
        default:
            printf("MADVISE FAIL: %d\n", advice);
            return -1;
    }
}

int mprotect(void *addr, size_t len, int prot) {
    trace_ddc_mmu_mprotect(addr, len, prot);
    assert(prot == (PROT_WRITE | PROT_READ) || prot == (PROT_READ));
    return 0;
}
int munmap(void *addr, size_t length) {
    trace_ddc_mmu_munmap(addr, length);
    {
        SCOPE_LOCK(vma_list_mutex.for_write());

        ddc_in_use -= length;
        auto it = mmu::find_intersecting_vma((uintptr_t)addr);
        assert(it->addr() == addr);
        assert(it->size() == align_up(length, mmu::page_size));
        auto &del = *it;
        vma_list.erase(del);
        delete &del;
    }

    // printf("free : %p (%ld): current %f GB\n", addr, length,
    //        (float)ddc_in_use / (1024 * 1024 * 1024));
    // TODO: unmap
    return 0;
}
int msync(void *addr, size_t length, int flags) {
    trace_ddc_mmu_msync(addr, length, flags);
    abort();
}
int mincore(void *addr, size_t length, unsigned char *vec) {
    trace_ddc_mmu_mincore(addr, length);
    abort();
}

int mlock(const void *addr, size_t len) {
    trace_ddc_mmu_mlock(addr, len);
    abort();

    return 0;
}
int munlock(const void *addr, size_t len) {
    trace_ddc_mmu_munlock(addr, len);
    abort();
}

namespace options {
extern std::string prefetcher;
void *prefetcher_init_f = NULL;
}  // namespace options

void ddc_init() {
    // remote_init();
    // reduce_total_memory(remote_reserve_size());

    // elf_replacer_init(); // !Important: for dilos++ do not replace malloc
}

}  // namespace ddc

void (*mi_process_load_hook)() = NULL;

namespace dispatcher {

static unsigned int ipi_vector;
// static void recv_ipi() {
//     unsigned my_cpu_id = sched::cpu::current()->id;
//     ddc::percpu_ctx[my_cpu_id].preempt_requested.store(
//         true, std::memory_order_release);
// }

void recv_ipi() {
    unsigned my_cpu_id = sched::cpu::current()->id;
    if (sched::get_preempt_counter() > 1) {
        return;
    }
    assert(sched::get_preempt_counter() == 1);
    // return;
    // if job yield

    if (ddc::percpu_ctx[my_cpu_id].worker &&
        ddc::percpu_ctx[my_cpu_id].worker->job &&
        ddc::percpu_ctx[my_cpu_id].worker->job->ctx() &&
        ddc::percpu_ctx[my_cpu_id].worker->job->ctx()->check_ptr_in_stk(
            (void *)&my_cpu_id)) {
        // DROP_LOCK(preempt_lock) {  // held by rcu lock
        DROP_LOCK(irq_lock) {  // held by hw
            // ddc::trace_pusnow_recvipi_job(
            //     ddc::percpu_ctx[my_cpu_id].worker->job->wr_id());
            // ddc::trace_pusnow_recvipi(4);
            // ddc::percpu_ctx[my_cpu_id].worker->yield_to_worker();
            adios_ctx_yield_global();

            // ddc::trace_pusnow_recvipi_job(
            //     ddc::percpu_ctx[my_cpu_id].worker->job->wr_id());
            // ddc::trace_pusnow_recvipi(5);
            // }
        }
        my_cpu_id = sched::cpu::current()->id;
    }

    // ddc::trace_pusnow_recvipi_job(
    //     ddc::percpu_ctx[my_cpu_id].worker->job->wr_id());
    // ddc::trace_pusnow_recvipi(6);
    // if worker, just return
}
void send_ipi(unsigned apic_id) {
    // ddc::trace_pusnow_recvipi(99);
    processor::apic->ipi(apic_id, ipi_vector);
}

int init_os() {
    for (size_t i = 0; i < sched::cpus.size(); ++i) {
        assert(i < ddc::max_cpu);
        ddc::percpu_ctx[i].worker =
            dispatcher::default_dispatcher.worker_checked_by_core_id(i);
        ddc::percpu_ctx[i].worker_core =
            dispatcher::default_dispatcher.worker_core_checked_by_core_id(i);
    }

    // ipi init
    ipi_vector = idt.register_handler_legacy(recv_ipi);
    // ipi_vector = idt.register_handler(recv_ipi);

    // mimalloc init
    if (mi_process_load_hook) mi_process_load_hook();
    return 0;
}
int change_pf_mode(int pf_mode) {
    WITH_LOCK(preempt_lock) {
        unsigned my_cpu_id = sched::cpu::current()->id;

        ddc::percpu_ctx[my_cpu_id].pf_mode = pf_mode;
    }

    return 0;
}
unsigned get_apic_id() {
    SCOPE_LOCK(preempt_lock);
    return sched::cpu::current()->arch.apic_id;
}

void disable_preemption() {
    assert(sched::preemptable());
    sched::preempt_disable();
    assert(!sched::preemptable());
}
void enable_preemption() {
    assert(!sched::preemptable());
    sched::preempt_enable();
    assert(sched::preemptable());
}
bool preemptable() { return sched::preemptable(); }

void set_ipi_stack(char *ptr, size_t s) {
    SCOPE_LOCK(irq_lock);
    sched::cpu::current()->arch.set_ist_entry(3, ptr, s);
}

bool handle_wait(job_t *job, void *ptep_raw) {
    // now worker context
    auto ptep = mmu::hw_ptep<0>::force((mmu::pt_element<0> *)ptep_raw);

    auto pte = ptep.read();
    auto protect_pte =
        ddc::protected_pte_with_addr(ptep,
                                     (uintptr_t)job);  // protect pte
    while (true) {
        if (pte.valid()) {
            // Handled by other thread
            return false;
        } else if (ddc::is_protected(pte)) {
            job->next = (dispatcher::job_t *)ddc::get_protect_addr(pte);
            if (ptep.compare_exchange(pte, protect_pte)) {
                return true;
            }

        } else if (!ddc::is_remote(pte)) {
            abort("unknwon fault 2");
        } else {
            return false;
        }
        pte = ptep.read();
    }

    abort();
}
unsigned get_cpu_id() { return sched::cpu::current()->id; }
void handle_preempt_requested(unsigned &my_cpu_id) {
    if (ddc::percpu_ctx[my_cpu_id].worker->preempt_requested.load(
            std::memory_order_acquire)) {
        ddc::percpu_ctx[my_cpu_id].worker->preempt_requested.store(
            false, std::memory_order_release);

        DROP_LOCK(preempt_lock) {
            // ddc::percpu_ctx[my_cpu_id].worker->yield_to_worker();
            adios_ctx_yield_global();
        }
        my_cpu_id = sched::cpu::current()->id;
    }
}

}  // namespace dispatcher
extern "C" {
enum ddc_prefetch_result_t ddc_prefetch(const struct ddc_event_t *event,
                                        struct ddc_prefetch_t *command) {
    // return ddc::do_prefetch(event, command);
    return DDC_RESULT_ERR_UNKNOWN;
}
}
