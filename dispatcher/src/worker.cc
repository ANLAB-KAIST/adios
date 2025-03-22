#include <dispatcher/ctx.h>
#include <emmintrin.h>

#include <config/static-config.hh>
#include <dispatcher/api.hh>
#include <dispatcher/dispatcher.hh>
#include <dispatcher/raw-eth.hh>
#include <dispatcher/rdma.hh>
#include <dispatcher/traces.hh>
#include <dispatcher/worker.hh>

#if defined(DILOS_SWAPCONTEXT_NANO)
#define swapcontext_w_to_a(X, Y) swapcontext_nano(X, Y)  // worker to app
#define swapcontext_a_to_w(X, Y) swapcontext_nano(X, Y)  // app to worker
#define do_getcontext(X) getcontext_nano(X)
#define do_setcontext_to_w(X) \
    setcontext_nano(X)  // exit app and return to worker

#elif defined(DILOS_SWAPCONTEXT_FASTER)
#define swapcontext_w_to_a(X, Y) \
    swapcontext_fast_no_sv_fp(X, Y)                           // worker to app
#define swapcontext_a_to_w(X, Y) swapcontext_very_fast(X, Y)  // app to worker
#define do_getcontext(X) getcontext_fast(X)
#define do_setcontext_to_w(X) \
    setcontext_fast_no_rs_fp(X)  // exit app and return to worker

#else

#define swapcontext_w_to_a(X, Y) swapcontext_fast(X, Y)  // worker to app
#define swapcontext_a_to_w(X, Y) swapcontext_fast(X, Y)  // app to worker
#define do_getcontext(X) getcontext_fast(X)
#define do_setcontext_to_w(X) \
    setcontext_fast(X)  // exit app and return to worker
#endif

