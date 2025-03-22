
#include <ddc/mmu.hh>
#include <dispatcher/reclaimer.hh>
#include <osv/mmu.hh>

#include "pte.hh"

//         ┌────────────────┐      ┌────────────────┐
//         │page_mgmt_local │      │page_mgmt_local │
//         └────────────────┘      └────────────────┘
//                  │                        │
//                  └──────────┬─────────────┘
//                             ▼
//                     ┌──────────────┐
//                     │  page_mgmt   │
//                     └──────────────┘
//                             │
//                             │
// ┌───────────────────────────▼─────────────────────────────┐
// │Cleaning           ┌──────────────┐                      │
// │          ┌────────│ active_pages │──────────┐           │
// │ accessed,│        └──────────────┘          │ dirty,    │
// │   unset  │           ▲   ▲  │unaccessed     │  unset    │
// │ accessed │    tlb    │   │  │and clean      │  dirty    │
// │          ▼  flushed  │   │  │               ▼           │
// │  ┌──────────────┐    │   │  │       ┌──────────────┐    │
// │  │ to_tlb_flush │────┘   │  │       │     RDMA     │    │
// │  └──────────────┘        │  │       └──────────────┘    │
// │                          │  │               │   I/O     │
// │                          │  │               │ polled    │
// └──────────────────────────┼──┼───────────────┼───────────┘
// ┌──────────────────────────┼──┼───────────────┼───────────┐
// │Eviction         accessed │  ▼               │           │
// │                   ┌──────────────┐          │           │
// │                   │ clean_pages  │◀─────────┘           │
// │                   └──────────────┘                      │
// │                           │unaccessed                   │
// │                           ▼                             │
// │                         free                            │
// └─────────────────────────────────────────────────────────┘

