#pragma once

#include <config/static-config.hh>
#include <dispatcher/api.hh>
#include <dispatcher/common.hh>
#include <dispatcher/job.hh>
#include <dispatcher/logger.hh>
#include <dispatcher/net.hh>
#include <dispatcher/page.hh>
#include <dispatcher/pagepool.hh>

namespace dispatcher {

class dispatcher_t;

class worker_core_t {
   public:
    static constexpr size_t MAX_RDMA_QP = 2;
    static constexpr int RDMA_SYNC_QP = 0;
    static constexpr int RDMA_ASYNC_QP = 1;

    struct worker_core_init_t {
        dispatcher_t *dispatcher;
        struct ibv_cq_ex *cq;
        struct ibv_cq_ex *cq_async;
        std::vector<struct ibv_qp_ex *> qps;
        pagepool_global_t<> *pp_global;
        page_mgmt_t *page_mgmt;
    };

    int init_wcore(const worker_core_init_t &config);

    int fetch_rdma(int qp_id, uintptr_t token, void *paddr, uintptr_t roffset,
                   size_t size);
    int push_rdma(int qp_id, uintptr_t token, void *paddr, uintptr_t roffset,
                  size_t size);
    int poll_rdma(uintptr_t tokens[], int len);

    pagepool_global_t<> *pp_global;
    pagepool_local_t<> pp_local;
    page_mgmt_local_t page_mgmt_local;

   protected:
    dispatcher_t *dispatcher;

    struct ibv_cq_ex *rdma_cq;
    struct ibv_cq_ex *rdma_cq_async;
    std::array<struct ibv_qp_ex *, MAX_RDMA_QP> rdma_qps;

#ifdef DILOS_RDMA_PERF
    size_t my_worker_id;
    size_t rdma_count;
    size_t rdma_last_print;
#endif
};

// per-core worker
class worker_t : public logger_t, public worker_core_t {
   public:
    static constexpr size_t NUM_ETH_TX_DESC = 512;

    static constexpr uint8_t STATE_READY = 1;
    static constexpr uint8_t STATE_RUNNING = 2;
    static constexpr uint8_t STATE_YIELD = 3;
    static constexpr uint8_t STATE_YIELD_GLOBAL = 4;  // used by sjk
    static constexpr uint8_t STATE_ERROR = 5;
    static constexpr uint8_t STATE_UNUSED = 6;
    static constexpr uint8_t STATE_UNINIT = 7;

    static constexpr int JOB_RET_ERROR = -1;
    static constexpr int JOB_RET_PF_ASYNC = -100;
    static constexpr int JOB_RET_YIELD = -101;
    static constexpr int JOB_RET_YIELD_GLOBAL = -102;
    static constexpr int JOB_RET_WAIT = -103;
    static constexpr int JOB_RET_ALLOC_ASYNC = -104;

    struct init_t {
        struct {
            struct ibv_qp *tx_qp_rtr;
            net_conifg_t config;
        } eth;
        worker_core_t::worker_core_init_t wcore;
        size_t rdma_async_depth;

        struct {
            worker_id_t worker_id;
            int core_id;
        } worker;
        int pf_mode;
    };

    int init(const init_t &config);
    int init_eth(struct ibv_qp *tx_qp_rtr, struct ibv_cq_ex *tx_qp = nullptr);
    // global thread unsafe
    int start(const job_adapter_t::init_t &app_config);

    template <int pf_mode>
    uint8_t handle_job();

    template <int pf_mode>
    int worker_main_aio();
    int poll_cq_async(job_t **jobs);

    int send_tx_eth_one(eth_addr_t dst, void *page, size_t len);
    int send_tx_eth_one_sync(eth_addr_t dst, void *page, size_t len);
    int poll_eth_cq_async();

   public:
    std::atomic_uint8_t state;

    job_t *job;
    job_t *dispatched_job;
    job_list_t yield_job_list;
    uint64_t yield_count;
    job_list_t yield_global_job_list;
    uint64_t yield_global_count;
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
    job_list_t free_job_list;
    job_list_t free_job_list_local;
#endif

    job_list_t ready_local;

#ifndef DILOS_SYNC_ALLOC

    job_list_t async_wait_alloc_list;
#endif

    size_t rdma_async_depth;
    size_t rdma_async_issued;
    int core_id;
    int pf_mode;
    int return_code;            // set by job
    uintptr_t async_pf_offset;  // set by job
    void *async_pf_pa_or_ptep;  // set by job
    worker_id_t worker_id;
    worker_core_t wcore;

#ifdef PROTOCOL_BREAKDOWN
    uint64_t num_pf;
    uint64_t major_pf;
    uint64_t sum_pf;
    uint64_t polling_pf;
#endif

    int run_job();
    int run_job_ctx();
    int run_job_ctx_with_nstx();
    int run_job_ctx_sjk();
    int resume_job_ctx();
    int pf_async_to_worker(uintptr_t va, void *pa);  // called by job
    int page_alloc_async_to_worker();                // called by job
    int yield_to_worker();                           // called by job
    int yield_global_to_worker();                    // called by job
    int wait_to_worker(void *ptep);                  // called by job
    int exit_to_worker();                            // called by job

    // called by dispatcher
    inline void send_preempt() {
        preempt_requested.store(true, atomic_on_store);  // always...
        send_ipi(apic_id);
    }

    std::atomic_bool preempt_requested;  // by sjk
   private:
    int init_worker(const init_t &config);

    void prepare_main();

    // member varaibles
    unsigned apic_id;

    struct ibv_cq_ex *eth_tx_cq;
    struct ibv_qp_ex *eth_tx_qp;

    net_conifg_t net_config;
    context_inner_t uctx;
    std::thread _th;
    std::mutex mtx;
    std::condition_variable cv;
};
}  // namespace dispatcher