namespace dispatcher {

static thread_local worker_t *current_worker = nullptr;

static int worker_main_outer(worker_t &worker,
                             const job_adapter_t::init_t &app_config) {
    job_adapter_t::init_per_worker(worker.worker_id, app_config);
    return worker.worker_main_aio<dispatcher_t::PF_MODE_SYNC>();
}
static int worker_main_outer_async_local(
    worker_t &worker, const job_adapter_t::init_t &app_config) {
    job_adapter_t::init_per_worker(worker.worker_id, app_config);
    return worker.worker_main_aio<dispatcher_t::PF_MODE_ASYNC_LOCAL>();
}
static int worker_main_outer_sjk(worker_t &worker,
                                 const job_adapter_t::init_t &app_config) {
    job_adapter_t::init_per_worker(worker.worker_id, app_config);
    return worker.worker_main_aio<dispatcher_t::PF_MODE_SYNC_SJK>();
}

int worker_t::init(const init_t &config) {
    SET_MODULE_NAME_ID("worker", config.worker.worker_id);

    state.store(worker_t::STATE_UNUSED, atomic_on_store);
    memcpy(&net_config, &config.eth.config, sizeof(net_config));
    int ret = 0;
    pf_mode = config.pf_mode;
    PRINT_LOG("Initing Worker\n");
    ret = init_worker(config);
    if (ret) return ret;
    PRINT_LOG("Initing Worker Core\n");
    ret = init_wcore(config.wcore);
    if (ret) return ret;
    rdma_async_depth = config.rdma_async_depth;
    rdma_async_issued = 0;
    yield_count = 0;
#ifndef DILOS_SYNC_ALLOC
    async_wait_alloc_list.reset();
#endif
    return ret;
}

int worker_t::init_eth(struct ibv_qp *tx_qp_rtr, struct ibv_cq_ex *tx_qp) {
    PRINT_LOG("Initing ETH\n");
    int ret = raweth::modify_qp_raw_rts(tx_qp_rtr);
    if (ret < 0) {
        PRINT_LOG("failed modify qp to send\n");
        return 1;
    }

    eth_tx_cq = tx_qp;
    eth_tx_qp = ibv_qp_to_qp_ex(tx_qp_rtr);

    return 0;
}
int worker_core_t::init_wcore(const worker_core_init_t &config) {
    dispatcher = config.dispatcher;
    rdma_cq = config.cq;
    rdma_cq_async = config.cq_async;

#ifdef DILOS_RDMA_PERF

    static size_t next_worker_id = 0;
    rdma_count = 0;
    rdma_last_print = 0;
    my_worker_id = next_worker_id;
    ++next_worker_id;
#endif

    for (auto &qp : rdma_qps) {
        qp = nullptr;
    }

    if (MAX_RDMA_QP < config.qps.size()) {
        return 1;
    }

    int i = 0;

    for (auto &qp : config.qps) {
        rdma_qps[i] = qp;
        ++i;
    }

    pp_global = config.pp_global;

    int ret = 0;
    ret = pp_local.init(config.pp_global);
    if (ret) {
        return ret;
    }
    ret = page_mgmt_local.init(config.page_mgmt);

    return ret;
}
int worker_t::init_worker(const init_t &config) {
    PRINT_LOG("core_id: %d\n", config.worker.core_id);
    state.store(STATE_UNINIT, atomic_on_store);
    worker_id = config.worker.worker_id;
    core_id = config.worker.core_id;
    dispatched_job = nullptr;
    job = nullptr;
    preempt_requested.store(false);
    return 0;
}

int worker_t::send_tx_eth_one(eth_addr_t dst, void *page, size_t len) {
    ethhdr_t *eth_pkt = (ethhdr_t *)page;

    // setting mac header start
    eth_pkt->dst = dst;

    eth_pkt->src = net_config.mac_addr;
    eth_pkt->ether_type = ETTYPE_IPV4;

    // setting mac header end
    // triggering raw packet send

    ibv_wr_start(eth_tx_qp);

    eth_tx_qp->wr_id = (uint64_t)page;
    eth_tx_qp->wr_flags = 0;  // todo: figure out IBV_SEND_INLINE
    ibv_wr_send(eth_tx_qp);

    auto &mr = dispatcher->mm.find_mr(page, len);
    ibv_wr_set_sge(eth_tx_qp, mr.eth_lkey, (uint64_t)page, len);

    return ibv_wr_complete(eth_tx_qp);
}

int worker_t::poll_eth_cq_async() {
    int ret;
    struct ibv_poll_cq_attr attr = {};
    ret = ibv_start_poll(eth_tx_cq, &attr);

    if (ret == ENOENT) {
        return 0;
    } else if (gcc_unlikely(ret > 0)) {
        return -ret;
    }
    int polled = 0;
    do {
        auto opcode = ibv_wc_read_opcode(eth_tx_cq);

        switch (opcode) {
            case IBV_WC_SEND:
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
                free_job_list_local.push_back((job_t *)eth_tx_cq->wr_id);
#endif
                break;
            default:
                PRINT_LOG("unknown opcode: %d\n", opcode);
                break;
        }
        ++polled;
        ret = ibv_next_poll(eth_tx_cq);
    } while (ret != ENOENT);

    ibv_end_poll(eth_tx_cq);
    return polled;
}

int worker_t::send_tx_eth_one_sync(eth_addr_t dst, void *page, size_t len) {
    int ret = send_tx_eth_one(dst, page, len);
    if (ret) {
        return ret;
    }

    // auto before = get_cycles();
    do {
        ret = poll_eth_cq_async();
    } while (ret == 0);

    // auto after = get_cycles();

    // debug_early_u64("poll: ", after - before);

    if (ret < 0) {
        return ret;
    }
    return 0;
}

void worker_t::prepare_main() {
    current_worker = this;
    std::unique_lock<std::mutex> lck(mtx);
    PRINT_LOG("thread starting... \n");

    if (core_id >= 0) {
        // 1. thread affinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        int rc =
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        (void)rc;
    }

    int ret = change_pf_mode(pf_mode);
    PRINT_LOG("change_pf_mode to %d: %d\n", pf_mode, ret);

    apic_id = get_apic_id();
    PRINT_LOG("apic_id: %u\n", apic_id);

    PRINT_LOG("dispatcher: %p\n", dispatcher);
    PRINT_LOG("rdma_cq: %p\n", rdma_cq);
    PRINT_LOG("rdma_cq_async: %p\n", rdma_cq_async);
    for (int i = 0; i < rdma_qps.size(); ++i) {
        PRINT_LOG("rdma_qps[%d]: %p\n", i, rdma_qps[i]);
    }

    PRINT_LOG("eth_tx_cq: %p\n", eth_tx_cq);
    PRINT_LOG("eth_tx_qp: %p\n", eth_tx_qp);

    state.store(STATE_READY, atomic_on_store);
    cv.notify_all();
}
extern "C" void debug_early(const char *msg);
extern "C" void debug_early_u64(const char *msg, unsigned long long val);

template <int pf_mode>
uint8_t worker_t::handle_job() {
    int ret = 0;
#ifdef PROTOCOL_BREAKDOWN
    uint64_t worker_cycle_tmp = 0;
    cmdpkt_t *cmdpkt = nullptr;
#endif

#ifdef PROTOCOL_BREAKDOWN
    num_pf = 0;
    major_pf = 0;
    sum_pf = 0;
    polling_pf = 0;
#endif
    // DO IP/UDP (NOT IMPLEMENTED)

    switch (job->state) {
        case job_t::STATE_NEW:
            // debug_early_u64("new ", (uint64_t)job);
            job->state = worker_id;
#ifdef DILOS_CHECK_GUARD
            job->ctx()->set_guard();
#endif

#ifdef PROTOCOL_BREAKDOWN
            worker_cycle_tmp = get_cycles();
            cmdpkt = job->to_pkt();
            cmdpkt->breakdown.queued += worker_cycle_tmp;
            cmdpkt->breakdown.worker -= worker_cycle_tmp;
#endif

            DP_TRACE(trace_start, job->wr_id());
            if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK) {
                ret = run_job_ctx_sjk();
            } else {
                ret = run_job_ctx();
            }
            break;
        case job_t::STATE_NONE:
            abort();
            break;
        default:
#ifdef PROTOCOL_BREAKDOWN
            worker_cycle_tmp = get_cycles();
            cmdpkt = job->to_pkt();
            cmdpkt->breakdown.worker -= worker_cycle_tmp;
#endif
            // state is worker_id
            DP_TRACE(trace_resume, job->wr_id());
            if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK) {
                disable_preemption();
                ret = resume_job_ctx();
            } else {
                ret = resume_job_ctx();
            }
            break;
    }

