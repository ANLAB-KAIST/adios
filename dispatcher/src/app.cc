
#include <ddc/mman.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include <config/static-config.hh>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dispatcher/app.hh>

extern "C" {
#define DP_ADDON_UNIMPL \
    printf("unimpl\n"); \
    abort();            \
    return 0;

int dp_addon_rocksdb_init(const dispatcher::job_adapter_t::init_t &config);
int dp_addon_rocksdb_scan(dispatcher::req_scan_t *req, size_t len);

int dp_addon_memcached_init(char *mem_base, size_t mem_size, size_t key_range,
                            size_t key_size, size_t value_size);
int dp_addon_memcached_get(req_get_t *req, size_t len);

#ifdef ENABLE_FAISS
int dp_addon_faiss_init(const std::string path);
int dp_addon_faiss_query(const float vec_in[], uint64_t vlen);
#else
int dp_addon_faiss_init(const std::string path) { DP_ADDON_UNIMPL }
int dp_addon_faiss_query(const float vec_in[], uint64_t vlen) {
    DP_ADDON_UNIMPL
}
#endif

#ifdef ENABLE_SILO
int dp_addon_silo_init(size_t app_mem_size, size_t num_warehouses,
                       size_t num_workers);
int dp_addon_silo_init_per_worker(uint8_t worker_id);
int dp_addon_silo_fn(uint8_t command);
#else
int dp_addon_silo_init(size_t app_mem_size, size_t num_warehouses,
                       size_t num_workers) {
    DP_ADDON_UNIMPL
}
int dp_addon_silo_init_per_worker(uint8_t worker_id) { DP_ADDON_UNIMPL }
int dp_addon_silo_fn(uint8_t command) { DP_ADDON_UNIMPL }

#endif
}

namespace dispatcher {

static uint64_t *array64_region = nullptr;
static size_t array64_region_len = 0;

int job_adapter_t::init_global(const init_t &config) {
#ifdef DISABLE_MAP_DDC
    // for Linux, lock currently allocated DSes

    int mlock_ret = mlockall(MCL_CURRENT);

    if (mlock_ret != 0) {
        return 1;
    }

#endif

    void *app_mem = nullptr;
    if (config.app_mem_size > 0 &&
        (config.array64_enable || config.memcached_enable)) {
        printf("CREATEING SHARED MEM\n");

        app_mem = mmap(NULL, config.app_mem_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_DDC, -1, 0);
        printf("APP MEM: %p\n", app_mem);
        printf("POPPULATING SHARED MEM\n");

        if (app_mem == MAP_FAILED) {
            return 1;
        }
    }
    if (config.array64_enable) {
        array64_region = (uint64_t *)app_mem;
        array64_region_len = config.app_mem_size;

        uint64_t base = 10000;

        uint64_t max_idx = config.app_mem_size / sizeof(uint64_t);

        for (uint64_t idx = 0; idx < max_idx; ++idx) {
            array64_region[idx] = base + idx;
        }

        for (uint64_t idx = 0; idx < max_idx; ++idx) {
            if (array64_region[idx] != base + idx) {
                return 1;
            }
        }

        printf("POPPULATING Done\n");
    }

    if (config.rocksdb_enable) {
        if (!config.rocksdb_path.empty()) {
            printf("RUNNING ROCKSDB INIT + DATA LOADING\n");
            int ret = dp_addon_rocksdb_init(config);
            if (ret) {
                return 1;
            }
            printf("LOADING ROCKSDB OK\n");
        }
    }

    if (config.memcached_enable) {
        printf("RUNNING MEMCACHED INIT + DATA LOADING\n");
        int ret = dp_addon_memcached_init((char *)app_mem, config.app_mem_size,
                                          config.key_range, config.key_size,
                                          config.value_size);
        printf("LOADING MEMCACHED OK\n");
        if (ret) {
            return 1;
        }
    }


    if (!config.faiss_path.empty()) {
        printf("RUNNING FAISS INIT + DATA LOADING\n");
        int ret = dp_addon_faiss_init(config.faiss_path);
        printf("LOADING FAISS OK\n");
        if (ret) {
            return 1;
        }
    }

    if (config.silo_enable) {
        printf("RUNNING SILO INIT + DATA LOADING\n");
        int ret = dp_addon_silo_init(config.app_mem_size, config.num_warehouses,
                                     config.num_workers);
        printf("LOADING SILO OK\n");
        if (ret) {
            return 1;
        }
    }

#ifdef DISABLE_MAP_DDC
    // for Linux, lock all after this moment
    mlock_ret = mlockall(MCL_FUTURE);

    if (mlock_ret != 0) {
        return 1;
    }

#endif
    return 0;
}

int job_adapter_t::init_per_worker(uint8_t worker_id, const init_t &config) {

    if (config.silo_enable) {
        int ret = dp_addon_silo_init_per_worker(worker_id);
        if (ret) {
            return 1;
        }
    }
    return 0;
}

int job_adapter_t::ping(req_ping_t *req, size_t len) {
    // int ret = 0;
    // ret = sched();

    // printf("after sched, ret: %d\n", ret);
    return 0;
}
int job_adapter_t::array64(req_array64_t *req, size_t len) {
    resp_array64_t *resp = (resp_array64_t *)req;

    for (uint64_t i = 0; i < req->num_idx; ++i) {
        uint64_t idx = req->idx[i];
        resp->value[i] = array64_region[idx];  // lookup
    }

    // num req == num resp, do not need to update
    return sizeof(resp_array64_t) + sizeof(uint64_t) * resp->num_value;
}

int job_adapter_t::warray64(req_warray64_t *req, size_t len) {
    // resp_warray64_t *resp = (resp_warray64_t *)req;

    for (uint64_t i = 0; i < req->num_idx; ++i) {
        uint64_t idx = req->payload[i].idx;
        array64_region[idx] = req->payload[i].value;  // write
    }

    // num req == num resp, do not need to update
    return sizeof(resp_array64_t);
}

int job_adapter_t::scan(req_scan_t *req, size_t len) {
    return dp_addon_rocksdb_scan(req, len);
}
int job_adapter_t::get(req_get_t *req, size_t len) {
    return dp_addon_memcached_get(req, len);
}


int job_adapter_t::vsearch(req_vsearch_t *req, size_t len) {
    int ret = dp_addon_faiss_query(req->vec, req->vlen);
    resp_vsearch_t *resp = (resp_vsearch_t *)req;
    resp->ret = ret;
    return sizeof(resp_vsearch_t);
}

int job_adapter_t::tpcc(req_tpcc_t *req, size_t len) {
    int ret = dp_addon_silo_fn(req->cmd);
    resp_tpcc_t *resp = (resp_tpcc_t *)req;
    resp->ret = ret;
    return sizeof(resp_tpcc_t);
}
}  // namespace dispatcher