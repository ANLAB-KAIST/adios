
#include <dispatcher/ctx.h>

#include <dispatcher/api.hh>
#include <dispatcher/app.hh>
#include <dispatcher/common.hh>
#include <dispatcher/job.hh>
#include <dispatcher/spinlock.hh>

#define CONFIG_H "config/config-perf.h"
#define NDB_MASSTREE 1
#define NO_MYSQL 1
#include <masstree/config.h>

#include <deque>
#include <map>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <benchmarks/bench.h>
#include <benchmarks/ndb_wrapper.h>
#include <benchmarks/ndb_wrapper_impl.h>
#pragma GCC diagnostic pop

// These are hacks to access protected members of classes defined in silo
class tpcc_bench_runner : public bench_runner {
   public:
    tpcc_bench_runner(abstract_db *db);
    std::vector<bench_loader *> make_loaders(void);
    std::vector<bench_worker *> make_workers(void);
    bench_worker *mkworker(unsigned int);
    bench_worker *mkworker(unsigned int, unsigned int);
    std::map<std::string, std::vector<abstract_ordered_index *>> partitions;
};

class my_bench_runner : public tpcc_bench_runner {
   public:
    my_bench_runner(abstract_db *db) : tpcc_bench_runner(db) {}
    std::vector<bench_loader *> call_make_loaders(void) {
        return make_loaders();
    }
    std::vector<bench_worker *> call_make_workers(void) {
        return make_workers();
    }
};

class my_bench_worker : public bench_worker {
   public:
    unsigned int get_worker_id(void) { return worker_id; }

    util::fast_random *get_r(void) { return &r; }

    void call_on_run_setup(void) { on_run_setup(); }
};

static abstract_db *db;
static my_bench_runner *runner;

extern "C" {

void silotpcc_exec_gc(void) {
    transaction_proto2_static::PurgeThreadOutstandingGCTasks();
}

static void silotpcc_load() {
    const std::vector<bench_loader *> loaders = runner->call_make_loaders();
    spin_barrier b(loaders.size());
    std::vector<std::thread> ths;
    for (std::vector<bench_loader *>::const_iterator it = loaders.begin();
         it != loaders.end(); ++it) {
        ths.emplace_back([=, &b] {
            (*it)->set_barrier(b);
            (*it)->start();
            (*it)->join();
        });
    }

    for (auto &w : ths) w.join();

    db->do_txn_epoch_sync();
    auto persisted_info = db->get_ntxn_persisted();
    assert(get<0>(persisted_info) == get<1>(persisted_info));
    db->reset_ntxn_persisted();
    persisted_info = db->get_ntxn_persisted();
    ALWAYS_ASSERT(get<0>(persisted_info) == 0 && get<1>(persisted_info) == 0 &&
                  get<2>(persisted_info) == 0.0);
}

std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::string::size_type start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}

int silotpcc_init(double scale_factor_, int number_threads, long numa_memory_) {
    enable_parallel_loading = 0;
    nthreads = number_threads;
    scale_factor = scale_factor_;
    pin_cpus = 1;
    silo_verbose = 1;
    long numa_memory = numa_memory_;
    size_t maxpercpu =
        util::iceil(numa_memory / nthreads, ::allocator::GetHugepageSize());
    ::allocator::Initialize(nthreads, maxpercpu);

    std::vector<std::string> logfiles;
    std::vector<std::vector<unsigned>> assignments;
    int nofsync = 0;
    int do_compress = 0;
    int fake_writes = 0;

    db = new ndb_wrapper<transaction_proto2>(logfiles, assignments, !nofsync,
                                             do_compress, fake_writes);
    ALWAYS_ASSERT(!transaction_proto2_static::get_hack_status());

    runner = new my_bench_runner(db);

    silotpcc_load();

    return 0;
}
}

std::pair<bool, ssize_t> do_neworder(bench_worker *w);

std::pair<bool, ssize_t> do_payment(bench_worker *w);

std::pair<bool, ssize_t> do_delivery(bench_worker *w);

std::pair<bool, ssize_t> do_orderstatus(bench_worker *w);

std::pair<bool, ssize_t> do_stocklevel(bench_worker *w);

extern "C" {

static size_t num_warehouses;
static size_t app_mem_size;
static size_t num_workers;
int dp_addon_silo_init(size_t _app_mem_size, size_t _num_warehouses,
                       size_t _num_workers) {
    num_warehouses = _num_warehouses;
    app_mem_size = _app_mem_size;
    num_workers = _num_workers;
    silotpcc_init(num_warehouses, num_workers, app_mem_size);
    ::allocator::DumpStats();
    return 0;
}

constexpr size_t default_worker_pool_counts = 16;
static thread_local std::vector<my_bench_worker *> worker_pool;
static dispatcher::spinlock_t worker_create_lock;

static thread_local int silo_worker_id = -1;

int dp_addon_silo_init_per_worker(uint8_t worker_id) {
    // rcu::s_instance.pin_current_thread(worker_id);

    // per-worker partition

    silo_worker_id = -1;

    size_t ware_per_worker = num_warehouses / num_workers;

    size_t ware_start = worker_id * ware_per_worker + 1;
    size_t ware_end = (worker_id + 1) * ware_per_worker + 1;
    worker_pool.reserve(default_worker_pool_counts);
    worker_create_lock.lock();

    for (size_t i = 0; i < default_worker_pool_counts; ++i) {
        auto w = (void *)runner->mkworker(ware_start, ware_end);
        my_bench_worker *worker = (my_bench_worker *)w;
        silo_worker_id = worker->get_worker_id();
        rcu::s_instance.pin_current_thread(worker_id);
        worker_pool.emplace_back(worker);
    }

    worker_create_lock.unlock();
    silo_worker_id = -1;

    if (worker_id == 0) {
        // warmw-up

        my_bench_worker *worker = worker_pool.back();
        bool ret;
        for (size_t i = 0; i < 100; ++i) {
            ret = do_neworder(worker).first;
            ret = do_payment(worker).first;
            ret = do_delivery(worker).first;
            ret = do_orderstatus(worker).first;
            ret = do_stocklevel(worker).first;
        }
    }
    return 0;
}

int dp_addon_silo_fn(uint8_t command) {
    if (worker_pool.empty()) {
        do {
            adios_ctx_yield();
        } while (worker_pool.empty());
    }
    my_bench_worker *worker = worker_pool.back();

    adios_ctx_set_cls((void *)worker);

    worker_pool.pop_back();
    bool ret = false;
    switch (command) {
        case 1:
            ret = do_neworder(worker).first;
            break;
        case 2:
            ret = do_payment(worker).first;
            break;
        case 3:
            ret = do_delivery(worker).first;
            break;
        case 4:
            ret = do_orderstatus(worker).first;
            break;
        case 5:
            ret = do_stocklevel(worker).first;
            break;
        default:
            printf("unknown command");
            abort();
    }
    worker_pool.push_back(worker);

    adios_ctx_set_cls((void *)nullptr);
    return ret != true;
}
}

extern "C" int get_silo_core_id() {
    void *v = adios_ctx_get_cls();

    if (v) {
        my_bench_worker *worker = (my_bench_worker *)v;
        return worker->get_worker_id();
    } else if (silo_worker_id != -1) {
        return silo_worker_id;
    }
    return -1;
}