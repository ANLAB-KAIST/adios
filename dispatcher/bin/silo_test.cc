#define CONFIG_H "config/config-perf.h"
#define NDB_MASSTREE 1
#define NO_MYSQL 1
#include <masstree/config.h>

#include <map>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <benchmarks/bench.h>
#include <benchmarks/ndb_wrapper.h>
#include <benchmarks/ndb_wrapper_impl.h>
#pragma GCC diagnostic pop

#include <me.hh>

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
// static vector<my_bench_worker *> workers;

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

// extern const unsigned g_txn_workload_mix[5];
// char *txn_desc;

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

    // This is a hack to access protected members of classes defined in silo
    // for (auto w: runner->call_make_workers())
    // 	workers.push_back((my_bench_worker *) w);

    // unsigned mix[5];

    // memcpy(mix, g_txn_workload_mix, sizeof(mix));
    // if (txn_desc) {
    //     auto tokens = split(std::string(txn_desc), ',');
    //     if (tokens.size() != 5) return -1;
    //     for (int i = 0; i < 5; i++) {
    //         mix[i] = std::stoul(tokens[i], nullptr, 0);
    //     }
    // }

    // for (int i = 0; i < 5; i++) {
    //     override_txn_mix[i] = ((double)mix[i]) / 100.0;
    //     fprintf(stderr, "%d: %.3f %u %u\n", i, override_txn_mix[i], mix[i],
    //             g_txn_workload_mix[i]);
    // }

    return 0;
}

// Hack to access private field coreid::tl_core_id
// extern __thread int _ZN6coreid10tl_core_idE;

void silotpcc_init_thread(int thread_id) {
    // auto worker = workers[thread_id];

    // Hack because the first thread of the program becomes a worker
    // _ZN6coreid10tl_core_idE = -1;

    // Copy-paste from benchmarks/bench.cc:112
    // coreid::set_core_id(worker->get_worker_id());
    // {
    // 	scoped_rcu_region r;
    // }
    // worker->call_on_run_setup();
}
}

std::pair<bool, ssize_t> do_neworder(bench_worker *w);

std::pair<bool, ssize_t> do_payment(bench_worker *w);

std::pair<bool, ssize_t> do_delivery(bench_worker *w);

std::pair<bool, ssize_t> do_orderstatus(bench_worker *w);

std::pair<bool, ssize_t> do_stocklevel(bench_worker *w);

int main() {
    // 1024 / 8 -> 73.7G

    double sf = 100;
    size_t num_w = (size_t)sf;

    silotpcc_init(sf, 8, 30ULL << 30);

    ::allocator::DumpStats();

    auto w = (void *)runner->mkworker(1, num_w + 1);
    my_bench_worker *worker = (my_bench_worker *)w;

    ME neworder("neworder");
    ME payment("payment");
    ME delivery("delivery");
    ME orderstatus("orderstatus");
    ME stocklevel("stocklevel");

    rcu::s_instance.pin_current_thread(0);
    while (true) {
        bool ret;
        neworder.start();
        ret = do_neworder(worker).first;
        neworder.end();
        payment.start();
        ret = do_payment(worker).first;
        payment.end();
        delivery.start();
        ret = do_delivery(worker).first;
        delivery.end();
        orderstatus.start();
        ret = do_orderstatus(worker).first;
        orderstatus.end();
        stocklevel.start();
        ret = do_stocklevel(worker).first;
        stocklevel.end();
    }

    return 0;
}

extern "C" int get_silo_core_id() { return -1; }
extern "C" void adios_ctx_yield() {}