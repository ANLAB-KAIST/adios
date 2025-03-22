

#include <emmintrin.h>
#include <protocol/rocksdb.h>

#include <algorithm>
#include <chrono>
#include <config/arg-config.hh>
#include <config/static-config.hh>
#include <dispatcher/net.hh>
#include <dispatcher/raw-eth.hh>
#include <filesystem>
#include <fstream>
#include <map>
#include <protocol/cmd.hh>
#include <protocol/req.hh>
#include <random>

#include "faiss_common.h"

using namespace dispatcher;
struct raw_eth_flow_attr {
    struct ibv_flow_attr attr;
    struct ibv_flow_spec_eth spec_eth;
} __attribute__((packed));

static struct ibv_device *ib_dev;
static struct ibv_context *ctx;
static struct ibv_pd *pd;

static std::unique_ptr<fvecs_mmap> bigann_query;

#define RANDOM_SEED 100

struct arg_config_t {
    ARG_STR_DEFAULT(out_path, "/tmp", "result out path");
    ARG_STR(dev_name, "IB device name. Example: mlx5_0");
    ARG_INT_DEFAULT(ib_port, 1, "device port number. Default:1");
    ARG_INT_DEFAULT(ib_num_rx_cq_desc, 2048,
                    "RX num of cq descs. Default:2048");
    ARG_INT_DEFAULT(ib_num_rx_wr_desc, 2048, "RX num of wr descs. Default:128");
    ARG_INT_DEFAULT(ib_max_entry_size, 4096,
                    "maximum tx/rx entry size. Default:4096");
    ARG_INT_LIST(core_ids, "core IDs of threads (non-zero, even)");
    ARG_INT_DEFAULT(core_id_start, -1, "core ID start. Prefer core_ids");
    ARG_INT_DEFAULT(core_num, -1,
                    "number of worker core. Prefer core_ids. 0 for auto");
    ARG_INT_DEFAULT(duration, 10,
                    "duration in seconds (non-zero). Prefer num_operation");
    ARG_INT_DEFAULT(num_operation, 0,
                    "nubmer of operations per TX threads (non-zero).");

    ARG_INT_DEFAULT(sample, 3000000, "sampling count");
    ARG_INT_DEFAULT(rps, 1000, "target TX requests per second (non-zero)");
    ARG_INT_DEFAULT(wait, 5, "seconds to wait after transmission");
    ARG_OPTIONS_DEFAULT(workload, "ping", "generating workload", "ping",
                        "array64", "warray64", "scan", "get", "tpcc",
                        "vsearch");
    ARG_INT_LIST_CUSTOM(receiver_mac,
                        "receiver's mac address. Default: 00:01:02:03:04:05",
                        ':', 16, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5);
    dispatcher::eth_addr_t _receiver_mac;
    ARG_INT_LIST_CUSTOM(
        loadgen_mac_prefix,
        "loadgen's mac address prefix. Last byte uses TX core id. "
        "Default: 02:01:02:03:04",
        ':', 16, 0x2, 0x1, 0x2, 0x3, 0x4);
    dispatcher::eth_addr_t _loadgen_mac_prefix;
    ARG_INT_LIST_CUSTOM(receiver_ip,
                        "receiver's IP address. Default: 192.168.2.200", '.',
                        10, 192, 168, 2, 200);
    dispatcher::ip_addr_t _receiver_ip;
    ARG_INT_LIST_CUSTOM(
        loadgen_ip_prefix,
        "loadgen's IP address prefix. Last byte uses TX core id. "
        "Default: 192.168.2",
        '.', 10, 192, 168, 2);
    dispatcher::ip_addr_t _loadgen_ip_prefix;

    ARG_INT_DEFAULT(wl_app_memsize, 1,
                    "workload array64: memory size in GB. Default: 1");

    ARG_OPTIONS_DEFAULT(wl_app_count_dist, "static",
                        "count distrubution (wl_app_count, wl_app_count)",
                        "static", "uniform", "multimodal");

    ARG_INT_LIST_CUSTOM(wl_app_count_multimodal,
                        "count multimodal distribtution (count1, count1_permil,"
                        "count2, count2_permil, ...)",
                        ',', 10, 1, 990, 100, 10);

    ARG_INT_DEFAULT(wl_app_klen, 16, "workload scan: key length. Default: 16");
    ARG_INT_DEFAULT(wl_app_kspace, 200000000,
                    "workload scan: key space. Default: 200000000");
    ARG_INT_DEFAULT(wl_app_count, 1, "workload: range count. Default: 1");
    ARG_INT_DEFAULT(wl_app_warehouses, 576,
                    "warehouses count for TPC-C. Default: 576");
    ARG_INT_DEFAULT(wl_app_is_num, 1, "is nubmer Default: 1");
    ARG_STR_DEFAULT(wl_app_vsearch,
                    "/opt/dilos/dataset/faiss/bigann_query.fvecs.128",
                    "workload path for vsearch");
    ARG_INT_DEFAULT(wl_app_vsearch_k, 10, "k value for vsearch");

    int parse(int argc, char **argv) {
        ARG_PARSE_START(argc, argv);
        ARG_PARSE(out_path);
        ARG_PARSE(dev_name);
        ARG_PARSE(ib_port);
        ARG_PARSE(ib_num_rx_cq_desc);
        ARG_PARSE(ib_num_rx_wr_desc);
        ARG_PARSE(ib_max_entry_size);
        ARG_PARSE(core_ids);
        ARG_PARSE(core_id_start);
        ARG_PARSE(core_num);
        ARG_PARSE(duration);
        ARG_PARSE(num_operation);
        ARG_PARSE(sample);
        ARG_PARSE(rps);
        ARG_PARSE(wait);
        ARG_PARSE(workload);
        ARG_PARSE(receiver_mac);
        ARG_PARSE(loadgen_mac_prefix);
        ARG_PARSE(receiver_ip);
        ARG_PARSE(loadgen_ip_prefix);
        ARG_PARSE(wl_app_memsize);
        ARG_PARSE(wl_app_klen);
        ARG_PARSE(wl_app_kspace);
        ARG_PARSE(wl_app_count_dist);
        ARG_PARSE(wl_app_count_multimodal);
        ARG_PARSE(wl_app_count);
        ARG_PARSE(wl_app_warehouses);
        ARG_PARSE(wl_app_vsearch);
        ARG_PARSE(wl_app_vsearch_k);
        ARG_PARSE_END();

        if (dev_name.empty()) {
            printf("No dev_name.\n");
            return 1;
        }

        if (!rps) {
            printf("Zero rps.\n");
            return 1;
        }
        if (!wait) {
            printf("Zero wait.\n");
            return 1;
        }

        if (core_ids.empty()) {
            if (core_num == 0) {
                // use max 500K for worker
                core_num = 2 * ((rps / 500000) + 1);
                printf("Calculated core_num: %lld\n", core_num);
            }

            if (core_id_start < 0 || core_num < 1) {
                printf(
                    "No core_ids or wrong "
                    "core_id_start/core_num\n");
                return 1;
            }

            for (long long id = core_id_start; id < core_num + core_id_start;
                 ++id) {
                core_ids.push_back(id);
            }
        }
        if (core_ids.size() % 2) {
            printf("Odd num of core ids\n");
            return 1;
        }
        if (!num_operation) {
            if (!duration) {
                printf("Zero num_operation and duration.\n");
                return 1;
            }
            num_operation = rps * duration;
            printf("Calculated num_operation: %lld\n", num_operation);
        }

        if (receiver_mac.size() != 6) {
            printf("MAC parsing error: receiver_mac\n");
            return 1;
        }
        for (int i = 0; i < 6; ++i) {
            unsigned int ul = receiver_mac[i];
            if (ul > 255) {
                printf("MAC parsing error: receiver_mac\n");
                return 1;
            }
            _receiver_mac[i] = ul;
        }

        if (receiver_ip.size() != 4) {
            printf("IP parsing error: receiver_ip\n");
            return 1;
        }
        for (int i = 0; i < 4; ++i) {
            unsigned int ul = receiver_ip[i];
            if (ul > 255) {
                printf("IP parsing error: receiver_ip\n");
                return 1;
            }
            _receiver_ip[i] = ul;
        }

        if (loadgen_mac_prefix.size() != 5) {
            printf("MAC parsing error: loadgen_mac_prefix\n");
            return 1;
        }
        for (int i = 0; i < 5; ++i) {
            unsigned int ul = loadgen_mac_prefix[i];
            if (ul > 255) {
                printf("MAC parsing error: loadgen_mac_prefix\n");
                return 1;
            }
            _loadgen_mac_prefix[i] = ul;
        }

        if (loadgen_ip_prefix.size() != 3) {
            printf("IP parsing error: loadgen_ip_prefix\n");
            return 1;
        }
        for (int i = 0; i < 3; ++i) {
            unsigned int ul = loadgen_ip_prefix[i];
            if (ul > 255) {
                printf("IP parsing error: loadgen_ip_prefix\n");
                return 1;
            }
            _loadgen_ip_prefix[i] = ul;
        }

        return 0;
    }