#ifdef PROTOCOL_BREAKDOWN
    cmdpkt = job->to_pkt();
    cmdpkt->breakdown.num_pf += num_pf;
    cmdpkt->breakdown.major_pf += major_pf;
    cmdpkt->breakdown.sum_pf += sum_pf;
    cmdpkt->breakdown.polling_pf += polling_pf;
#endif

    if (ret >= 0) {
        DP_TRACE(trace_exit, job->wr_id());

#ifdef DILOS_CHECK_GUARD
        job->ctx()->check_guard(11);
#endif

#ifdef PROTOCOL_BREAKDOWN
        cmdpkt->breakdown.worker += get_cycles();
        cmdpkt->breakdown.worker_id = worker_id;
#endif
        eth_addr_t dst = job->src_mac;
        job->state = job_t::STATE_NONE;

#if defined(DILOS_ND_ETH_SYNC)
        ret = send_tx_eth_one_sync(dst, job, sizeof(cmdpkt_t) + ret);
#else
        ret = send_tx_eth_one(dst, job, sizeof(cmdpkt_t) + ret);
#endif
        // !IMPORTANT DO NOT MODIFIY JOB FROM NOW

        if (gcc_unlikely(ret)) {
            printf("[coreid: %d]ret not zero: %d\n", core_id, ret);
        }

        DP_TRACE(trace_exit, job->wr_id());
        if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK) {
            enable_preemption();
        }

        // debug_early_u64("done ", (uint64_t)job);
        // 3. Job done
        return STATE_READY;
    } else {
        int rdma_ret = 0;
        int poll_count = 0;
        switch (ret) {
            case JOB_RET_PF_ASYNC:
                job->state = worker_id;
#ifdef DILOS_CHECK_GUARD
                job->ctx()->check_guard(12);
#endif

                while (gcc_unlikely(rdma_async_issued == rdma_async_depth)) {
                    // consume rdma
                    // abort();
                    ++poll_count;
                    job_t *polled_jobs[rdma_async_depth];
                    int polled = 0;
                    polled = poll_cq_async(polled_jobs);
                    rdma_async_issued -= polled;
                    for (int i = 0; i < polled; ++i) {
                        // uint8_t next_state = handle_job<pf_mode>();
                        // if (gcc_unlikely(next_state != STATE_READY)) {
                        //     abort();
                        // }

                        ready_local.push_back(polled_jobs[i]);
                    }
                    // if (polled) {
                    //     debug_early_u64("poll_count: ", poll_count);
                    // }
                }

                rdma_ret =
                    fetch_rdma(worker_t::RDMA_ASYNC_QP, (uintptr_t)job,
                               async_pf_pa_or_ptep, async_pf_offset, 4096);
                if (gcc_unlikely(rdma_ret)) {
                    static std::mutex tmp_mtx;
                    std::unique_lock<std::mutex> tmp_lck(tmp_mtx);
                    debug_early_u64("rdma_async_issued: ", rdma_async_issued);
                    debug_early_u64("fetch rdma error: ", rdma_ret);
                    abort();
                }
                ++rdma_async_issued;
                // TODO: impl prefetching

                // !IMPORTANT DO NOT MODIFIY JOB FROM NOW
                // handle pf async, at this moment there is nothing to do in
                // dispather
                DP_TRACE(trace_asyncpf, job->wr_id(), (void *)job->ctx());

                return STATE_READY;

            case JOB_RET_YIELD:
                job->state = worker_id;
                // handle yield
                DP_TRACE(trace_yield, job->wr_id());

#ifdef DILOS_CHECK_GUARD
                job->ctx()->check_guard(13);
#endif
#ifdef PROTOCOL_BREAKDOWN
                cmdpkt->breakdown.worker += get_cycles();
                cmdpkt->breakdown.worker_id = worker_id;
#endif

                yield_job_list.push_back(job);
                yield_count += 1;

                if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK) {
                    enable_preemption();
                }

                return STATE_READY;

            case JOB_RET_YIELD_GLOBAL:
                job->state = worker_id;
                // handle yield
                DP_TRACE(trace_yield, job->wr_id());
#ifdef DILOS_CHECK_GUARD
                job->ctx()->check_guard(13);
#endif
#ifdef PROTOCOL_BREAKDOWN
                cmdpkt->breakdown.worker += get_cycles();
                cmdpkt->breakdown.worker_id = worker_id;
#endif
                yield_global_job_list.push_back(job);
                yield_global_count += 1;
                if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK) {
                    enable_preemption();
                }
                return STATE_YIELD_GLOBAL;

            case JOB_RET_WAIT:
                job->state = worker_id;

                if (!handle_wait(job, async_pf_pa_or_ptep)) {
                    // if job is not handled should resume, currently recursion
                    // todo: fix this
                    return handle_job<pf_mode>();
                }

                // do nothing; woke by other coroutine
                return STATE_READY;
