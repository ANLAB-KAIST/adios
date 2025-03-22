#pragma once

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <config/static-config.hh>
#include <dispatcher/page.hh>
#include <dispatcher/worker.hh>

// RECLAIMER is only used in DiLOS. (dilos/core-ddc/reclaimer.cc)

namespace dispatcher {

constexpr size_t reclaim_slice_size = 32;
constexpr size_t reclaim_max_push = max_evict;

class reclaimer_t : public logger_t, public worker_core_t {
   public:
    struct init_t {
        worker_core_t::worker_core_init_t wcore;

        struct {
            int core_id;
            size_t start_percent;
            size_t end_percent;
        } reclaimer;
    };
    int init(const init_t &config);
    int start();
    int reclaimer_main();

    size_t request_memory(size_t num_pages);

   private:
    void fill_active_pages();
    size_t clean_slice(size_t N);  // push pages to remove memory
    size_t evict_slice(size_t N);  // free clean pages from local memory
    void token_polled(uintptr_t token);
    size_t poll_once();
    size_t poll_until_one();
    size_t poll_all();
    void start_tlb_flush();
    bool end_tlb_flush();
    void prepare_main();
    int core_id;
    page_mgmt_t *page_mgmt;
    pagepool_global_t<> *pp_global;
    std::thread _th;
    std::mutex mtx;
    std::condition_variable cv;

    size_t reclaim_start;
    size_t reclaim_end;

    page_slice_t active_pages;
    page_slice_t clean_pages;
    page_slice_t to_tlb_flush;
    page_slice_t tlb_flushing;
    size_t pushed;
};
}  // namespace dispatcher