    static std::string usage(int argc, char **argv) {
        ARG_USAGE_START(argc, argv);
        ARG_USAGE(out_path);
        ARG_USAGE(dev_name);
        ARG_USAGE(ib_port);
        ARG_USAGE(ib_num_rx_cq_desc);
        ARG_USAGE(ib_num_rx_wr_desc);
        ARG_USAGE(ib_max_entry_size);
        ARG_USAGE(core_ids);
        ARG_USAGE(core_id_start);
        ARG_USAGE(core_num);
        ARG_USAGE(duration);
        ARG_USAGE(num_operation);
        ARG_USAGE(sample);
        ARG_USAGE(rps);
        ARG_USAGE(wait);
        ARG_USAGE(workload);
        ARG_USAGE(receiver_mac);
        ARG_USAGE(loadgen_mac_prefix);
        ARG_USAGE(receiver_ip);
        ARG_USAGE(loadgen_ip_prefix);
        ARG_USAGE(wl_app_memsize);
        ARG_USAGE(wl_app_count_dist);
        ARG_USAGE(wl_app_count_multimodal);
        ARG_USAGE(wl_app_klen);
        ARG_USAGE(wl_app_kspace);
        ARG_USAGE(wl_app_count);
        ARG_USAGE(wl_app_warehouses);
        ARG_USAGE(wl_app_is_num);
        ARG_USAGE(wl_app_vsearch);
        ARG_USAGE(wl_app_vsearch_k);
        return ARG_USAGE_END();
    }
    std::string str() {
        ARG_PRINT_START();
        ARG_PRINT(out_path);
        ARG_PRINT(dev_name);
        ARG_PRINT(ib_port);
        ARG_PRINT(ib_num_rx_cq_desc);
        ARG_PRINT(ib_num_rx_wr_desc);
        ARG_PRINT(ib_max_entry_size);
        ARG_PRINT(core_ids);
        ARG_PRINT(core_id_start);
        ARG_PRINT(core_num);
        ARG_PRINT(duration);
        ARG_PRINT(num_operation);
        ARG_PRINT(sample);
        ARG_PRINT(rps);
        ARG_PRINT(wait);
        ARG_PRINT(workload);
        ARG_PRINT(receiver_mac);
        ARG_PRINT(loadgen_mac_prefix);
        ARG_PRINT(receiver_ip);
        ARG_PRINT(loadgen_ip_prefix);
        ARG_PRINT(wl_app_memsize);
        ARG_PRINT(wl_app_count_dist);
        ARG_PRINT(wl_app_count_multimodal);
        ARG_PRINT(wl_app_klen);
        ARG_PRINT(wl_app_kspace);
        ARG_PRINT(wl_app_count);
        ARG_PRINT(wl_app_warehouses);
        ARG_PRINT(wl_app_is_num);
        ARG_PRINT(wl_app_vsearch);
        ARG_PRINT(wl_app_vsearch_k);
        return ARG_PRINT_END();
    }
};

static arg_config_t aconfig;

static uint64_t cycles_per_sec = 0;

static void set_cycles_per_sec() {
    constexpr int CAL_TIME = 4;
    printf("calculating cycles per sec...\n");
    auto before_s = std::chrono::high_resolution_clock::now();
    auto after_s = before_s;
    uint64_t prev = get_cycles();
    while (after_s - before_s < std::chrono::seconds(CAL_TIME)) {
        after_s = std::chrono::high_resolution_clock::now();
    }
    uint64_t after = get_cycles();

    cycles_per_sec = (after - prev) / CAL_TIME;

    printf("calculated: %lu\n", cycles_per_sec);
}

struct tx_measure_t {
    uint64_t timestamp;
    uint64_t idx;
    uint64_t count;
    uint8_t cmd;
};

struct rx_breakdown_t {
    uint64_t queued;
    uint64_t worker;
    uint64_t num_pf;
    uint64_t major_pf;
    uint64_t sum_pf;
    uint64_t polling_pf;
    uint64_t num_pending;
    uint64_t num_enqueue;
    uint64_t prev_wr_id;
    uint64_t num_pending_local;
    uint8_t worker_id;
};
struct rx_measure_t {
    uint64_t timestamp;
#ifdef PROTOCOL_BREAKDOWN
    rx_breakdown_t breakdown;
#endif
};

#ifdef PROTOCOL_MEASURE

struct result_t {
    uint64_t lg_lat;
    uint64_t lg_cmd;
    uint64_t lg_idx;
    uint64_t lg_count;
#ifdef PROTOCOL_BREAKDOWN
    uint64_t dp_queued;
    uint64_t dp_worker;
    uint64_t dp_pf;
    uint64_t dp_polling;
    uint64_t dp_pf_cnt;
    uint64_t dp_pf_major_cnt;
    uint64_t dp_num_pending;
    uint64_t dp_num_enqueue;
    uint64_t dp_prev_wr_id;
    uint64_t dp_prev_total_worker;
    uint64_t dp_prev_total_pf;
    uint64_t dp_prev_total_polling;
    uint64_t dp_num_pending_local;
    uint64_t dp_worker_id;
#endif
    static void add_head(std::ofstream &out) {
        out << ",lg_lat";
        out << ",lg_cmd";
        out << ",lg_idx";
        out << ",lg_count";
#ifdef PROTOCOL_BREAKDOWN
        out << ",dp_queued";
        out << ",dp_worker";
        out << ",dp_pf";
        out << ",dp_polling";
        out << ",dp_pf_cnt";
        out << ",dp_pf_major_cnt";
        out << ",dp_num_pending";
        out << ",dp_num_enqueue";
        out << ",dp_prev_wr_id";
        out << ",dp_prev_total_worker";
        out << ",dp_prev_total_pf";
        out << ",dp_prev_total_polling";
        out << ",dp_num_pending_local";
        out << ",dp_worker_id";
#endif
        out << "\n";
    }
    void add_line(std::ofstream &out) const {
        out << "," << lg_lat;
        out << "," << lg_cmd;
        out << "," << lg_idx;
        out << "," << lg_count;
#ifdef PROTOCOL_BREAKDOWN
        out << "," << dp_queued;
        out << "," << dp_worker;
        out << "," << dp_pf;
        out << "," << dp_polling;
        out << "," << dp_pf_cnt;
        out << "," << dp_pf_major_cnt;
        out << "," << dp_num_pending;
        out << "," << dp_num_enqueue;
        out << "," << dp_prev_wr_id;
        out << "," << dp_prev_total_worker;
        out << "," << dp_prev_total_pf;
        out << "," << dp_prev_total_polling;
        out << "," << dp_num_pending_local;
        out << "," << dp_worker_id;
#endif
        out << "\n";
    }
};