#ifndef DILOS_SYNC_ALLOC
            case JOB_RET_ALLOC_ASYNC:
                job->state = worker_id;
                async_wait_alloc_list.push_back(job);
                // do nothing; woke by other coroutine
                return STATE_READY;
#endif
            default:
                printf("[coreid: %d] job's ret error: %d\n", core_id, ret);
                return STATE_ERROR;
        }
    }
}

template <int pf_mode>
int worker_t::worker_main_aio() {
    prepare_main();

    job_t *polled_jobs[rdma_async_depth];
    int polled = 0;

    bool new_job_dispatched = false;
    while (1) {
        if (state.load(std::memory_order_acquire) == STATE_RUNNING &&
            yield_count + rdma_async_issued < MAX_REQS_PER_WORKER) {
            job = dispatched_job;
#ifdef PROTOCOL_BREAKDOWN
            cmdpkt_t *cmdpkt = job->to_pkt();
            cmdpkt->breakdown.num_pending_local += polled;
#endif
            handle_job<pf_mode>();

            new_job_dispatched = true;
#if defined(DILOS_ND_ETH_ASYNC)
            poll_eth_cq_async();
#endif
#if defined(DILOS_ND_ETH_ASYNC) || defined(DILOS_ND_ETH_SYNC)
            if (!free_job_list_local.empty()) {
                free_job_list.push_back_all(&free_job_list_local);
                free_job_list_local.reset();
            }
#endif
        }

#ifndef DILOS_SYNC_ALLOC
        if (!async_wait_alloc_list.empty() && pp_global->count_bch() > 16) {
            // 16 is double of number of workers. heuristic
            ready_local.push_back_all(&async_wait_alloc_list);
            async_wait_alloc_list.reset();
        }
#endif

        if constexpr (pf_mode == dispatcher_t::PF_MODE_ASYNC_LOCAL) {
            polled = poll_cq_async(polled_jobs);
            for (int i = 0; i < polled; ++i) {
                job = polled_jobs[i];
                handle_job<pf_mode>();
            }
            rdma_async_issued -= polled;
        }

#if 0
        if (gcc_unlikely(!ready_local.empty())) {
            abort();
            do {
                job = ready_local.pop();
                handle_job<pf_mode>();
                

            } while (!ready_local.empty());
        }
#endif

        if (gcc_unlikely(yield_count)) {
            size_t yield_count_this = yield_count;
            for (size_t i = 0; i < yield_count_this; ++i) {
                job = yield_job_list.pop();
                --yield_count;
                handle_job<pf_mode>();
            }
        }

        if constexpr (pf_mode == dispatcher_t::PF_MODE_SYNC_SJK ||
                      pf_mode == dispatcher_t::PF_MODE_SYNC_SJK2) {
            if (yield_global_count > 0) {
                state.store(STATE_YIELD_GLOBAL, atomic_on_store);

                while (state.load(std::memory_order_acquire) !=
                       STATE_YIELD_GLOBAL) {
                    _mm_pause();
                }
                yield_global_count = 0;

                continue;
            }
        }

        if (new_job_dispatched) {
            new_job_dispatched = false;
            state.store(STATE_READY, atomic_on_store);
        }
    }
    PRINT_LOG("thread exit\n");
    return 0;
}