namespace dispatcher {

using ptep_t = mmu::hw_ptep<0>;
using pte_t = mmu::pt_element<0>;

int reclaimer_t::init(const init_t &config) {
    SET_MODULE_NAME("reclaimer");
    int ret = 0;
    PRINT_LOG("core_id: %d\n", config.reclaimer.core_id);

    core_id = config.reclaimer.core_id;

    page_mgmt = config.wcore.page_mgmt;
    reclaim_start =
        (page_mgmt->num_pages * config.reclaimer.start_percent) / 100;
    reclaim_end = (page_mgmt->num_pages * config.reclaimer.end_percent) / 100;
    pp_global = config.wcore.pp_global;
    PRINT_LOG(
        "num_pages: %ld, reclaim_start_pages: %ld reclaim_end_pages: %ld\n",
        page_mgmt->num_pages, reclaim_start, reclaim_end);
    PRINT_LOG("Initing Worker Core\n");
    ret = init_wcore(config.wcore);
    if (ret) return ret;

    return ret;
}

static int reclaimer_main_outer(reclaimer_t &reclaimer) {
    return reclaimer.reclaimer_main();
}
int reclaimer_t::start() {
    std::unique_lock<std::mutex> lck(mtx);

    _th = std::thread(reclaimer_main_outer, std::ref(*this));
    cv.wait(lck);
    PRINT_LOG("thread started (main)\n");

    return 0;
}
void reclaimer_t::prepare_main() {
    std::unique_lock<std::mutex> lck(mtx);
    PRINT_LOG("thread starting...\n");

    if (core_id >= 0) {
        // 1. thread affinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        int rc =
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        (void)rc;
    }
    PRINT_LOG("core_id: %d\n", core_id);
    cv.notify_all();
}
int reclaimer_t::reclaimer_main() {
    prepare_main();
    size_t current_num_pages = 0;
    current_num_pages = pp_global->count_pages();
    u64 freeed = 0;
    while (1) {
        // reclaim logic: watermark
        do {
            _mm_pause();
            current_num_pages = pp_global->count_pages();
        } while (current_num_pages > reclaim_end);

        // debug_early("reclaim start\n");
        freeed = request_memory(reclaim_end - current_num_pages);
        (void)freeed;
    }
    abort();

    return 0;
}

size_t reclaimer_t::request_memory(size_t num_pages) {
    size_t freed = 0;

    while (freed < num_pages) {
        bool tlb_flushed = end_tlb_flush();

        freed += evict_slice(128);
        if (tlb_flushed && to_tlb_flush.size() > 128) {
            start_tlb_flush();
        }
        poll_once();
    }
    end_tlb_flush();
    return freed;
}

void reclaimer_t::fill_active_pages() {
    page_mgmt->pop_pages_active_all(active_pages);
}
size_t reclaimer_t::evict_slice(size_t N) {
    size_t freed = 0;
    if (clean_pages.size() == 0) {
        clean_slice(N);
    }

    auto page = clean_pages.begin();

    for (size_t count = 0; page != clean_pages.end() && count < N; ++count) {
        auto ptep = ptep_t::force((pte_t *)page->ptep);
        auto pte_old = ptep.read();
        if (pte_old.accessed()) {
            auto next = clean_pages.erase(page);
            active_pages.push_back(*page);
            page = next;
        } else {
            // not accessed -> no entry in TLB
            auto pte_new = ddc::remote_pte_vec(ptep, 0);

            if (!ptep.compare_exchange(pte_old, pte_new)) {
                // retry
                --count;
                continue;
            }

            auto next = clean_pages.erase(page);
            ++freed;
            auto &pg = *page;
            void *paddr = (void *)page_mgmt->lookup(pg);
            pg.reset();
            pp_local.free_page(paddr);
            page = next;
        }
    }

    return freed;
}
size_t reclaimer_t::clean_slice(size_t N) {
    size_t cleaned = 0;
    fill_active_pages();
    auto page = active_pages.begin();
    page_slice_t::iterator next;
    for (size_t count = 0; page != active_pages.end() && count < N; ++count) {
        auto ptep = ptep_t::force((pte_t *)page->ptep);
        auto pte_old = ptep.read();

        if (ddc::is_remote(pte_old) || ddc::is_protected(pte_old)) {
            // skip
            ++page;
            continue;
        }

        if (!pte_old.valid()) {
            // unknown state
            abort();
        }

        if (pte_old.accessed()) {
            // if accessed unset accessed and push to to_tlb_flush
            pte_t pte_new = pte_old;
            pte_new.set_accessed(false);

            if (!ptep.compare_exchange(pte_old, pte_new)) {
                // retry
                --count;
                continue;
            }
            next = active_pages.erase(page);
            to_tlb_flush.push_back(*page);
            page = next;

            // if accessed, clock
        } else if (pte_old.dirty()) {
            // if dirty (and not accessed) unset dirty and push to remote
            pte_t pte_new = pte_old;
            pte_new.set_dirty(false);

            if (!ptep.compare_exchange(pte_old, pte_new)) {
                // retry
                --count;
                continue;
            }
            void *paddr = mmu::phys_to_virt(pte_old.addr());

            while (pushed >= reclaim_max_push) {
                poll_until_one();
            }
            push_rdma(RDMA_SYNC_QP, (uintptr_t)paddr, paddr,
                      ddc::va_to_offset(page->va), NET_PAGE_SIZE);
            ++pushed;

            page = active_pages.erase(page);

        } else {
            // if clean (and not accessed), insert to clean buffer
            ++cleaned;
            next = active_pages.erase(page);  // push to clean
            clean_pages.push_back(*page);
            page = next;
        }
    }

    return cleaned;
}

void reclaimer_t::token_polled(uintptr_t token) {
    auto &page = page_mgmt->lookup((void *)token);
    clean_pages.push_back(page);
}

size_t reclaimer_t::poll_once() {
    if (pushed == 0) return 0;
    uintptr_t tokens[pushed];

    int polled = poll_rdma(tokens, pushed);
    for (int i = 0; i < polled; ++i) {
        token_polled(tokens[i]);
    }
    // assert(pushed != 224);
    pushed -= polled;
    assert(pushed >= 0);
    return polled;
}
size_t reclaimer_t::poll_until_one() {
    size_t polled = poll_once();
    while (!polled) {
        _mm_pause();
        polled = poll_once();
    }
    return polled;
}
size_t reclaimer_t::poll_all() {
    size_t polled = poll_once();
    while (pushed) {
        _mm_pause();
        polled += poll_once();
    }
    return polled;
}

void reclaimer_t::start_tlb_flush() {
    // currently IPI
    if (to_tlb_flush.empty()) {
        return;
    }
    // TODO: ipi-less tlb flush
    tlb_flushing.splice(tlb_flushing.end(), to_tlb_flush);
    mmu::flush_tlb_all();
}
bool reclaimer_t::end_tlb_flush() {
    // currently IPI, always ok

    if (tlb_flushing.empty()) {
        return true;
    }
    active_pages.splice(active_pages.end(), tlb_flushing);

    return true;
}

}  // namespace dispatcher