using latency_list_t = std::vector<std::pair<uint64_t, uint64_t>>;
using result_map_t = std::vector<std::optional<result_t>>;

#ifdef PROTOCOL_BREAKDOWN
void sum_pending(const result_map_t &rmap, uint64_t prev_wr_id, uint64_t count,
                 uint64_t &total_worker, uint64_t &total_pf,
                 uint64_t &total_polling) {
    total_worker = 0;
    total_pf = 0;
    total_polling = 0;
    while (count--) {
        auto entry = rmap[prev_wr_id];

        if (!entry.has_value()) {
            break;
        }

        total_worker += entry->dp_worker;
        total_pf += entry->dp_pf;
        total_polling += entry->dp_polling;
        prev_wr_id = entry->dp_prev_wr_id;
    }
}
#endif

void write_csv_to_out(std::ofstream &out, latency_list_t &latencies,
                      const result_map_t &result_map, long long sample) {
    std::sort(latencies.begin(), latencies.end());
    if (latencies.size()) {
        out << "wr_id";
        result_t::add_head(out);
        size_t step = latencies.size() < sample || sample == 0
                          ? 1
                          : latencies.size() / sample;
        for (size_t i = 0; i < latencies.size(); i += step) {
            auto &wr_id = latencies[i].second;
            auto &entry = result_map[wr_id];
            if (entry.has_value()) {
                out << std::to_string(wr_id);
                entry->add_line(out);
            }
        }
    }
}

struct loadgen_t {
    int core_id;
    uint64_t wr_id_start;
    uint64_t wr_id_end;
    uint8_t last_mac;
    struct ibv_cq_ex *eth_cq;
    struct ibv_qp_ex *eth_qp;

    uint8_t *buffer;

    struct ibv_mr *mr;

    std::thread thread;
    std::mutex mtx;
    std::condition_variable cv;

    int init_common();
    virtual int init() = 0;
    void run_common();
    virtual void run() = 0;

    static void start(loadgen_t &worker) { worker.run(); }
    void start_thread() {
        std::unique_lock<std::mutex> lck(mtx);
        thread = std::thread(start, std::ref(*this));
        cv.wait(lck);
    }

    void join() { thread.join(); }
};
static std::atomic_bool tx_fire;

struct cmd_desc_t {
    uint16_t payload_size;
    dispatcher::cmd_t cmd;
};

struct rv_t {
    std::random_device rd;
    std::mt19937_64 generator;
    std::uniform_int_distribution<uint64_t> dist;
    rv_t()
        : generator(
#ifdef RANDOM_SEED
              RANDOM_SEED
#else
              rd()
#endif
          ) {
    }
    virtual uint64_t gen() { return 0; }
    virtual ~rv_t() {}
};
struct static_rv_t : public rv_t {
    uint64_t value;
    static_rv_t(uint64_t value) : value(value) {}
    virtual uint64_t gen() override { return value; }
};
struct uniform_rv_t : public rv_t {
    // generate 1 ~ value
    uint64_t start, len;
    uniform_rv_t(uint64_t start, uint64_t len) : start(start), len(len) {}
    virtual uint64_t gen() override { return (dist(generator) % len) + start; }
};
struct uniform_int_rv_t : public rv_t {
    uniform_int_rv_t() {}
    virtual uint64_t gen() override { return dist(generator); }
};

struct table_rv_t : public rv_t {
    // generate based on table
    std::vector<uint64_t> tbl;
    table_rv_t(std::vector<uint64_t> &&tbl) : tbl(tbl) {}
    virtual uint64_t gen() override {
        return tbl[(dist(generator) % tbl.size())];
    }
};

struct non_uniform_rv_t : public rv_t {
    // generate NURand based on TPC-C
    uint64_t A, C, start, len;
    non_uniform_rv_t(uint64_t A, uint64_t C, uint64_t start, uint64_t len)
        : A(A), C(C), start(start), len(len) {}

    virtual uint64_t gen() override {
        return ((((dist(generator) % A) | (dist(generator) % len) + start) +
                 C) %
                len) +
               start;
    }
};

struct multimodal_rv_t : public rv_t {
    // generate 1 ~ value
    std::vector<std::pair<uint64_t, uint64_t>> permils;
    multimodal_rv_t(std::vector<std::pair<uint64_t, uint64_t>> &&permils)
        : permils(permils) {
        uint64_t sum_of_permils = 0;
        for (auto &permil : permils) {
            sum_of_permils += permil.second;
        }
        if (sum_of_permils != 1000) {
            throw;
        }
    }
    virtual uint64_t gen() override {
        uint64_t v = dist(generator) % 1000;
        uint64_t cp = 0;
        for (auto &permil : permils) {
            cp += permil.second;
            if (v < cp) {
                return permil.first;
            }
        }

        return 0;
    }
};

struct workload_gen_t {
    rv_t *idx_rv;
    rv_t *count_rv;

    workload_gen_t(rv_t *idx_rv, rv_t *count_rv)
        : idx_rv(idx_rv), count_rv(count_rv) {}
    uint16_t ping(void *addr, uint64_t &idx, uint64_t &count) {
        // addr is at least 4K
        // dispatcher::req_ping_t *req = (dispatcher::req_ping_t *)addr;
        idx = 0;
        count = 0;

        return sizeof(dispatcher::req_ping_t);
    }
    uint16_t array64(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_array64_t *req = (dispatcher::req_array64_t *)addr;
        count = count_rv->gen();
        for (uint64_t i = 0; i < count; ++i) {
            req->idx[i] = idx_rv->gen();
        }
        idx = req->idx[0];
        req->num_idx = count;
        return sizeof(dispatcher::req_array64_t) + count * sizeof(uint64_t);
    }
    uint16_t warray64(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_warray64_t *req = (dispatcher::req_warray64_t *)addr;
        count = count_rv->gen();
        for (uint64_t i = 0; i < count; ++i) {
            req->payload[i].idx = idx_rv->gen();
            req->payload[i].value = idx_rv->gen();  // just random
        }
        idx = req->payload[0].idx;
        req->num_idx = count;
        return sizeof(dispatcher::req_warray64_t) + count * sizeof(uint64_t);
    }
    uint16_t scan(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_scan_t *req = (dispatcher::req_scan_t *)addr;
        uint64_t key = idx_rv->gen();
        idx = key;
        count = count_rv->gen();
        req->count = count;
        req->klen = ROCKSDB_KEYLEN + 1;

        rocksdb_gen_key_init(req->key, ROCKSDB_KEYLEN + 1);
        rocksdb_gen_key(req->key, ROCKSDB_KEYLEN + 1, key);

        return sizeof(dispatcher::req_scan_t) + ROCKSDB_KEYLEN + 1;
    }
    uint16_t get(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_get_t *req = (dispatcher::req_get_t *)addr;
        uint64_t key = idx_rv->gen();
        idx = key;
        req->klen = aconfig.wl_app_klen + 1;
        count = 1;

        rocksdb_gen_key_init(req->key, aconfig.wl_app_klen + 1);
        rocksdb_gen_key(req->key, aconfig.wl_app_klen + 1, key);

        return sizeof(dispatcher::req_get_t) + aconfig.wl_app_klen + 1;
    }
    uint16_t vsearch(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_vsearch_t *req = (dispatcher::req_vsearch_t *)addr;

        idx = idx_rv->gen();
        const float *query = &bigann_query->array[idx * bigann_query->d];
        count = 0;

        req->k = aconfig.wl_app_vsearch_k;
        req->vlen = bigann_query->d;
        memcpy(req->vec, query, sizeof(float) * bigann_query->d);

        return sizeof(dispatcher::req_vsearch_t) +
               sizeof(float) * bigann_query->d;
    }
    uint16_t tpcc(void *addr, uint64_t &idx, uint64_t &count) {
        dispatcher::req_tpcc_t *req = (dispatcher::req_tpcc_t *)addr;

        idx = 0;
        count = count_rv->gen();
        req->cmd = count;

        return sizeof(dispatcher::req_tpcc_t);
    }
};