int worker_t::run_job() { return job->run(); }
int worker_t::run_job_ctx() {
    context_inner_t *jb_ctx = job->uctx();
    int ret = do_getcontext(jb_ctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
#ifdef DILOS_SWAPCONTEXT_NANO
    makecontext_nano(jb_ctx, job->ctx()->stack(), context_t::stack_size(),
                     (void (*)())job_t::do_run, job);

#else
    jb_ctx->uc_stack.ss_sp = job->ctx()->stack();
    jb_ctx->uc_stack.ss_size = context_t::stack_size();
    makecontext_fast(jb_ctx, (void (*)())job_t::do_run, 1, job);
#endif
    return resume_job_ctx();
}
int worker_t::run_job_ctx_with_nstx() {
    context_inner_t *jb_ctx = job->uctx();
    int ret = do_getcontext(jb_ctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;

    void *stack_buffer = mmap(NULL, ETH_BUFFER_SIZE + (4096 * 2), PROT_NONE,
                              MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (gcc_unlikely(stack_buffer == MAP_FAILED)) {
        printf("stack_buffer: %p\n", stack_buffer);
        abort();
    }
    char *stack_start = (char *)stack_buffer + 4096;

    // printf("stack: %p - %p\n", stack_start, stack_start + ETH_BUFFER_SIZE);

    mprotect(stack_start, ETH_BUFFER_SIZE, PROT_READ | PROT_WRITE);

    makecontext_nano(jb_ctx, stack_start, ETH_BUFFER_SIZE,
                     (void (*)())job_t::do_run, job);

    int rc = resume_job_ctx();

    if (munmap(stack_buffer, ETH_BUFFER_SIZE + 8192)) {
        printf("errno: %d\n", errno);
        abort();
    }

    return rc;
}

int worker_t::run_job_ctx_sjk() {
    context_inner_t *jb_ctx = job->uctx();
    int ret = do_getcontext(jb_ctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
#ifdef DILOS_SWAPCONTEXT_NANO
    makecontext_nano(jb_ctx, job->ctx()->stack(), context_t::stack_size(),
                     (void (*)())job_t::do_run_sjk, job);

#else
    jb_ctx->uc_stack.ss_sp = job->ctx()->stack();
    jb_ctx->uc_stack.ss_size = context_t::stack_size();
    makecontext_fast(jb_ctx, (void (*)())job_t::do_run_sjk, 1, job);
#endif
    return resume_job_ctx();
}

int worker_t::resume_job_ctx() {
#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard();
#endif
    int ret = swapcontext_w_to_a(&uctx, job->uctx());
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    return return_code;  // ret is len
}

int worker_t::pf_async_to_worker(uintptr_t offset, void *pa) {
    DP_TRACE(trace_asyncpf_fj, job->wr_id());

#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard();
#endif
    async_pf_offset = offset;
    async_pf_pa_or_ptep = pa;
    return_code = JOB_RET_PF_ASYNC;
    int ret = swapcontext_a_to_w(job->uctx(), &uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    // !important never use this pointer after swapcontext
    // !important worker can be changed
    return 0;  // handled well
}
int worker_t::yield_to_worker() {
    DP_TRACE(trace_yield_fj, job->wr_id());
#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard(14);
#endif
    return_code = JOB_RET_YIELD;
    int ret = swapcontext_a_to_w(job->uctx(), &uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    // !important never use this pointer after swapcontext
    // !important worker can be changed
    return 0;  // handled well
}
int worker_t::yield_global_to_worker() {
    DP_TRACE(trace_yield_fj, job->wr_id());
#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard(14);
#endif
    return_code = JOB_RET_YIELD_GLOBAL;
    int ret = swapcontext_a_to_w(job->uctx(), &uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    // !important never use this pointer after swapcontext
    // !important worker can be changed
    return 0;  // handled well
}

int worker_t::wait_to_worker(void *ptep) {
    DP_TRACE(trace_yield_fj, job->wr_id());
#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard(17);
#endif
    async_pf_pa_or_ptep = ptep;
    return_code = JOB_RET_WAIT;
    int ret = swapcontext_a_to_w(job->uctx(), &uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    // !important never use this pointer after swapcontext
    // !important worker can be changed
    return 0;  // handled well
}

int worker_t::page_alloc_async_to_worker() {
    // DP_TRACE(trace_yield_fj, job->wr_id());
#ifdef DILOS_CHECK_GUARD
    job->ctx()->check_guard(14);
#endif
    return_code = JOB_RET_ALLOC_ASYNC;
    int ret = swapcontext_a_to_w(job->uctx(), &uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    // !important never use this pointer after swapcontext
    // !important worker can be changed
    return 0;  // handled well
}

int worker_t::exit_to_worker() {
    DP_TRACE(trace_exit_fj, job->wr_id());
    int ret = do_setcontext_to_w(&uctx);
    if (gcc_unlikely(ret == -1)) return JOB_RET_ERROR;
    return 0;  // handled well
}

int worker_t::start(const job_adapter_t::init_t &app_config) {
    std::unique_lock<std::mutex> lck(mtx);

    switch (pf_mode) {
        case dispatcher_t::PF_MODE_ASYNC_LOCAL:
            _th = std::thread(worker_main_outer_async_local, std::ref(*this),
                              std::ref(app_config));
            break;

        case dispatcher_t::PF_MODE_SYNC_SJK:
            _th = std::thread(worker_main_outer_sjk, std::ref(*this),
                              std::ref(app_config));
            break;

        default:
            _th = std::thread(worker_main_outer, std::ref(*this),
                              std::ref(app_config));
            break;
    }

    while (state.load(std::memory_order_acquire) == STATE_UNINIT) {
        cv.wait(lck);
    }
    PRINT_LOG("thread started (main)\n");

    return 0;
}

int worker_t::poll_cq_async(job_t **jobs) {
    if (rdma_async_issued == 0) {
        return 0;
    }

    int ret;
    struct ibv_poll_cq_attr attr = {};
    ret = ibv_start_poll(rdma_cq_async, &attr);

    if (ret == ENOENT) {
        return 0;
    } else if (gcc_unlikely(ret > 0)) {
        return -ret;
    }
    int polled = 0;
    do {
        auto opcode = ibv_wc_read_opcode(rdma_cq_async);

        switch (opcode) {
            case IBV_WC_RDMA_READ:
                jobs[polled] = (job_t *)rdma_cq_async->wr_id;
                break;
            default:
                PRINT_LOG("unknown opcode: %d\n", opcode);
                break;
        }
        ++polled;
        ret = ibv_next_poll(rdma_cq_async);
    } while (ret != ENOENT && polled < rdma_async_issued);

    ibv_end_poll(rdma_cq_async);

#ifdef DILOS_RDMA_PERF
    rdma_count += polled;
    size_t cycles = get_cycles();

    if (gcc_unlikely(rdma_last_print == 0)) {
        rdma_last_print = get_cycles() + my_worker_id * 1'000'000'000ULL;
    } else if (gcc_likely(cycles > rdma_last_print)) {
        size_t diff = cycles - rdma_last_print;
        if (gcc_unlikely(diff > 10'000'000'000ULL)) {
            rdma_last_print = cycles;
            debug_early_u64("ID:", my_worker_id);
            debug_early_u64("GRPS:", rdma_count * 1'000'000'000ULL / diff);
            rdma_count = 0;
        }
    }

#endif

    return polled;
}

int worker_core_t::fetch_rdma(int qp_id, uintptr_t token, void *paddr,
                              uintptr_t roffset, size_t size) {
    auto &qp = rdma_qps[qp_id];

    auto &meta = dispatcher->mm.find_mr(paddr, size);
    ibv_wr_start(qp);
    qp->wr_id = token;
    qp->wr_flags = IBV_SEND_SIGNALED;
    ibv_wr_rdma_read(qp, dispatcher->rdma.server_rkey,
                     dispatcher->rdma.server_addr + roffset);
    ibv_wr_set_sge(qp, meta.ib_lkey, (uint64_t)paddr, size);
    int ret = ibv_wr_complete(qp);

    DP_TRACE(trace_fetch, qp_id, (void *)token, paddr, (void *)roffset, size,
             ret);
    return ret;
}
int worker_core_t::push_rdma(int qp_id, uintptr_t token, void *paddr,
                             uintptr_t roffset, size_t size) {
    auto &qp = rdma_qps[qp_id];

    auto &meta = dispatcher->mm.find_mr(paddr, size);
    ibv_wr_start(qp);
    qp->wr_id = token;
    qp->wr_flags = IBV_SEND_SIGNALED;
    ibv_wr_rdma_write(qp, dispatcher->rdma.server_rkey,
                      dispatcher->rdma.server_addr + roffset);
    ibv_wr_set_sge(qp, meta.ib_lkey, (uint64_t)paddr, size);
    int ret = ibv_wr_complete(qp);
    // DP_TRACE(trace_push, qp_id, (void *)token, paddr, (void *)roffset,
    // size,
    //          ret);
    return ret;
}
int worker_core_t::poll_rdma(uintptr_t tokens[], int len) {
    int ret = 0;
    struct ibv_poll_cq_attr attr = {};
    ret = ibv_start_poll(rdma_cq, &attr);
    if (ret == ENOENT) {
        return 0;
    } else if (gcc_unlikely(ret > 0)) {
        return -ret;
    }

    int polled = 0;
    do {
        rdma_cq->status;
        tokens[polled] = rdma_cq->wr_id;
        // DP_TRACE(trace_polled, (void *)rdma_cq->wr_id);
        ret = ibv_next_poll(rdma_cq);
        ++polled;
    } while (ret != ENOENT && polled < len);

    ibv_end_poll(rdma_cq);

#ifdef DILOS_RDMA_PERF
    rdma_count += polled;
    size_t cycles = get_cycles();

    if (gcc_unlikely(rdma_last_print == 0)) {
        rdma_last_print = get_cycles() + my_worker_id * 1'000'000'000ULL;
    } else if (gcc_likely(cycles > rdma_last_print)) {
        size_t diff = cycles - rdma_last_print;
        if (gcc_unlikely(diff > 10'000'000'000ULL)) {
            rdma_last_print = cycles;
            debug_early_u64("ID:", my_worker_id);
            debug_early_u64("RPGC:", rdma_count * 1'000'000'000ULL / diff);
            rdma_count = 0;
        }
    }

#endif

    return polled;
}
}  // namespace dispatcher

extern "C" {

void adios_ctx_yield() {
    if (dispatcher::current_worker && dispatcher::current_worker->job) {
        dispatcher::current_worker->yield_to_worker();
    }
}
void adios_ctx_yield_global() {
    adios_ctx_yield();
    return;
    if (dispatcher::current_worker && dispatcher::current_worker->job) {
        dispatcher::current_worker
            ->yield_global_to_worker();  // todo: change this to global
    }
}
int adios_ctx_id() {
    if (dispatcher::current_worker && dispatcher::current_worker->job) {
        return (int)(uintptr_t)dispatcher::current_worker->job;
    } else {
        return (int)(uintptr_t)pthread_self();
    }
}

char adios_ctx_worker_id() {
    if (dispatcher::current_worker) {
        return dispatcher::current_worker->worker_id;
    } else {
        return -1;
    }
}

void adios_ctx_set_cls(void *v) {
    if (dispatcher::current_worker && dispatcher::current_worker->job) {
        dispatcher::current_worker->job->ctx()->cls = v;
        return;
    }
    abort();
}
void *adios_ctx_get_cls() {
    if (dispatcher::current_worker && dispatcher::current_worker->job) {
        return dispatcher::current_worker->job->ctx()->cls;
    }
    return nullptr;
}
}

namespace dispatcher {
worker_t &get_current_worker() { return *current_worker; }
}  // namespace dispatcher
