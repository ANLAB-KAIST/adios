#include <dispatcher/dispatcher.hh>
#include <dispatcher/raw-eth.hh>
#include <dispatcher/rdma.hh>
#include <dispatcher/traces.hh>

namespace dispatcher {
struct raw_eth_flow_attr {
    struct ibv_flow_attr attr;
    struct ibv_flow_spec_eth spec_eth;
} __attribute__((packed));

int dispatcher_t::init(const init_t &config) {
    SET_MODULE_NAME("dispatcher");
    int ret = 0;

    num_job_pending.store(0, atomic_on_store);

    PRINT_LOG("Initing Config\n");
    pf_mode = config.pf_mode;
    dp_mode = config.dp_mode;
    sched_mode = config.sched_mode;
    preemption_cycles = config.preemption_cycles;
    eth_port_num = config.eth.port_num;
    rdma_port_num = config.mem_server.rdma_port_num;
    rdma_sgid_idx = config.mem_server.sgid_idx;
    last_worker_id_start = 0;
    eth_rx_count = NUM_ETH_RX_DESC;

    PRINT_LOG("Initing DEV\n");
    ret = init_dev(config);
    if (ret) return ret;
    PRINT_LOG("Initing MM\n");
    ret = mm.init(config.mm, eth_pd, ib_pd);
    if (ret) return ret;
    PRINT_LOG("Initing RDMA\n");
    ret = init_rdma(config);
    if (ret) return ret;
    PRINT_LOG("Initing Dispatcher\n");
    ret = init_dispatcher(config);
    if (ret) return ret;
    PRINT_LOG("Initing ETH\n");
    ret = init_eth(config);
    if (ret) return ret;
    PRINT_LOG("Initing Workers\n");
    ret = init_worker(config);
    if (ret) return ret;
#ifdef DRIVER_OSV
    PRINT_LOG("Initing Reclaimer\n");
    ret = init_reclaimer(config);
    if (ret) return ret;
#endif

    PRINT_LOG("Initing OS\n");
    ret = init_os();
    if (ret) return ret;

#ifdef DRIVER_OSV
    PRINT_LOG("Starting Reclaimer\n");
    if (!config.mem_server.disable) {
        ret = start_reclaimer();
        if (ret) return ret;
    }
#endif

    PRINT_LOG("Initing App\n");
    ret = job_t::init_global(config.app);
    if (ret) return ret;

    PRINT_LOG("Starting Workers\n");
    ret = start_workers(config.app);
    if (ret) return ret;

    PRINT_LOG("Initing Ok\n");
    return ret;
}

int dispatcher_t::init_dev(const init_t &config) {
    struct ibv_device **dev_list;
    ib_dev = NULL;
    eth_dev = NULL;
    ib_ctx = NULL;
    eth_ctx = NULL;
    int num_device = 0;
    dev_list = ibv_get_device_list(&num_device);
    if (!dev_list) {
        perror("Failed to get devices list");
        return 1;
    }

    for (int i = 0; i < num_device; ++i) {
        if (config.dev.ib_name == dev_list[i]->name) {
            ib_dev = dev_list[i];
        }
        if (config.dev.eth_name == dev_list[i]->name) {
            eth_dev = dev_list[i];
        }
    }

    if (!eth_dev) {
        PRINT_LOG("ETH device not found\n");
        return 1;
    }

    if (!ib_dev) {
        PRINT_LOG("IB device not found\n");
        return 1;
    }
    eth_ctx = ibv_open_device(eth_dev);
    if (!eth_ctx) {
        PRINT_LOG("Couldn't get context for %s\n",
                  ibv_get_device_name(eth_dev));
        return 1;
    }

    if (eth_dev == ib_dev) {
        ib_ctx = eth_ctx;
    } else {
        ib_ctx = ibv_open_device(ib_dev);
        if (!ib_ctx) {
            PRINT_LOG("Couldn't get context for %s\n",
                      ibv_get_device_name(ib_dev));
            return 1;
        }
    }

    eth_pd = ibv_alloc_pd(eth_ctx);

    if (!eth_pd) {
        PRINT_LOG("Couldn't allocate PD\n");
        return 1;
    }

    if (eth_dev == ib_dev) {
        ib_pd = eth_pd;
    } else {
        ib_pd = ibv_alloc_pd(ib_ctx);

        if (!ib_pd) {
            PRINT_LOG("Couldn't allocate PD\n");
            return 1;
        }
    }

    if (eth_dev != ib_dev && config.dp_mode == DP_MODE_SINGLE &&
        config.pf_mode == PF_MODE_ASYNC) {
        PRINT_LOG(
            "Cannot set DP_MODE_SINGLE+PF_MODE_ASYNC, if eth_dev != ib_dev\n");

        return 1;
    }

    if (eth_dev != ib_dev && config.dp_mode == DP_MODE_SINGLE &&
        config.pf_mode == PF_MODE_HYBRID) {
        PRINT_LOG(
            "Cannot set DP_MODE_SINGLE+PF_MODE_HYBRID, if eth_dev != ib_dev\n");

        return 1;
    }

    size_t num_rdma_wq = 0;
    if (config.pf_mode == PF_MODE_ASYNC || config.pf_mode == PF_MODE_HYBRID) {
        num_rdma_wq =
            config.worker.rdma_async_depth * config.worker.core_ids.size();

        // PF_MODE_ASYNC_LOCAL is handled in worker, so do not reserve space
    }

    struct ibv_cq_init_attr_ex cq_init_attr_ex;
    memset(&cq_init_attr_ex, 0, sizeof(cq_init_attr_ex));

    cq_init_attr_ex.cqe = NUM_ETH_RX_DESC + worker_t::NUM_ETH_TX_DESC *
                                                config.worker.core_ids.size();
    if (dp_mode == DP_MODE_SINGLE) {
        cq_init_attr_ex.cqe += num_rdma_wq;
    }
    cq_init_attr_ex.wc_flags = 0;
    cq_init_attr_ex.flags = IBV_CREATE_CQ_ATTR_SINGLE_THREADED;

    central_cq_eth = ibv_create_cq_ex(eth_ctx, &cq_init_attr_ex);
    if (!central_cq_eth) {
        PRINT_LOG("Couldn't create central CQ eth %d\n", errno);
        return 1;
    }

    if (num_rdma_wq && dp_mode == DP_MODE_MULTI) {
        // ASYNC page fetch is handled in centralized dispatcher, reserve
        // room for that.

        struct ibv_cq_init_attr_ex cq_init_attr_ex_rdma;
        memset(&cq_init_attr_ex_rdma, 0, sizeof(cq_init_attr_ex_rdma));

        cq_init_attr_ex_rdma.cqe = num_rdma_wq;
        cq_init_attr_ex_rdma.wc_flags = 0;
        cq_init_attr_ex_rdma.flags = IBV_CREATE_CQ_ATTR_SINGLE_THREADED;

        central_cq_rdma = ibv_create_cq_ex(ib_ctx, &cq_init_attr_ex_rdma);
        if (!central_cq_rdma) {
            PRINT_LOG("Couldn't create central CQ rdma %d\n", errno);
            return 1;
        }
    } else {
        central_cq_rdma = nullptr;
    }
    return 0;
}
int dispatcher_t::init_eth(const init_t &config) {
    memcpy(&net_config, &config.eth.config, sizeof(net_config));

    // do not need to modify qp to RTS
    struct ibv_qp_cap cap_rx;
    memset(&cap_rx, 0, sizeof(cap_rx));
    int ret = 0;
    cap_rx.max_inline_data = 128;
    cap_rx.max_send_sge = 1;
    cap_rx.max_send_wr = 64;

    cap_rx.max_recv_sge = 1;
    cap_rx.max_recv_wr = NUM_ETH_RX_DESC;

    eth_rx_qp = raweth::create_qp_raw_rtr(eth_ctx, config.eth.port_num, eth_pd,
                                          central_cq_eth, cap_rx, 1);

    if (!eth_rx_qp) {
        PRINT_LOG("err: create_qp_raw_rtr: %p\n", eth_rx_qp);
        return -1;
    }

    // register flow

    uint8_t flow_attr_bytes[sizeof(struct raw_eth_flow_attr)];
    struct raw_eth_flow_attr *flow_attr =
        (struct raw_eth_flow_attr *)flow_attr_bytes;

    memset(flow_attr_bytes, 0, sizeof(flow_attr_bytes));

    flow_attr->attr.comp_mask = 0;
    flow_attr->attr.type = IBV_FLOW_ATTR_NORMAL;
    flow_attr->attr.size = sizeof(flow_attr_bytes);
    flow_attr->attr.priority = 0;
    flow_attr->attr.num_of_specs = 1;
    flow_attr->attr.port = config.eth.port_num;
    flow_attr->attr.flags = 0;

    flow_attr->spec_eth.type = IBV_FLOW_SPEC_ETH;
    flow_attr->spec_eth.size = sizeof(struct ibv_flow_spec_eth);

    memcpy(flow_attr->spec_eth.val.dst_mac, config.eth.config.mac_addr.data(),
           sizeof(flow_attr->spec_eth.val.dst_mac));
    memset(flow_attr->spec_eth.val.src_mac, 0,
           sizeof(flow_attr->spec_eth.val.src_mac));
    flow_attr->spec_eth.val.ether_type = 0;
    flow_attr->spec_eth.val.vlan_tag = 0;

    memset(flow_attr->spec_eth.mask.dst_mac, 0xFF,
           sizeof(flow_attr->spec_eth.mask.dst_mac));
    memset(flow_attr->spec_eth.mask.src_mac, 0,
           sizeof(flow_attr->spec_eth.mask.src_mac));
    flow_attr->spec_eth.mask.ether_type = 0;
    flow_attr->spec_eth.mask.vlan_tag = 0;

    eth_rx_flow =
        ibv_create_flow(eth_rx_qp, (struct ibv_flow_attr *)flow_attr_bytes);

    if (!eth_rx_flow) {
        PRINT_LOG("Couldn't attach steering flow\n");
        return 1;
    }

    for (size_t i = 0; i < NUM_ETH_RX_DESC; ++i) {
        eth_recv_wrs[i].sg_list = &eth_recv_sges[i];
        eth_recv_wrs[i].num_sge = 1;
        eth_recv_sges[i].length = NET_MTU_ASSUME;
    }

    ret = post_eth_recvs(NUM_ETH_RX_DESC);

    if (ret) {
        PRINT_LOG("fail to post eth recvs: %d\n", ret);
        return -1;
    }

    return 0;
}

int dispatcher_t::init_rdma(const init_t &config) {
    if (config.mem_server.disable) {
        return 0;
    }

    memset(&rdma_port_attr, 0, sizeof(rdma_port_attr));
    if (ibv_query_port(ib_ctx, rdma_port_num, &rdma_port_attr)) {
        printf("fail to query port");
        return 1;
    }

    memset(&rdma_gid, 0, sizeof(rdma_gid));
    if (ibv_query_gid(ib_ctx, rdma_port_num, rdma_sgid_idx, &rdma_gid)) {
        printf("fail to query gid");
        return 1;
    }

    return rdma.connect_mem_server(config.mem_server.hostname,
                                   config.mem_server.port);
}

int dispatcher_t::init_dispatcher(const init_t &config) {
    core_id = config.dispatcher.core_id;
    // 1. thread affinity
    PRINT_LOG("core id: %ld\n", config.dispatcher.core_id);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(config.dispatcher.core_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (rc) {
        return rc;
    }

    if (config.mm.addrs.size() == 0) {
        return 1;
    }

    char *start_min = (char *)config.mm.addrs[0].first;
    char *end_max = nullptr;
    for (auto &tp : config.mm.addrs) {
        char *start = (char *)tp.first;
        char *end = start + tp.second;

        if (start < start_min) {
            start_min = start;
        }
        if (end > end_max) {
            end_max = end;
        }
    }
    page_mgmt.init(start_min, end_max);
    worker_core_init_t wconfig;
    memset(&wconfig, 0, sizeof(wconfig));
    wconfig.dispatcher = this;
    if (config.dispatcher.rdma_sync_depth) {
        wconfig.cq = create_rdma_cqx(config.dispatcher.rdma_sync_depth);
        if (!wconfig.cq) {
            PRINT_LOG("Couldn't create CQ %d\n", errno);
            return 1;
        }
        struct ibv_qp_ex *rdma_qpx =
            create_rdma_qpx(wconfig.cq, config.dispatcher.rdma_sync_depth);
        wconfig.qps.push_back(rdma_qpx);
    } else {
        wconfig.cq = nullptr;
        wconfig.cq_async = nullptr;
    }
    wconfig.pp_global = &pp_global;
    wconfig.page_mgmt = &page_mgmt;

    pp_pkt_local.init();
    rc = init_wcore(wconfig);
    if (rc) {
        return rc;
    }

    // adding pages
    size_t num_eth_pkt_pages = 0;

    size_t pkt_pool_size_total = 0;
    size_t page_pool_size_total = 0;

    for (auto &tp : config.mm.addrs) {
        char *start = (char *)tp.first;
        char *end = start + tp.second;
        while (start < end) {
            if (num_eth_pkt_pages < NUM_ETH_PKT_PAGES &&
                (end - start) >= ETH_BUFFER_SIZE) {
                pp_pkt_local.free_page_initial(start);
                ++pp_pkt_free;
                start += ETH_BUFFER_SIZE;
                ++num_eth_pkt_pages;
                pkt_pool_size_total += ETH_BUFFER_SIZE;
            } else {
                pp_local.free_page(start);
                start += NET_PAGE_SIZE;
                page_pool_size_total += NET_PAGE_SIZE;
            }
        }
    }

    PRINT_LOG("packet pool size: %lf GB\n",
              (double)pkt_pool_size_total / 1024 / 1024 / 1024);
    PRINT_LOG("page pool size: %lf GB\n",
              (double)page_pool_size_total / 1024 / 1024 / 1024);
    if (num_eth_pkt_pages != NUM_ETH_PKT_PAGES) {
        printf("Mismatch num_eth_pkt_pages(%ld) vs NUM_ETH_PKT_PAGES(%ld)\n",
               num_eth_pkt_pages, NUM_ETH_PKT_PAGES);
        return 1;
    }

    return rc;
}

int dispatcher_t::init_worker(const init_t &config) {
    int ret = 0;
    num_pinned_workers = 0;

    for (auto &core_id : config.worker.core_ids) {
        PRINT_LOG("adding worker(%d) %d\n", num_pinned_workers, core_id);

        ret = add_worker(config, num_pinned_workers, core_id);

        if (ret) {
            PRINT_LOG("fail to add worker(%d) %d code: %d\n",
                      num_pinned_workers, core_id, ret);
            return ret;
        }
        ++num_pinned_workers;
    }

    PRINT_LOG("preparing workers\n");

    for (worker_id_t worker_id = 0; worker_id < num_pinned_workers;
         ++worker_id) {
        auto &worker = workers[worker_id];

#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
        PRINT_LOG("Preparing eth tx cq\n");
        struct ibv_cq_init_attr_ex cq_init_attr_ex;
        memset(&cq_init_attr_ex, 0, sizeof(cq_init_attr_ex));
        cq_init_attr_ex.cqe = worker_t::NUM_ETH_TX_DESC;
        cq_init_attr_ex.wc_flags = 0;
        cq_init_attr_ex.flags = IBV_CREATE_CQ_ATTR_SINGLE_THREADED;
        struct ibv_cq_ex *eth_cq = ibv_create_cq_ex(eth_ctx, &cq_init_attr_ex);

#else
        struct ibv_cq_ex *eth_cq = nullptr;
#endif

        PRINT_LOG("Preparing eth tx\n");
        struct ibv_qp_cap cap;
        cap.max_inline_data = 128;
        cap.max_send_sge = 1;
        cap.max_send_wr = worker_t::NUM_ETH_TX_DESC;

        cap.max_recv_sge = 0;
        cap.max_recv_wr = 0;

#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
        struct ibv_qp *tx_qp_rtr = raweth::create_qp_raw_rtr(
            eth_ctx, eth_port_num, eth_pd, eth_cq, cap, 1);
#else
        struct ibv_qp *tx_qp_rtr = raweth::create_qp_raw_rtr(
            eth_ctx, eth_port_num, eth_pd, central_cq_eth, cap, 1);

#endif
        if (tx_qp_rtr == nullptr) {
            PRINT_LOG("fail to create tx_qp_rtr\n");
            return ret;
        }
        ret = worker.init_eth(tx_qp_rtr, eth_cq);
        if (ret) {
            PRINT_LOG("fail to init eth for worker code: %d\n", ret);
            return ret;
        }
    }

    return 0;
}
int dispatcher_t::start_workers(const job_adapter_t::init_t &app_config) {
    int ret = 0;
    for (worker_id_t worker_id = 0; worker_id < num_pinned_workers;
         ++worker_id) {
        auto &worker = workers[worker_id];
        ret = worker.start(app_config);

        if (ret) {
            PRINT_LOG("fail to start worker code: %d\n", ret);
            return ret;
        }
    }

    return 0;
}

#ifdef DRIVER_OSV

int dispatcher_t::init_reclaimer(const init_t &config) {
    if (config.mem_server.disable) {
        return 0;
    }
    reclaimer_t::init_t rconfig;

    struct ibv_cq_ex *reclaim_cq = create_rdma_cqx(max_evict);
    if (!reclaim_cq) {
        PRINT_LOG("Couldn't create CQ %d\n", errno);
        return 1;
    }

    struct ibv_qp_ex *reclaim_qpx = create_rdma_qpx(reclaim_cq, max_evict);
    if (!reclaim_qpx) {
        PRINT_LOG("Couldn't create QPX %d\n", errno);
        return 1;
    }

    rconfig.wcore.dispatcher = this;
    rconfig.wcore.cq = reclaim_cq;
    rconfig.wcore.cq_async = nullptr;
    rconfig.wcore.qps.push_back(reclaim_qpx);
    rconfig.wcore.pp_global = &pp_global;
    rconfig.wcore.page_mgmt = &page_mgmt;

    rconfig.reclaimer.core_id = config.reclaimer.core_id;
    rconfig.reclaimer.start_percent = config.reclaimer.start_percent;
    rconfig.reclaimer.end_percent = config.reclaimer.end_percent;

    int ret = reclaimer.init(rconfig);
    if (ret) {
        return ret;
    }

    return 0;
}
int dispatcher_t::start_reclaimer() {
    PRINT_LOG("starting reclaimer\n");
    int ret = reclaimer.start();
    return ret;
}

#endif

int dispatcher_t::post_eth_recvs(int num) {
    int ret = 0;
    struct ibv_recv_wr *bad_wr;
    int i = 0;
    for (; i < num; ++i) {
        void *page = pp_pkt_local.alloc_page();
        if (gcc_unlikely(!page)) {
            break;
        }
        --pp_pkt_free;
        --eth_rx_count;
        eth_recv_wrs[i].next = &eth_recv_wrs[i + 1];
        eth_recv_wrs[i].wr_id = (uint64_t)page;
        eth_recv_sges[i].addr = (uint64_t)page;
        eth_recv_sges[i].lkey = mm.find_mr(page, NET_PAGE_SIZE).eth_lkey;
    }
    if (gcc_likely(i > 0)) {
        eth_recv_wrs[i - 1].next = NULL;
        ret = ibv_post_recv(eth_rx_qp, eth_recv_wrs.data(), &bad_wr);
        ++pp_pkt_posted;
    }
    return ret;
}

worker_t &dispatcher_t::worker(worker_id_t worker_id) {
    return workers[worker_id];
}

worker_t *dispatcher_t::worker_checked_by_core_id(int lu_core_id) {
    for (int i = 0; i < num_pinned_workers; ++i) {
        if (workers[i].core_id == lu_core_id) {
            return &workers[i];
        }
    }

    return nullptr;
}

worker_core_t *dispatcher_t::worker_core_checked_by_core_id(int lu_core_id) {
    for (int i = 0; i < num_pinned_workers; ++i) {
        if (workers[i].core_id == lu_core_id) {
            return static_cast<worker_core_t *>(&workers[i]);
        }
    }

    if (core_id == lu_core_id) {
        return static_cast<worker_core_t *>(this);
    }

    return nullptr;
}

#if 0
int dispatcher_t::process_cq_n(struct ibv_cq_ex *cq_to_process, size_t num) {
    int ret;
    struct ibv_poll_cq_attr attr = {};
    ret = ibv_start_poll(cq_to_process, &attr);

    if (ret == ENOENT) {
        return 0;
    } else if (gcc_unlikely(ret > 0)) {
        return -ret;
    }

    int polled = 0;
    do {
        ++polled;
        auto opcode = ibv_wc_read_opcode(cq_to_process);
        switch (opcode) {
            case IBV_WC_RECV:
                --pp_pkt_posted;
                handle_eth_rx((void *)cq_to_process->wr_id);
                ++eth_rx_count;
                break;
            case IBV_WC_SEND:
                pp_pkt_local.free_page((void *)cq_to_process->wr_id);
                ++pp_pkt_free;
                break;
            case IBV_WC_RDMA_READ:
                DP_TRACE(trace_reenqueue,
                         ((job_t *)cq_to_process->wr_id)->wr_id());
                enqueue_job((job_t *)cq_to_process->wr_id);
                break;
            default:
                PRINT_LOG("unknown opcode: %d\n", opcode);
                break;
        }
        ret = ibv_next_poll(cq_to_process);
    } while (ret != ENOENT && polled < num);

    ibv_end_poll(cq_to_process);

    if (pp_pkt_posted < NUM_ETH_RX_DESC) {
        size_t to_post = NUM_ETH_RX_DESC - pp_pkt_posted;
        if (pp_pkt_free >= to_post)
            post_eth_recvs(to_post);
        else if (pp_pkt_free)
            post_eth_recvs(pp_pkt_free);
    }

    return polled;
}

#endif

int dispatcher_t::process_cq_all(struct ibv_cq_ex *cq_to_process) {
    int ret;
    struct ibv_poll_cq_attr attr = {};
    ret = ibv_start_poll(cq_to_process, &attr);

    if (ret == ENOENT) {
        return 0;
    } else if (gcc_unlikely(ret > 0)) {
        return -ret;
    }

    int polled = 0;
    do {
        ++polled;
        auto opcode = ibv_wc_read_opcode(cq_to_process);

        switch (opcode) {
            case IBV_WC_RECV:
                handle_eth_rx((void *)cq_to_process->wr_id);
                ++eth_rx_count;
                break;
            case IBV_WC_SEND:
                pp_pkt_local.free_page((void *)cq_to_process->wr_id);
                ++pp_pkt_free;
                break;
            case IBV_WC_RDMA_READ:
                DP_TRACE(trace_reenqueue,
                         ((job_t *)cq_to_process->wr_id)->wr_id());
                enqueue_job((job_t *)cq_to_process->wr_id);
                break;
            default:
                PRINT_LOG("unknown opcode: %d\n", opcode);
                break;
        }
        ret = ibv_next_poll(cq_to_process);
    } while (ret != ENOENT);

    ibv_end_poll(cq_to_process);

    if (eth_rx_count) {
        ret = post_eth_recvs(eth_rx_count);
        if (gcc_unlikely(ret != 0)) {
            printf("post eth recv error\n");
            abort();
        }
    }
    return polled;
}
inline void dispatcher_t::enqueue_job(job_t *job) {
    DP_TRACE(trace_enqueue, job->wr_id());
#ifdef PROTOCOL_BREAKDOWN
    cmdpkt_t *pkt = job->to_pkt();
    pkt->breakdown.queued = -get_cycles();
    pkt->breakdown.num_pending += num_job_pending.load(atomic_on_load);
    pkt->breakdown.num_enqueue += 1;
    pkt->breakdown.prev_wr_id = last_job_wr_id;
    last_job_wr_id = pkt->measure.lgen_wr_id;
#endif

    job_ready.push_back(job);

    // below is faster than fetch_add, and okay.
    // this is modified by dispatcher only
    uint64_t num_job_pending_prev = num_job_pending.load(atomic_on_load);

    num_job_pending.store(num_job_pending_prev + 1, atomic_on_store);
}
inline void dispatcher_t::enqueue_jobs(job_list_t *jobs, uint64_t count) {
    job_ready.push_back_all(jobs);
    uint64_t num_job_pending_prev = num_job_pending.load(atomic_on_load);
    num_job_pending.store(num_job_pending_prev + count, atomic_on_store);
}
template <bool use_yield_local>
void dispatcher_t::dispatch_job(worker_id_t worker_id) {
    if constexpr (use_yield_local) {
        job_t *job_local = job_ready_local[worker_id].pop();
        if (job_local != nullptr) {
            // handle local first
            DP_TRACE(trace_dequeue, job_local->wr_id(), worker_id);
            workers[worker_id].dispatched_job = job_local;  // dispatch
            workers[worker_id].state.store(worker_t::STATE_RUNNING,
                                           atomic_on_store);
            // below is faster than fetch_add, and okay.
            // this is modified by dispatcher only
            uint64_t num_job_pending_prev =
                num_job_pending.load(atomic_on_load);
            num_job_pending.store(num_job_pending_prev - 1, atomic_on_store);
            return;
        }
    }

    job_t *job = job_ready.pop();

    if (gcc_unlikely(job == nullptr)) {
        // no job
        return;
    }

    if constexpr (use_yield_local) {
        while (job->state < MAX_WORKERS && job->state != worker_id) {
            job_ready_local[job->state].push_back(job);
            job = job_ready.pop();
            if (gcc_unlikely(job == nullptr)) {
                // no job
                return;
            }
        }
    }

    DP_TRACE(trace_dequeue, job->wr_id(), worker_id);
    // PRINT_DEBUG_LOG("dispatch task to worker[%d]\n", worker_id);
    workers[worker_id].dispatched_job = job;  // dispatch
    workers[worker_id].state.store(worker_t::STATE_RUNNING, atomic_on_store);

    // below is faster than fetch_add, and okay.
    // this is modified by dispatcher only
    uint64_t num_job_pending_prev = num_job_pending.load(atomic_on_load);
    num_job_pending.store(num_job_pending_prev - 1, atomic_on_store);
}

void dispatcher_t::handle_eth_rx(void *page) {
    cmdpkt_t *pkt = (cmdpkt_t *)page;

    job_t *job_new = job_t::from_pkt(pkt);

    DP_TRACE(trace_newjob, job_new->wr_id());
    enqueue_job(job_new);
}

void dispatcher_t::handle_eth_rx_in_dp(void *page) {
    // cmdpkt_t *pkt = (cmdpkt_t *)page;

    // pkt->eth.dst = pkt->eth.src;
    // pkt->eth.src = net_config.mac_addr;
    // pkt->eth.ether_type = eth_type;

    // ibv_wr_start(eth_tx_qp);
    // eth_tx_qp->wr_id = (uint64_t)page;
    // eth_tx_qp->wr_flags = 0;  // todo: figure out IBV_SEND_INLINE
    // ibv_wr_send(eth_tx_qp);
    // auto &mr = mm.find_mr(page, 128);
    // ibv_wr_set_sge(eth_tx_qp, mr.lkey, (uint64_t)page, 128);

    // int ret = ibv_wr_complete(eth_tx_qp);

    // if (gcc_unlikely(ret)) {
    //     printf("err: %d\n", ret);
    // }
}

void dispatcher_t::check_workers() {
    worker_id_t worker_id = last_worker_id_start;
    do {
        uint8_t state = workers[worker_id].state.load(atomic_on_load);

        switch (state) {
            case worker_t::STATE_READY:
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
                free_jobs(&workers[worker_id].free_job_list);
#endif

                dispatch_job(worker_id);
                break;
            case worker_t::STATE_RUNNING:
                // skip
                break;
            case worker_t::STATE_YIELD_GLOBAL:
                // requeue
                DP_TRACE(trace_reenqueue,
                         workers[worker_id].dispatched_job->wr_id());
                enqueue_jobs(&workers[worker_id].yield_global_job_list,
                             workers[worker_id].yield_global_count);

                workers[worker_id].state.store(worker_t::STATE_READY,
                                               atomic_on_store);
                break;
            default:
                PRINT_LOG("Unknown worker[%d]'s state: %d\n", worker_id, state);
                break;
        }
        worker_id = (worker_id + 1) % num_pinned_workers;
    } while (worker_id != last_worker_id_start);
    last_worker_id_start = (last_worker_id_start + 1) % num_pinned_workers;
}

void dispatcher_t::check_workers_smart() {
    worker_id_t num_ready = 0;
    std::array<worker_id_t, MAX_WORKERS> idx;
    std::array<size_t, MAX_WORKERS> rdma_async_issued;

    for (size_t worker_id = 0; worker_id < num_pinned_workers; ++worker_id) {
        uint8_t state = workers[worker_id].state.load(atomic_on_load);
        switch (state) {
            case worker_t::STATE_READY:
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
                free_jobs(&workers[worker_id].free_job_list);
#endif
                idx[num_ready] = worker_id;
                rdma_async_issued[worker_id] =
                    workers[worker_id].rdma_async_issued;
                ++num_ready;
                break;
            case worker_t::STATE_RUNNING:
                // skip
                break;
            case worker_t::STATE_YIELD_GLOBAL:
                // requeue
                DP_TRACE(trace_reenqueue,
                         workers[worker_id].dispatched_job->wr_id());
                enqueue_jobs(&workers[worker_id].yield_global_job_list,
                             workers[worker_id].yield_global_count);

                idx[num_ready] = worker_id;
                rdma_async_issued[num_ready] =
                    workers[worker_id].rdma_async_issued;
                ++num_ready;

                workers[worker_id].state.store(worker_t::STATE_READY,
                                               atomic_on_store);
                break;
            default:
                PRINT_LOG("Unknown worker[%d]'s state: %d\n", worker_id, state);
                break;
        }
    }

    std::sort(idx.begin(), idx.begin() + num_ready,
              [&rdma_async_issued](size_t i1, size_t i2) {
                  return rdma_async_issued[i1] < rdma_async_issued[i2];
              });

    for (size_t i = 0; i < num_ready; ++i) {
        dispatch_job(idx[i]);
    }
}

template <bool no_ipi>
void dispatcher_t::check_workers_preempt(uint64_t *dispatch_time) {
    uint64_t cycle;
    for (worker_id_t worker_id = 0; worker_id < num_pinned_workers;
         ++worker_id) {
        uint8_t state = workers[worker_id].state.load(atomic_on_load);

        switch (state) {
            case worker_t::STATE_READY:
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
                free_jobs(&workers[worker_id].free_job_list);
#endif
                workers[worker_id].preempt_requested.store(false,
                                                           atomic_on_store);
                dispatch_job<true>(worker_id);
                dispatch_time[worker_id] = get_cycles();
                break;
            case worker_t::STATE_RUNNING:
                cycle = get_cycles();

                if (dispatch_time[worker_id] &&
                    (cycle - dispatch_time[worker_id]) > preemption_cycles) {
                    if constexpr (no_ipi) {
                        workers[worker_id].preempt_requested.store(
                            true, atomic_on_store);
                    } else {
                        workers[worker_id].send_preempt();
                    }
                    dispatch_time[worker_id] = 0;  // prevent duplicated ipis
                }

                break;
            case worker_t::STATE_YIELD_GLOBAL:
                // requeue
                DP_TRACE(trace_reenqueue,
                         workers[worker_id].dispatched_job->wr_id());
                enqueue_jobs(&workers[worker_id].yield_global_job_list,
                             workers[worker_id].yield_global_count);

                workers[worker_id].state.store(worker_t::STATE_READY,
                                               atomic_on_store);
                break;
            default:
                PRINT_LOG("Unknown worker[%d]'s state: %d\n", worker_id, state);
                break;
        }
    }
}
struct ibv_cq_ex *dispatcher_t::create_rdma_cqx(size_t depth) {
    struct ibv_cq_init_attr_ex cq_init_attr_ex;
    memset(&cq_init_attr_ex, 0, sizeof(cq_init_attr_ex));
    cq_init_attr_ex.cqe = depth;
    cq_init_attr_ex.wc_flags = 0;
    cq_init_attr_ex.flags = IBV_CREATE_CQ_ATTR_SINGLE_THREADED;
    return ibv_create_cq_ex(ib_ctx, &cq_init_attr_ex);
}
struct ibv_qp_ex *dispatcher_t::create_rdma_qpx(struct ibv_cq_ex *r_cq,
                                                size_t depth) {
    struct ibv_qp_cap cap;
    memset(&cap, 0, sizeof(cap));
    int ret = 0;

    cap.max_inline_data = 0;
    cap.max_recv_wr = 0;
    cap.max_recv_sge = 0;

    cap.max_send_wr = depth;
    cap.max_send_sge = 1;

    struct ibv_qp *qp =
        rdma.create_qp_init(ib_ctx, rdma_port_num, ib_pd, r_cq, cap, 1);
    if (!qp) {
        PRINT_LOG("Couldn't create rdma qp %d\n", errno);
        return nullptr;
    }
    uint32_t server_rdma_qp_num = rdma.new_qp();

    if (!server_rdma_qp_num) {
        PRINT_LOG("Couldn't create remote rdma qp %d\n", errno);
        return nullptr;
    }

    ret = rdma.modify_qp_rtr(qp, server_rdma_qp_num, rdma_port_num,
                             rdma_sgid_idx);
    if (ret) {
        PRINT_LOG("Couldn't modify rdma qp rtr\n");
        return nullptr;
    }

    ret = rdma.modify_qp_rts(qp);
    if (ret) {
        PRINT_LOG("Couldn't modify rdma qp rts\n");
        return nullptr;
    }
    ret = rdma.connect_qp(rdma_port_attr.lid, rdma_gid.raw, qp->qp_num,
                          server_rdma_qp_num);
    if (ret) {
        PRINT_LOG("Failed to connect QP\n");
        return nullptr;
    }
    struct ibv_qp_ex *qpx = ibv_qp_to_qp_ex(qp);

    return qpx;
}

int dispatcher_t::add_worker(const init_t &dconfig, worker_id_t worker_id,
                             int core_id) {
    int ret = 0;

    size_t sync_depth = pf_mode == PF_MODE_SYNC || pf_mode == PF_MODE_HYBRID ||
                                pf_mode == PF_MODE_SYNC_SJK ||
                                pf_mode == PF_MODE_SYNC_SJK2
                            ? dconfig.worker.rdma_sync_depth
                            : 0;

    size_t async_depth = pf_mode == PF_MODE_ASYNC ||
                                 pf_mode == PF_MODE_HYBRID ||
                                 pf_mode == PF_MODE_ASYNC_LOCAL
                             ? dconfig.worker.rdma_async_depth
                             : 0;

    worker_t::init_t config;
    config.wcore.dispatcher = this;
    PRINT_LOG("Preparing RDMA qp\n");
    if (sync_depth) {
        config.wcore.cq = create_rdma_cqx(sync_depth);
        if (!config.wcore.cq) {
            PRINT_LOG("Couldn't create CQ %d\n", errno);
            return 1;
        }
        struct ibv_qp_ex *rdma_qpx =
            create_rdma_qpx(config.wcore.cq, sync_depth);
        if (!rdma_qpx) {
            PRINT_LOG("Couldn't create QPX %d\n", errno);
            return 1;
        }
        config.wcore.qps.push_back(rdma_qpx);
    } else {
        config.wcore.cq = nullptr;
        config.wcore.qps.push_back(nullptr);
    }
    if (async_depth) {
        config.wcore.cq_async = nullptr;

        struct ibv_qp_ex *rdma_qpx = nullptr;
        if (pf_mode == PF_MODE_ASYNC_LOCAL) {
            config.wcore.cq_async = create_rdma_cqx(async_depth);
            if (!config.wcore.cq_async) {
                PRINT_LOG("Couldn't create CQ %d\n", errno);
                return 1;
            }
            rdma_qpx = create_rdma_qpx(config.wcore.cq_async, async_depth);
        } else {
            config.wcore.cq_async = nullptr;
            if (dp_mode == DP_MODE_MULTI) {
                rdma_qpx = create_rdma_qpx(central_cq_rdma, async_depth);
            } else {
                rdma_qpx = create_rdma_qpx(central_cq_eth, async_depth);
            }
        }
        if (!rdma_qpx) {
            PRINT_LOG("Couldn't create QPX %d\n", errno);
            return 1;
        }
        config.wcore.qps.push_back(rdma_qpx);
    } else {
        config.wcore.cq_async = nullptr;
        config.wcore.qps.push_back(nullptr);
    }
    if (ret) return ret;
    config.rdma_async_depth = async_depth;
    config.worker.worker_id = worker_id;
    config.worker.core_id = core_id;
    config.pf_mode = pf_mode;
    config.wcore.pp_global = &pp_global;
    config.wcore.page_mgmt = &page_mgmt;
    memcpy(&config.eth.config, &dconfig.eth.config, sizeof(config.eth.config));

    PRINT_LOG("calling worker(%d) init\n", worker_id);
    return workers[worker_id].init(config);
}

template <bool do_smart>
int dispatcher_t::run_multi_cq() {
    int ret = 0;

    PRINT_LOG("Running Main Loop (run_multi_cq)\n");
    while (gcc_likely(ret >= 0)) {
        if constexpr (do_smart) {
            check_workers_smart();
        } else {
            check_workers();
        }
        ret = process_cq_all(central_cq_eth);
        ret += process_cq_all(central_cq_rdma);
    }
    abort();
    PRINT_LOG("Terminating Main Loop\n");
    return ret;
}

template <bool do_smart>
int dispatcher_t::run_single_cq() {
    int ret = 0;

    PRINT_LOG("Running Main Loop (run_single_cq)\n");
    while (gcc_likely(ret >= 0)) {
        if constexpr (do_smart) {
            check_workers_smart();
        } else {
            check_workers();
        }
        ret = process_cq_all(central_cq_eth);  // use eth for both RDMA/ETH
    }
    PRINT_LOG("!!!\n");
    abort();
    PRINT_LOG("Terminating Main Loop\n");
    return ret;
}

template <bool no_ipi>
int dispatcher_t::run_single_cq_preempt() {
    int ret = 0;
    uint64_t dispatch_time[num_pinned_workers];
    PRINT_LOG("Running Main Loop (run_single_cq_preempt)\n");
    while (gcc_likely(ret >= 0)) {
        check_workers_preempt<no_ipi>(dispatch_time);
        ret = process_cq_all(central_cq_eth);  // use eth for both RDMA/ETH
    }
    abort();
    PRINT_LOG("Terminating Main Loop\n");
    return ret;
}

int dispatcher_t::run() {
    // disable_preemption();
    if (preemption_cycles) {
        if (pf_mode != PF_MODE_SYNC_SJK && pf_mode != PF_MODE_SYNC_SJK2 &&
            pf_mode != PF_MODE_SYNC_SJK2_NONE) {
            printf(
                "preemption cycles is only for PF_MODE_SYNC_SJK(%lld or "
                "%lld)\n",
                PF_MODE_SYNC_SJK, PF_MODE_SYNC_SJK2);
            return 1;
        }
        switch (pf_mode) {
            case PF_MODE_SYNC_SJK:
                return run_single_cq_preempt<false>();
            case PF_MODE_SYNC_SJK2:
            case PF_MODE_SYNC_SJK2_NONE:
                return run_single_cq_preempt<true>();
            default:
                return -1;
        }
    }

    switch (dp_mode) {
        case DP_MODE_SINGLE:
            if (pf_mode == PF_MODE_ASYNC_LOCAL)
                if (sched_mode == 1) {
                    return run_single_cq<false>();
                } else {
                    return run_single_cq<true>();
                }
            else
                return run_single_cq<false>();
        case DP_MODE_MULTI:
            if (pf_mode == PF_MODE_ASYNC_LOCAL) {
                if (sched_mode == 1) {
                    return run_multi_cq<false>();
                } else {
                    return run_multi_cq<true>();
                }
            } else {
                return run_multi_cq<false>();
            }
        default:
            return -1;
    }
}

dispatcher_t default_dispatcher;
}  // namespace dispatcher