dispatcher::cmd_t parse_cmd(std::string cmd_str) {
#define PARSE_CMD(X) \
    if (cmd_str == #X) return dispatcher::cmd_t::X;

    PARSE_CMD(ping);
    PARSE_CMD(array64);
    PARSE_CMD(scan);
    PARSE_CMD(get);
    PARSE_CMD(tpcc);
    PARSE_CMD(warray64);
    PARSE_CMD(vsearch);
    return dispatcher::cmd_t::none;

#undef PARSE_CMD
}

struct loadgen_tx_t : public loadgen_t {
    virtual int init() override;
    virtual void run() override;
    void init_rv();
    void init_cmd();
    void wait_rv(uint64_t cycles_before);

    uint64_t avg_tx_ns();
    void first_and_last_ts(uint64_t &first, uint64_t &last);

    uint64_t target_rps;
    std::vector<uint64_t> rvs;

    uint8_t *cmd_buffer;

    std::vector<tx_measure_t> tx_measure;

    size_t current_id;
};
struct loadgen_rx_t : public loadgen_t {
    std::atomic_bool running;
    struct ibv_flow *eth_flow;

    std::vector<rx_measure_t> rx_measure;
    std::vector<struct ibv_recv_wr> recvs;
    std::vector<struct ibv_sge> recv_sges;
    virtual int init() override;
    virtual void run() override;
    void first_and_last_ts(uint64_t &first, uint64_t &last);
};

int loadgen_t::init_common() {
    struct ibv_cq_init_attr_ex cq_init_attr_ex;
    memset(&cq_init_attr_ex, 0, sizeof(cq_init_attr_ex));
    cq_init_attr_ex.cqe = aconfig.ib_num_rx_cq_desc;
    cq_init_attr_ex.wc_flags = IBV_WC_EX_WITH_COMPLETION_TIMESTAMP_WALLCLOCK;
    cq_init_attr_ex.flags = IBV_CREATE_CQ_ATTR_SINGLE_THREADED;

    eth_cq = ibv_create_cq_ex(ctx, &cq_init_attr_ex);

    if (eth_cq == nullptr) {
        printf("fail to create cq\n");
        return 1;
    }

    return 0;
}

int loadgen_tx_t::init() {
    tx_measure.resize(wr_id_end - wr_id_start);
    init_common();
    init_rv();
    init_cmd();
    struct ibv_qp_cap cap;
    int rc = 0;
    memset(&cap, 0, sizeof(cap));
    cap.max_inline_data = 128;
    cap.max_send_wr = 1;
    cap.max_send_sge = 1;
    buffer =
        (uint8_t *)mmap(NULL, aconfig.ib_max_entry_size, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    if (buffer == MAP_FAILED) {
        printf("Coudln't allocate memory\n");
        return 1;
    }
    mr = ibv_reg_mr(pd, buffer, aconfig.ib_max_entry_size,
                    IBV_ACCESS_LOCAL_WRITE);
    if (!mr) {
        printf("Couldn't register mr\n");
        return 1;
    }

    struct ibv_qp *eth_qp =
        raweth::create_qp_raw_rtr(ctx, aconfig.ib_port, pd, eth_cq, cap, 1);
    if (eth_qp == NULL) {
        printf("fail to create QP\n");
        return 1;
    }
    this->eth_qp = ibv_qp_to_qp_ex(eth_qp);
    rc = raweth::modify_qp_raw_rts(&this->eth_qp->qp_base);
    if (rc) {
        printf("fail to modify QP to RTS\n");
        return 1;
    }

    return 0;
}

static std::string NameTokens[] = {
    std::string("BAR"),  std::string("OUGHT"), std::string("ABLE"),
    std::string("PRI"),  std::string("PRES"),  std::string("ESE"),
    std::string("ANTI"), std::string("CALLY"), std::string("ATION"),
    std::string("EING"),
};

static inline size_t GetCustomerLastName(char *buf, int num) {
    const std::string &s0 = NameTokens[num / 100];
    const std::string &s1 = NameTokens[(num / 10) % 10];
    const std::string &s2 = NameTokens[num % 10];
    char *const begin = buf;
    const size_t s0_sz = s0.size();
    const size_t s1_sz = s1.size();
    const size_t s2_sz = s2.size();
    memcpy(buf, s0.data(), s0_sz);
    buf += s0_sz;
    memcpy(buf, s1.data(), s1_sz);
    buf += s1_sz;
    memcpy(buf, s2.data(), s2_sz);
    buf += s2_sz;
    return buf - begin;
}

void loadgen_tx_t::init_cmd() {
    size_t num_operations = wr_id_end - wr_id_start;
    size_t inc_unit = ((aconfig.ib_max_entry_size + 4095) >> 12) << 12;
    inc_unit *= 16;

    std::unique_ptr<rv_t> rv_idx = nullptr;
    std::unique_ptr<rv_t> rv_count = nullptr;

    if (aconfig.workload == "array64" || aconfig.workload == "warray64") {
        uint64_t max_idx = (aconfig.wl_app_memsize << 30) / sizeof(uint64_t);
        rv_idx = std::make_unique<uniform_rv_t>(0, max_idx);
    } else if (aconfig.workload == "scan") {
        rv_idx = std::make_unique<uniform_rv_t>(0, ROCKSDB_KEYSPACE);
    } else if (aconfig.workload == "get") {
        rv_idx = std::make_unique<uniform_rv_t>(0, aconfig.wl_app_kspace);
    } else if (aconfig.workload == "ping") {
    } else if (aconfig.workload == "vsearch") {
        rv_idx = std::make_unique<uniform_rv_t>(0, bigann_query->n);
    } else if (aconfig.workload == "tpcc") {
    } else {
        throw;
    }

    if (aconfig.workload == "tpcc") {
        std::vector<std::pair<uint64_t, uint64_t>> permils;
        //  45, 43, 4, 4, 4
        permils.emplace_back(
            std::make_pair<uint64_t, uint64_t>(1, 445));  // new order
        permils.emplace_back(
            std::make_pair<uint64_t, uint64_t>(2, 431));  // payment
        permils.emplace_back(
            std::make_pair<uint64_t, uint64_t>(3, 42));  // delivery
        permils.emplace_back(
            std::make_pair<uint64_t, uint64_t>(4, 41));  // order status
        permils.emplace_back(
            std::make_pair<uint64_t, uint64_t>(5, 41));  // stock level
        rv_count = std::make_unique<multimodal_rv_t>(std::move(permils));
    } else if (aconfig.workload == "get") {
        rv_count = std::make_unique<static_rv_t>(1);
    } else if (aconfig.workload == "ping") {
    } else if (aconfig.wl_app_count_dist == "static") {
        rv_count = std::make_unique<static_rv_t>(aconfig.wl_app_count);
    } else if (aconfig.wl_app_count_dist == "uniform") {
        rv_count = std::make_unique<uniform_rv_t>(1, aconfig.wl_app_count);
    } else if (aconfig.wl_app_count_dist == "multimodal") {
        if (aconfig.wl_app_count_multimodal.size() % 2 != 0) {
            throw;
        }
        std::vector<std::pair<uint64_t, uint64_t>> permils;
        for (size_t i = 0; i < aconfig.wl_app_count_multimodal.size(); i += 2) {
            permils.emplace_back(aconfig.wl_app_count_multimodal[i],
                                 aconfig.wl_app_count_multimodal[i + 1]);
        }
        rv_count = std::make_unique<multimodal_rv_t>(std::move(permils));
    } else {
        throw;
    }

    workload_gen_t gen(rv_idx.get(), rv_count.get());
    size_t buff_size = inc_unit;

    uint8_t *buffer =
        (uint8_t *)mmap(NULL, buff_size, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    uint64_t offset = 0;
    for (size_t i = 0; i < num_operations; ++i) {
        auto cmd = parse_cmd(aconfig.workload);
        size_t payload_size = 0;
        cmd_desc_t *cmd_desc = (cmd_desc_t *)(buffer + offset);

        offset += sizeof(cmd_desc_t);
        tx_measure[i].cmd = static_cast<uint8_t>(cmd);
        uint64_t idx, count;

        switch (cmd) {
            case dispatcher::cmd_t::ping:
                payload_size = gen.ping(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::array64:
                payload_size = gen.array64(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::warray64:
                payload_size = gen.warray64(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::scan:
                payload_size = gen.scan(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::get:
                payload_size = gen.get(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::vsearch:
                payload_size = gen.vsearch(buffer + offset, idx, count);
                break;
            case dispatcher::cmd_t::tpcc:
                payload_size = gen.tpcc(buffer + offset, idx, count);
                break;
            default:
                printf("unknown cmd\n");
                break;
        }
        if (payload_size >= 1500) {
            printf("payload too large!!!\n");
            abort();
        }

        tx_measure[i].idx = idx;
        tx_measure[i].count = count;
        cmd_desc->payload_size = payload_size;
        cmd_desc->cmd = cmd;
        offset += payload_size;

        if (aconfig.ib_max_entry_size > (buff_size - offset)) {
            buffer = (uint8_t *)mremap(buffer, buff_size, buff_size + inc_unit,
                                       MREMAP_MAYMOVE);
            if (buffer == MAP_FAILED) {
                printf("remap failed\n");
                exit(1);
            }
            buff_size += inc_unit;
        }
    }

    cmd_buffer = buffer;

    if (bigann_query.get() && bigann_query->array)
        bigann_query->do_unmap();  // reduce memory usage
}
void loadgen_tx_t::init_rv() {
    current_id = 0;

    rvs.reserve(wr_id_end - wr_id_start);

    if (!target_rps) return;

    uint64_t cps = cycles_per_sec;

    std::random_device rd;

#ifdef RANDOM_SEED
    std::mt19937_64 gen(RANDOM_SEED);
#else
    std::mt19937_64 gen(rd());
#endif

    uint64_t calculated_rps = 1000000 * target_rps / (1000000 - target_rps);

    std::exponential_distribution<> d(calculated_rps);

    for (size_t i = 0; i < wr_id_end - wr_id_start; ++i) {
        double seconds_to_wait = d(gen);
        double uint64_to_wait = seconds_to_wait * cps;
        uint64_t uint64_to_wait_uint = (uint64_t)uint64_to_wait;
        rvs.push_back(uint64_to_wait_uint);
    }
    uint64_t sum = 0;
    for (auto &rv : rvs) {
        sum += rv;
    }
}
void loadgen_tx_t::wait_rv(uint64_t cycles_before) {
    uint64_t uint64_to_wait = rvs[current_id++];
    uint64_t cycles_after = 0;
    do {
        _mm_pause();
        cycles_after = get_cycles();
    } while ((cycles_after - cycles_before) < uint64_to_wait);
}
int loadgen_rx_t::init() {
    init_common();
    struct ibv_qp_cap cap;
    memset(&cap, 0, sizeof(cap));
    cap.max_recv_wr = aconfig.ib_num_rx_wr_desc;
    cap.max_recv_sge = 1;

    size_t buff_size = aconfig.ib_num_rx_wr_desc * aconfig.ib_max_entry_size;
    buffer = (uint8_t *)mmap(NULL, buff_size, PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    if (buffer == MAP_FAILED) {
        printf("Coudln't allocate memory\n");
        return 1;
    }
    mr = ibv_reg_mr(pd, buffer, buff_size, IBV_ACCESS_LOCAL_WRITE);
    if (!mr) {
        printf("Couldn't register mr\n");
        return 1;
    }

    struct ibv_qp *eth_qp =
        raweth::create_qp_raw_rtr(ctx, aconfig.ib_port, pd, eth_cq, cap, 1);
    if (eth_qp == NULL) {
        printf("fail to create QP\n");
        return 1;
    }
    this->eth_qp = ibv_qp_to_qp_ex(eth_qp);

    recvs.resize(aconfig.ib_num_rx_wr_desc);
    recv_sges.resize(aconfig.ib_num_rx_wr_desc);
    memset(recvs.data(), 0,
           aconfig.ib_num_rx_wr_desc * sizeof(struct ibv_recv_wr));

    // flow

    uint8_t flow_attr_bytes[sizeof(struct raw_eth_flow_attr)];
    struct raw_eth_flow_attr *flow_attr =
        (struct raw_eth_flow_attr *)flow_attr_bytes;

    flow_attr->attr = {
        .comp_mask = 0,
        .type = IBV_FLOW_ATTR_NORMAL,
        .size = sizeof(flow_attr),
        .priority = 0,
        .num_of_specs = 1,
        .port = (uint8_t)aconfig.ib_port,
        .flags = 0,
    };
    flow_attr->spec_eth = {.type = IBV_FLOW_SPEC_ETH,
                           .size = sizeof(struct ibv_flow_spec_eth),
                           .val =
                               {
                                   .dst_mac = {0, 0, 0, 0, 0, 0},
                                   .src_mac = {0, 0, 0, 0, 0, 0},
                                   .ether_type = 0,
                                   .vlan_tag = 0,
                               },
                           .mask = {
                               .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                               .src_mac = {0, 0, 0, 0, 0, 0},
                               .ether_type = 0,
                               .vlan_tag = 0,
                           }};

    flow_attr->spec_eth.val.dst_mac[0] = aconfig.loadgen_mac_prefix[0];
    flow_attr->spec_eth.val.dst_mac[1] = aconfig.loadgen_mac_prefix[1];
    flow_attr->spec_eth.val.dst_mac[2] = aconfig.loadgen_mac_prefix[2];
    flow_attr->spec_eth.val.dst_mac[3] = aconfig.loadgen_mac_prefix[3];
    flow_attr->spec_eth.val.dst_mac[4] = aconfig.loadgen_mac_prefix[4];
    flow_attr->spec_eth.val.dst_mac[5] = last_mac;
    eth_flow = ibv_create_flow(&this->eth_qp->qp_base,
                               (struct ibv_flow_attr *)flow_attr_bytes);

    if (!eth_flow) {
        printf("Couldn't attach steering flow\n");
        return 1;
    }
    rx_measure.resize(wr_id_end - wr_id_start);

    return 0;
}

void loadgen_rx_t::first_and_last_ts(uint64_t &first, uint64_t &last) {
    first = -1;
    last = 0;
    for (auto &rx_ : rx_measure) {
        if (rx_.timestamp < first) {
            first = rx_.timestamp;
        }

        if (rx_.timestamp > last) {
            last = rx_.timestamp;
        }
    }
}

void loadgen_t::run_common() {
    std::unique_lock<std::mutex> lck(mtx);
    printf("starting worker @ cpu %d\n", core_id);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (rc) {
        printf("worker @ cpu %d failed: pin\n", core_id);
        return;
    }
    cv.notify_all();
}
void loadgen_tx_t::run() {
    run_common();

    int ret;

    dispatcher::cmdpkt_t *udp_wr = (dispatcher::cmdpkt_t *)buffer;
    uint8_t *payload = buffer + sizeof(dispatcher::cmdpkt_t);
    udp_wr->eth.src = aconfig._loadgen_mac_prefix;
    udp_wr->eth.src[5] = last_mac;
    udp_wr->eth.dst = aconfig._receiver_mac;

    uint8_t *curr = cmd_buffer;

    while (!tx_fire.load(std::memory_order_acquire)) {
        // wait fire
        _mm_pause();
    }

    uint64_t cycles_before = get_cycles();
    for (uint64_t wr_id = wr_id_start; wr_id < wr_id_end; ++wr_id) {
        // setup pkt
        cmd_desc_t *cmd_desc = (cmd_desc_t *)curr;
        udp_wr->cmd.cmd = cmd_desc->cmd;
        memcpy(payload, curr + sizeof(cmd_desc_t), cmd_desc->payload_size);

        // 1. post send from last-send - last-comp

        ibv_wr_start(eth_qp);

        eth_qp->wr_id = wr_id;
        eth_qp->wr_flags = 0;
#ifdef PROTOCOL_MEASURE
        udp_wr->measure.lgen_wr_id = eth_qp->wr_id;

#ifdef PROTOCOL_BREAKDOWN
        udp_wr->breakdown.queued = 0;
        udp_wr->breakdown.worker = 0;
        udp_wr->breakdown.num_pf = 0;
        udp_wr->breakdown.major_pf = 0;
        udp_wr->breakdown.sum_pf = 0;
        udp_wr->breakdown.polling_pf = 0;
        udp_wr->breakdown.num_pending = 0;
        udp_wr->breakdown.num_enqueue = 0;
        udp_wr->breakdown.num_pending_local = 0;
        udp_wr->breakdown.worker_id = -1;
#endif
#endif
        ibv_wr_send(eth_qp);
        ibv_wr_set_sge(eth_qp, mr->lkey, (uintptr_t)buffer,
                       sizeof(dispatcher::cmdpkt_t) + cmd_desc->payload_size);

        curr += sizeof(cmd_desc_t) + cmd_desc->payload_size;

        wait_rv(cycles_before);  // wait until next fire
        ret = ibv_wr_complete(eth_qp);

        if (gcc_unlikely(ret)) {
            printf("fail post: %d\n", ret);
            break;
        }

        struct ibv_poll_cq_attr attr = {};
        do {
            ret = ibv_start_poll(eth_cq, &attr);
        } while (ret == ENOENT);

        if (gcc_unlikely(ret)) {
            fprintf(stderr, "poll CQ failed %d\n", ret);
            break;
        }
        do {
            uint64_t ns = ibv_wc_read_completion_wallclock_ns(eth_cq);
            tx_measure[eth_cq->wr_id - wr_id_start].timestamp = ns;
            ret = ibv_next_poll(eth_cq);
        } while (ret != ENOENT);

        if (gcc_unlikely(ret != ENOENT)) {
            fprintf(stderr, "next poll QP failed %d\n", ret);
            break;
        }

        ibv_end_poll(eth_cq);
        cycles_before = get_cycles();
    }
}

void loadgen_rx_t::run() {
    run_common();

    running.store(true, std::memory_order_release);
    for (size_t i = 0; i < recvs.size(); ++i) {
        memset(&recvs[i], 0, sizeof(recvs[i]));
        recvs[i].sg_list = &recv_sges[i];
        recvs[i].num_sge = 1;
    }

    int last_comp = aconfig.ib_num_rx_wr_desc - 1;
    int last_recv = 0;
    int ret;

    do {
        // 1. post recv
        struct ibv_recv_wr *next;
        struct ibv_recv_wr **last_next = &next;
        struct ibv_recv_wr *first = &recvs[last_recv];
        while (last_recv != last_comp) {
            *last_next = &recvs[last_recv];
            recvs[last_recv].wr_id = last_recv;

            uint64_t buff_addr =
                (uint64_t)(buffer) + last_recv * aconfig.ib_max_entry_size;

            recv_sges[last_recv].addr = buff_addr;
            recv_sges[last_recv].length = aconfig.ib_max_entry_size;
            recv_sges[last_recv].lkey = mr->lkey;

            last_next = &recvs[last_recv].next;
            last_recv = (last_recv + 1) % aconfig.ib_num_rx_wr_desc;
        }
        *last_next = NULL;
        ret = ibv_post_recv(&eth_qp->qp_base, first, &next);
        if (gcc_unlikely(ret)) {
            fprintf(stderr, "post recv failed %d\n", ret);
            break;
        }

        // 2. poll cq

        struct ibv_poll_cq_attr attr = {};
        do {
            ret = ibv_start_poll(eth_cq, &attr);
        } while (ret == ENOENT && running.load(std::memory_order_acquire));

        if (gcc_unlikely(ret)) {
            if (running.load(std::memory_order_acquire)) {
                fprintf(stderr, "poll CQ failed %d\n", ret);
            }
            break;
        }
        do {
#ifdef PROTOCOL_MEASURE

            uint64_t buff_addr =
                (uint64_t)(buffer) + eth_cq->wr_id * aconfig.ib_max_entry_size;

            dispatcher::cmdpkt_t *udp = (dispatcher::cmdpkt_t *)buff_addr;
            uint64_t ns = ibv_wc_read_completion_wallclock_ns(eth_cq);

            auto idx = udp->measure.lgen_wr_id - wr_id_start;
            if (gcc_likely(idx < wr_id_end - wr_id_start)) {
                rx_measure[idx].timestamp = ns;
#ifdef PROTOCOL_BREAKDOWN
                rx_measure[idx].breakdown.queued = udp->breakdown.queued;
                rx_measure[idx].breakdown.worker = udp->breakdown.worker;
                rx_measure[idx].breakdown.num_pf = udp->breakdown.num_pf;
                rx_measure[idx].breakdown.major_pf = udp->breakdown.major_pf;
                rx_measure[idx].breakdown.sum_pf = udp->breakdown.sum_pf;
                rx_measure[idx].breakdown.polling_pf =
                    udp->breakdown.polling_pf;
                rx_measure[idx].breakdown.num_pending =
                    udp->breakdown.num_pending;
                rx_measure[idx].breakdown.num_enqueue =
                    udp->breakdown.num_enqueue;
                rx_measure[idx].breakdown.prev_wr_id =
                    udp->breakdown.prev_wr_id;
                rx_measure[idx].breakdown.num_pending_local =
                    udp->breakdown.num_pending_local;
                rx_measure[idx].breakdown.worker_id = udp->breakdown.worker_id;
#endif
            } else {
                printf("unknown wr_id: %ld\n", idx + wr_id_start);
            }
#endif

            last_comp = (last_comp + 1) % aconfig.ib_num_rx_wr_desc;
            ret = ibv_next_poll(eth_cq);
        } while (ret != ENOENT);

        if (gcc_unlikely(ret != ENOENT)) {
            fprintf(stderr, "next poll QP failed %d\n", ret);
            break;
        }

        ibv_end_poll(eth_cq);

    } while (running.load(std::memory_order_acquire));
}

uint64_t loadgen_tx_t::avg_tx_ns() {
    if (tx_measure.size() < 2) {
        return 0;
    }
    uint64_t sum = 0;
    for (size_t i = 0; i < tx_measure.size() - 1; ++i) {
        uint64_t diff = tx_measure[i + 1].timestamp - tx_measure[i].timestamp;
        sum += diff;
    }

    uint64_t avg = sum / (tx_measure.size() - 1);

    return avg;
}

void loadgen_tx_t::first_and_last_ts(uint64_t &first, uint64_t &last) {
    first = -1;
    last = 0;
    for (auto &tx_ : tx_measure) {
        if (tx_.timestamp < first) {
            first = tx_.timestamp;
        }

        if (tx_.timestamp > last) {
            last = tx_.timestamp;
        }
    }
}

int main(int argc, char **argv) {
    auto usage = arg_config_t::usage(argc, argv);

    int parse_result = aconfig.parse(argc, argv);

    printf("[Configuration]\n\n%s\n", aconfig.str().c_str());

    if (parse_result) {
        printf("\n\nUsage: %s\n", usage.c_str());
        return 1;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (rc) {
        printf("pin fail\n");
        return 1;
    }
    set_cycles_per_sec();
    // init
    struct ibv_device **dev_list;
    ib_dev = NULL;
    ctx = NULL;
    int num_device = 0;
    dev_list = ibv_get_device_list(&num_device);
    if (!dev_list) {
        perror("Failed to get devices list");
        return 1;
    }

    for (int i = 0; i < num_device; ++i) {
        if (aconfig.dev_name == dev_list[i]->name) {
            ib_dev = dev_list[i];
        }
    }

    if (!ib_dev) {
        printf("IB device not found\n");
        return 1;
    }
    ctx = ibv_open_device(ib_dev);
    if (!ctx) {
        printf("Couldn't get context for %s\n", ibv_get_device_name(ib_dev));
        return 1;
    }

    pd = ibv_alloc_pd(ctx);

    if (!pd) {
        printf("Couldn't allocate PD\n");
        return 1;
    }

    auto vsearch_path = std::filesystem::path(aconfig.wl_app_vsearch);

    if (aconfig.workload == "vsearch") {
        if (std::filesystem::is_regular_file(vsearch_path)) {
            bigann_query = std::make_unique<fvecs_mmap>(vsearch_path.c_str());
        } else {
            printf("no %s\n", vsearch_path.c_str());
        }
    }

    std::vector<std::shared_ptr<loadgen_tx_t>> workers_tx;
    std::vector<std::shared_ptr<loadgen_rx_t>> workers_rx;
    tx_fire.store(false, std::memory_order_release);
    size_t num_tx_threads = aconfig.core_ids.size() / 2;

    for (worker_id_t worker_id = 0; worker_id < aconfig.core_ids.size();
         worker_id += 2) {
        auto loadgen_tx = std::make_shared<loadgen_tx_t>();
        auto worker_rx = std::make_shared<loadgen_rx_t>();

        loadgen_tx->target_rps = aconfig.rps / num_tx_threads;

        loadgen_tx->core_id = aconfig.core_ids[worker_id];
        worker_rx->core_id = aconfig.core_ids[worker_id + 1];

        loadgen_tx->last_mac = worker_id;
        worker_rx->last_mac = worker_id;
        uint64_t num_operations_per_th = aconfig.num_operation / num_tx_threads;
        loadgen_tx->wr_id_start = num_operations_per_th * (worker_id / 2);
        worker_rx->wr_id_start = num_operations_per_th * (worker_id / 2);

        loadgen_tx->wr_id_end = num_operations_per_th * ((worker_id / 2) + 1);
        worker_rx->wr_id_end = num_operations_per_th * ((worker_id / 2) + 1);

        loadgen_tx->init();
        worker_rx->init();
        workers_tx.push_back(loadgen_tx);
        workers_rx.push_back(worker_rx);
    }

    printf("Starting rx workers\n");

    for (auto &worker_rx : workers_rx) {
        worker_rx->start_thread();
    }

    printf("Starting tx workers\n");
    for (auto &loadgen_tx : workers_tx) {
        loadgen_tx->start_thread();
    }

    tx_fire.store(true, std::memory_order_release);
    printf("Waiting tx workers\n");
    for (auto &loadgen_tx : workers_tx) {
        loadgen_tx->join();
    }

    printf("Tx finished. Waiting %llds\n", aconfig.wait);
    std::this_thread::sleep_for(std::chrono::seconds(aconfig.wait));

    printf("Stopping rx workers\n");
    for (auto &worker_rx : workers_rx) {
        worker_rx->running.store(false, std::memory_order_release);
    }

    printf("Waiting rx workers\n");
    for (auto &worker_rx : workers_rx) {
        worker_rx->join();
    }
    printf("Rx finished.\n");

    for (worker_id_t worker_id = 0; worker_id < num_tx_threads; ++worker_id) {
        uint64_t avg = workers_tx[worker_id]->avg_tx_ns();
        printf(
            "[Result] tx_worker[%d]'s avg tx interval: %ld ns"
            "Target interval: %ld ns\n",
            worker_id, avg, 1000000000 / workers_tx[worker_id]->target_rps);
    }

    std::vector<std::optional<tx_measure_t>> tx_measure_global;
    std::vector<std::optional<rx_measure_t>> rx_measure_global;
    tx_measure_global.resize(aconfig.num_operation);
    rx_measure_global.resize(aconfig.num_operation);

    std::map<std::pair<uint64_t, uint8_t>, uint64_t> count_cmd_list;

    for (worker_id_t worker_id = 0; worker_id < num_tx_threads; ++worker_id) {
        for (auto wr_id = workers_tx[worker_id]->wr_id_start;
             wr_id < workers_tx[worker_id]->wr_id_end; ++wr_id) {
            auto measure =
                workers_tx[worker_id]
                    ->tx_measure[wr_id - workers_tx[worker_id]->wr_id_start];
            if (measure.timestamp) {
                if (wr_id >= tx_measure_global.size()) {
                    printf("unknwon wr_id 1: %ld\n", wr_id);
                    abort();
                }
                if (tx_measure_global[wr_id].has_value()) {
                    printf("duplicated tx wr_id: %ld\n", wr_id);
                }

                tx_measure_global[wr_id] = measure;
                auto key = std::make_pair<>(measure.count, measure.cmd);
                if (count_cmd_list.find(key) == count_cmd_list.end()) {
                    count_cmd_list[key] = 0;
                }
                count_cmd_list[key] += 1;
            }
        }

#ifdef PROTOCOL_MEASURE
        for (auto wr_id = workers_rx[worker_id]->wr_id_start;
             wr_id < workers_rx[worker_id]->wr_id_end; ++wr_id) {
            auto measure =
                workers_rx[worker_id]
                    ->rx_measure[wr_id - workers_rx[worker_id]->wr_id_start];
            if (measure.timestamp) {
                if (wr_id >= tx_measure_global.size()) {
                    printf("unknwon wr_id 2: %ld\n", wr_id);
                    abort();
                }

                if (rx_measure_global[wr_id].has_value()) {
                    printf("duplicated rx wr_id: %ld\n", wr_id);
                }
                rx_measure_global[wr_id] = measure;
            }
        }
#endif
    }

    uint64_t dropped = 0;
    std::vector<uint64_t> dropped_wr_id;
    latency_list_t latencies;

    std::map<std::pair<uint64_t, uint8_t>, latency_list_t> latencies_count_cmd;

    for (auto &key : count_cmd_list) {
        latencies_count_cmd[key.first] = latency_list_t();
        latencies_count_cmd[key.first].reserve(key.second);
    }

    result_map_t result_map;
    result_map.reserve(aconfig.num_operation);
    latencies.reserve(aconfig.num_operation);
    for (uint64_t wr_id = 0; wr_id < tx_measure_global.size(); ++wr_id) {
        auto &tx = tx_measure_global[wr_id];
        auto &rx = rx_measure_global[wr_id];

        if (wr_id != result_map.size()) {
            printf("unknown error wr_id: %lu vs %lu\n", wr_id,
                   result_map.size());
            abort();
        }

        if (!tx.has_value()) {
            result_map.push_back(std::nullopt);
            continue;
        }

        if (!rx.has_value()) {
            dropped_wr_id.emplace_back(wr_id);
            ++dropped;
            result_map.push_back(std::nullopt);
            continue;
        }
        result_t res;
        res.lg_lat = rx->timestamp - tx->timestamp;
        res.lg_cmd = tx->cmd;
        res.lg_idx = tx->idx;
        res.lg_count = tx->count;
#ifdef PROTOCOL_BREAKDOWN
        res.dp_queued = rx->breakdown.queued;
        res.dp_worker = rx->breakdown.worker;
        res.dp_pf = rx->breakdown.sum_pf;
        res.dp_polling = rx->breakdown.polling_pf;
        res.dp_pf_cnt = rx->breakdown.num_pf;
        res.dp_pf_major_cnt = rx->breakdown.major_pf;
        res.dp_num_pending = rx->breakdown.num_pending;
        res.dp_num_enqueue = rx->breakdown.num_enqueue;
        res.dp_prev_wr_id = rx->breakdown.prev_wr_id;
        res.dp_num_enqueue = rx->breakdown.num_enqueue;
        res.dp_num_pending_local = rx->breakdown.num_pending_local;
        res.dp_worker_id = rx->breakdown.worker_id;
#endif
        result_map.push_back(res);

        auto key = std::make_pair<>(tx->count, tx->cmd);
        latencies.emplace_back(rx->timestamp - tx->timestamp, wr_id);
        latencies_count_cmd[key].emplace_back(rx->timestamp - tx->timestamp,
                                              wr_id);
    }

    printf("[Result] dropped: %ld\n", dropped);
    if (dropped_wr_id.size() < 100) {
        printf("Dropped wr_id:\n");
        for (auto &wr_id : dropped_wr_id) {
            printf("  %lu\n", wr_id);
        }
    } else {
        printf("Dropped wr_id (last 10):\n");
        for (auto it = dropped_wr_id.end() - 10; it != dropped_wr_id.end();
             ++it) {
            printf("  %lu\n", *it);
        }
    }

#ifdef PROTOCOL_BREAKDOWN

    for (auto &kv : result_map) {
        if (kv.has_value()) {
            sum_pending(result_map, kv->dp_prev_wr_id, kv->dp_num_pending,
                        kv->dp_prev_total_worker, kv->dp_prev_total_pf,
                        kv->dp_prev_total_polling);
        }
    }

#endif

    std::sort(latencies.begin(), latencies.end());
    if (!aconfig.out_path.empty()) {
        std::filesystem::path fpath(aconfig.out_path);
        {
            std::ofstream out(fpath / "output.csv");
            write_csv_to_out(out, latencies, result_map, aconfig.sample);
            out.close();
        }
        for (auto &kv : latencies_count_cmd) {
            std::ofstream out(fpath /
                              (std::string("output-") +
                               std::to_string(kv.first.first) + "-" +
                               std::to_string(kv.first.second) + ".csv"));
            write_csv_to_out(out, kv.second, result_map, aconfig.sample);
            out.close();
        }
    }

#endif

    if (!aconfig.out_path.empty()) {
        std::filesystem::path fpath(aconfig.out_path);

        std::ofstream out(fpath / "report.txt");
        out << "[REPORT]\n";
        out << "host_cps = " << cycles_per_sec << std::endl;
        out << "dev_name = " << aconfig.dev_name << std::endl;
        out << "ib_port = " << aconfig.ib_port << std::endl;
        out << "ib_num_rx_cq_desc = " << aconfig.ib_num_rx_cq_desc << std::endl;
        out << "ib_num_rx_wr_desc = " << aconfig.ib_num_rx_wr_desc << std::endl;
        out << "ib_max_entry_size = " << aconfig.ib_max_entry_size << std::endl;
        out << "core_ids = " << aconfig.core_ids_str() << std::endl;
        out << "core_id_start = " << aconfig.core_id_start << std::endl;
        out << "core_num = " << aconfig.core_num << std::endl;
        out << "num_operation = " << aconfig.num_operation << std::endl;
        out << "rps = " << aconfig.rps << std::endl;
        out << "workload = " << aconfig.workload << std::endl;
        out << "receiver_mac = " << aconfig.receiver_mac_str() << std::endl;
        out << "loadgen_mac_prefix = " << aconfig.loadgen_mac_prefix_str()
            << std::endl;
        out << "receiver_ip = " << aconfig.receiver_ip_str() << std::endl;
        out << "loadgen_ip_prefix = " << aconfig.loadgen_ip_prefix_str()
            << std::endl;
        out << "wl_app_memsize = " << aconfig.wl_app_memsize << std::endl;
        out << "wl_app_count = " << aconfig.wl_app_count << std::endl;

#ifdef PROTOCOL_MEASURE
        out << "dropped = " << dropped << std::endl;
#endif
        for (worker_id_t worker_id = 0; worker_id < num_tx_threads;
             ++worker_id) {
            out << "[WORKER_" << std::to_string(worker_id) << "]\n";
            uint64_t avg = workers_tx[worker_id]->avg_tx_ns();
            uint64_t tx_first, tx_last, rx_first, rx_last;

            workers_tx[worker_id]->first_and_last_ts(tx_first, tx_last);
            workers_rx[worker_id]->first_and_last_ts(rx_first, rx_last);

            out << "avg_tx_ns = " << avg << std::endl;
            out << "target_rps = " << workers_tx[worker_id]->target_rps
                << std::endl;
            out << "tx_first = " << tx_first << std::endl;
            out << "tx_last = " << tx_last << std::endl;
            out << "rx_first = " << rx_first << std::endl;
            out << "rx_last = " << rx_last << std::endl;
        }

        out.close();
    }

    printf("terminating...\n");

    return 0;
}
