#pragma once

#include <config/static-config.hh>
#include <dispatcher/common.hh>
#include <dispatcher/job.hh>
#include <dispatcher/logger.hh>
#include <dispatcher/mm.hh>
#include <dispatcher/net.hh>
#include <dispatcher/page.hh>
#include <dispatcher/pagepool.hh>
#include <dispatcher/rdma.hh>
#include <dispatcher/reclaimer.hh>
#include <dispatcher/worker.hh>

namespace dispatcher {

int init_os();

class dispatcher_t : public logger_t, public worker_core_t {
   public:
    static constexpr long long PF_MODE_NONE = 0;
    static constexpr long long PF_MODE_SYNC = 1;
    static constexpr long long PF_MODE_ASYNC = 2;
    static constexpr long long PF_MODE_HYBRID = 3;
    static constexpr long long PF_MODE_ASYNC_LOCAL = 4;
    static constexpr long long PF_MODE_SYNC_SJK = 5;
    static constexpr long long PF_MODE_SYNC_SJK2 = 6;
    static constexpr long long PF_MODE_SYNC_SJK2_NONE = 7;

    static constexpr long long DP_MODE_SINGLE = 0;
    static constexpr long long DP_MODE_MULTI = 1;

    struct init_t {
        long long pf_mode;
        long long dp_mode;
        long long sched_mode;
        long long preemption_cycles;
        struct {
            std::string eth_name;
            std::string ib_name;
        } dev;
        struct {
            int port_num = 0;
            net_conifg_t config;
        } eth;

        struct mm_t::init_t mm;
        struct {
            int core_id = 0;
            size_t rdma_sync_depth;
        } dispatcher;
        struct {
            bool disable = false;
            int rdma_port_num = 0;
            int sgid_idx = 0;
            std::string hostname;
            std::string port;
        } mem_server;

        struct {
            std::vector<int> core_ids;
            size_t rdma_sync_depth;
            size_t rdma_async_depth;
        } worker;

        struct {
            int core_id;
            size_t start_percent;
            size_t end_percent;
        } reclaimer;

        struct job_t::init_t app;
    };

    int init(const init_t &config);
    int run();
    template <bool do_smart = false>
    int run_multi_cq();
    template <bool do_smart = false>
    int run_single_cq();
    template <bool no_ipi = false>
    int run_single_cq_preempt();

    int process_cq_n(struct ibv_cq_ex *cq_to_process, size_t num);
    int process_cq_all(struct ibv_cq_ex *cq_to_process);

    worker_t &worker(worker_id_t worker_id);
    worker_t *worker_checked_by_core_id(int lu_core_id);
    worker_core_t *worker_core_checked_by_core_id(int core_id);

    std::atomic_uint64_t num_job_pending;
    mm_t mm;
    rdma_t rdma;

   private:
    // private methods
    // 1. init methods

    int init_dev(const init_t &config);
    int init_eth(const init_t &config);
    int init_rdma(const init_t &config);
    int init_dispatcher(const init_t &config);
    int init_worker(const init_t &config);
    int start_workers(const job_adapter_t::init_t &app_config);
    int init_reclaimer(const init_t &config);
    int start_reclaimer();

    int init_app(const init_t &config);

    int add_worker(const init_t &config, worker_id_t worker_id, int core_id);
    // 3. operations
    int post_eth_recvs(int num);

    void handle_eth_rx(void *page);
    void handle_eth_rx_in_dp(void *page);
    void check_workers();
    void check_workers_smart();

    template <bool no_ipi = false>
    void check_workers_preempt(uint64_t *dispatch_time);

    template <bool use_yield_local = false>
    void dispatch_job(worker_id_t worker_id);
    struct ibv_cq_ex *create_rdma_cqx(size_t depth);
    struct ibv_qp_ex *create_rdma_qpx(struct ibv_cq_ex *rdma_cq, size_t depth);

    void enqueue_job(job_t *job);
    void enqueue_jobs(job_list_t *jobs, uint64_t count);

    inline void free_jobs(job_list_t *jobs) {
        while (!jobs->empty()) {
            pp_pkt_local.free_page((void *)jobs->pop());
        }
    }

    // member variables
    struct ibv_device *eth_dev;
    struct ibv_device *ib_dev;
    struct ibv_context *eth_ctx;
    struct ibv_context *ib_ctx;
    struct ibv_pd *ib_pd;
    struct ibv_pd *eth_pd;
    struct ibv_cq_ex *central_cq_eth;
    struct ibv_cq_ex *central_cq_rdma;
    struct ibv_flow *eth_rx_flow;
    struct ibv_qp *eth_rx_qp;
    int eth_port_num;

    int pf_mode;
    int dp_mode;
    int sched_mode;
    int preemption_cycles;
    int rdma_port_num;
    int rdma_sgid_idx;
    size_t eth_rx_count;
    struct ibv_port_attr rdma_port_attr;
    union ibv_gid rdma_gid;

    std::array<struct ibv_recv_wr, NUM_ETH_RX_DESC> eth_recv_wrs;
    std::array<struct ibv_sge, NUM_ETH_RX_DESC> eth_recv_sges;

    std::array<worker_t, MAX_WORKERS> workers;
    reclaimer_t reclaimer;  // only used in OSV
    int core_id;
    worker_id_t num_pinned_workers;
    worker_id_t last_worker_id_start;

    pagepool_global_t<> pp_global;
    page_mgmt_t page_mgmt;

    pagepool_local_only_t<ETH_BUFFER_BTCH_SIZE> pp_pkt_local;
    size_t pp_pkt_free = 0;
    size_t pp_pkt_posted = 0;

    net_conifg_t net_config;

    job_list_t job_ready;
    std::array<job_list_t, MAX_WORKERS> job_ready_local;
#ifdef PROTOCOL_BREAKDOWN
    uint64_t last_job_wr_id = 0;
#endif
};

extern dispatcher_t default_dispatcher;
}  // namespace dispatcher