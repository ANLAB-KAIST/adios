#pragma once

#include <protocol/req.hh>
#include <string>

namespace dispatcher {

class job_adapter_t {
   public:
    struct init_t {
        size_t app_mem_size = 0;
        int array64_enable;
        int rocksdb_enable;
        int memcached_enable;
        int silo_enable;
        std::string rocksdb_addon_path;
        std::string rocksdb_path;
        size_t key_range;
        size_t key_size;
        size_t value_size;
        size_t num_warehouses;
        size_t num_workers;
        std::string gapbs_path;
        std::string faiss_path;
    };
    static int init_global(const init_t &config);
    static int init_per_worker(uint8_t worker_id, const init_t &config);

   protected:
    int sched();  // rarely used

    // command return:
    //   * positive or zero -> response length
    //   * negative -> error
    int ping(req_ping_t *req, size_t len);
    int array64(req_array64_t *req, size_t len);
    int warray64(req_warray64_t *req, size_t len);
    int get(req_get_t *req, size_t len);
    int scan(req_scan_t *req, size_t len);
    int tpcc(req_tpcc_t *req, size_t len);
    int vsearch(req_vsearch_t *req, size_t len);
};
}  // namespace dispatcher