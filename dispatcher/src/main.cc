
#include <algorithm>
#include <array>
#include <config/arg-config.hh>
#include <config/static-config.hh>
#include <dispatcher/api.hh>
#include <dispatcher/dispatcher.hh>
#include <dispatcher/loader.hh>

struct arg_config_t {
    ARG_STR(eth_dev_name, "ETH IB device name. Example: mlx5_0");
    ARG_STR(ib_dev_name, "IB device name. Example: mlx5_0");
    ARG_INT_DEFAULT(eth_port, 1, "eth NIC port. Default: 1");
    ARG_INT_LIST_CUSTOM(eth_mac,
                        "eth mac address. Example: AA:BB:CC:DD:EE:FF. Default: "
                        "00:01:02:03:04:05",
                        ':', 16, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5);
    dispatcher::eth_addr_t _eth_mac;
    ARG_INT_LIST_CUSTOM(
        eth_ip, "eth ip address. Example: 192.168.0.1. Default: 192.168.2.200",
        '.', 10, 192, 168, 2, 200);
    dispatcher::ip_addr_t _eth_ip;
    ARG_INT_DEFAULT(ib_port, 1, "IB port number. Default:1");
    ARG_INT_DEFAULT(ib_gid_index, 1, "IB GID index. Default:1");

    ARG_STR(server_hostname, "memory server hostname");
    ARG_STR_DEFAULT(server_port, "12345",
                    "memory server port number. Default:12345");

    ARG_INT_DEFAULT(dispatcher_core_id, 1, "core ID for dispatcher. Default:1");
    ARG_INT_LIST(worker_core_ids, "worker core IDs");
    ARG_INT_DEFAULT(worker_core_id_start, -1,
                    "worker core ID start. Prefer worker_core_ids");
    ARG_INT_DEFAULT(worker_core_num, -1,
                    "worker core ID end. Prefer worker_core_ids");
    ARG_INT_DEFAULT(reclaimer_core_id, -1,
                    "reclaimer core ID. Default: after last worker");
    ARG_INT_DEFAULT(reclaimer_start_percent, 10,
                    "reclaimer start percent. Default: 10");
    ARG_INT_DEFAULT(reclaimer_end_percent, 15,
                    "reclaimer end percent. Default: 15");
    ARG_INT_DEFAULT(dp_mode, 0, "dp handling mode. Default: 0");

    ARG_INT_DEFAULT(pf_mode, 0,
                    "pf handling mode. 0 for no-pf, 1 for sync pf, 2 for async "
                    "pf. Default: 0");
    ARG_INT_DEFAULT(sched_mode, 0, "Default: 0");

    ARG_INT_DEFAULT(
        preemption_cycles, 0,
        "preemption_cycles for Preeptible scheduling. 0 for cooperatinve, "
        "positive values for preeption cycles. Default: 0");

    ARG_INT(app_memsize, "app memory size in GB");
    ARG_INT_DEFAULT(app_array64_enable, 0, "Enable Array64");
    ARG_INT_DEFAULT(app_rocksdb_enable, 0, "Enable RocksDB");
    ARG_INT_DEFAULT(app_memcached_enable, 0, "Enable Memcached");
    ARG_INT_DEFAULT(app_silo_enable, 0, "Enable Silo");
    ARG_INT_DEFAULT(app_num_warehouses, 576, "Num Silo warehouses");
    ARG_STR_DEFAULT(app_rocksdb_addon, "", "RocksDB addon path.");
    ARG_STR_DEFAULT(app_rocksdb_path, "/mnt/rocksdb",
                    "RocksDB path. Default: none");
    ARG_INT_DEFAULT(app_key_range, 200000000, "key_range. Default: 200000000");
    // 200000000
    ARG_INT_DEFAULT(app_key_size, 16, "key_size. Default: 8");
    ARG_INT_DEFAULT(app_value_size, 128, "value_size. Default: 128");

    ARG_STR_DEFAULT(app_faiss_path, "", "faiss path");

    int parse(int argc, char **argv) {
        ARG_PARSE_START(argc, argv);
        ARG_PARSE(eth_dev_name);
        ARG_PARSE(ib_dev_name);
        ARG_PARSE(eth_port);
        ARG_PARSE(eth_mac);
        ARG_PARSE(eth_ip);
        ARG_PARSE(ib_port);
        ARG_PARSE(ib_gid_index);
        ARG_PARSE(server_hostname);
        ARG_PARSE(server_port);
        ARG_PARSE(dispatcher_core_id);
        ARG_PARSE(worker_core_ids);
        ARG_PARSE(worker_core_id_start);
        ARG_PARSE(worker_core_num);
        ARG_PARSE(reclaimer_core_id);
        ARG_PARSE(reclaimer_start_percent);
        ARG_PARSE(reclaimer_end_percent);
        ARG_PARSE(dp_mode);
        ARG_PARSE(pf_mode);
        ARG_PARSE(sched_mode);
        ARG_PARSE(preemption_cycles);
        ARG_PARSE(app_memsize);
        ARG_PARSE(app_array64_enable);
        ARG_PARSE(app_rocksdb_enable);
        ARG_PARSE(app_memcached_enable);
        ARG_PARSE(app_silo_enable);
        ARG_PARSE(app_num_warehouses);
        ARG_PARSE(app_rocksdb_addon);
        ARG_PARSE(app_rocksdb_path);
        ARG_PARSE(app_key_range);
        ARG_PARSE(app_key_size);
        ARG_PARSE(app_value_size);
        ARG_PARSE(app_faiss_path);
        ARG_PARSE_END();

        if (eth_dev_name.empty()) {
            printf("No eth_dev_name.\n");
            return 1;
        }
        if (ib_dev_name.empty()) {
            printf("No ib_dev_name.\n");
            return 1;
        }
        if (eth_mac.empty()) {
            printf("No eth_mac.\n");
            return 1;
        }
        if (eth_ip.empty()) {
            printf("No eth_ip.\n");
            return 1;
        }

        if (ib_port == 0) {
            printf("No ib_port.\n");
            return 1;
        }
        if (ib_gid_index == 0) {
            printf("No ib_gid_index.\n");
            return 1;
        }

        if (pf_mode != dispatcher::dispatcher_t::PF_MODE_NONE &&
            pf_mode != dispatcher::dispatcher_t::PF_MODE_SYNC_SJK2_NONE) {
            // only remote pf handlers require remote mem server
            if (server_hostname.empty()) {
                printf("No server_hostname.\n");
                return 1;
            }
            if (server_port.empty()) {
                printf("No server_port.\n");
                return 1;
            }
        }
        if (eth_mac.size() != 6) {
            printf("MAC parsing error: eth_mac\n");
            return 1;
        }
        for (int i = 0; i < 6; ++i) {
            unsigned int ul = eth_mac[i];
            if (ul > 255) {
                printf("MAC parsing error: eth_mac\n");
                return 1;
            }
            _eth_mac[i] = ul;
        }
        if (eth_ip.size() != 4) {
            printf("IP parsing error: eth_ip\n");
            return 1;
        }
        for (int i = 0; i < 4; ++i) {
            unsigned int ul = eth_ip[i];
            if (ul > 255) {
                printf("MAC parsing error: eth_ip\n");
                return 1;
            }
            _eth_ip[i] = ul;
        }

        if (worker_core_ids.empty()) {
            if (worker_core_id_start < 0 || worker_core_num < 1) {
                printf(
                    "No worker_core_ids or wrong "
                    "worker_core_id_start/worker_core_num\n");
                return 1;
            }

            for (long long id = worker_core_id_start;
                 id < worker_core_num + worker_core_id_start; ++id) {
                worker_core_ids.push_back(id);
            }
        }
        std::sort(worker_core_ids.begin(), worker_core_ids.end());

        if (reclaimer_core_id == -1) {
            reclaimer_core_id = worker_core_ids.back() + 1;
        }
        return 0;
    }

    static std::string usage(int argc, char **argv) {
        ARG_USAGE_START(argc, argv);
        ARG_USAGE(eth_dev_name);
        ARG_USAGE(ib_dev_name);
        ARG_USAGE(eth_port);
        ARG_USAGE(eth_mac);
        ARG_USAGE(eth_ip);
        ARG_USAGE(ib_port);
        ARG_USAGE(ib_gid_index);
        ARG_USAGE(server_hostname);
        ARG_USAGE(server_port);
        ARG_USAGE(dispatcher_core_id);
        ARG_USAGE(worker_core_ids);
        ARG_USAGE(worker_core_id_start);
        ARG_USAGE(worker_core_num);
        ARG_USAGE(reclaimer_core_id);
        ARG_USAGE(reclaimer_start_percent);
        ARG_USAGE(reclaimer_end_percent);
        ARG_USAGE(dp_mode);
        ARG_USAGE(pf_mode);
        ARG_USAGE(sched_mode);
        ARG_USAGE(preemption_cycles);
        ARG_USAGE(app_memsize);
        ARG_USAGE(app_array64_enable);
        ARG_USAGE(app_rocksdb_enable);
        ARG_USAGE(app_memcached_enable);
        ARG_USAGE(app_silo_enable);
        ARG_USAGE(app_num_warehouses);
        ARG_USAGE(app_rocksdb_addon);
        ARG_USAGE(app_rocksdb_path);
        ARG_USAGE(app_key_range);
        ARG_USAGE(app_key_size);
        ARG_USAGE(app_value_size);
        ARG_USAGE(app_faiss_path);

        return ARG_USAGE_END();
    }
    std::string str() {
        ARG_PRINT_START();
        ARG_PRINT(eth_dev_name);
        ARG_PRINT(ib_dev_name);
        ARG_PRINT(eth_port);
        ARG_PRINT(eth_mac);
        ARG_PRINT(eth_ip);
        ARG_PRINT(ib_port);
        ARG_PRINT(ib_gid_index);
        ARG_PRINT(server_hostname);
        ARG_PRINT(server_port);
        ARG_PRINT(dispatcher_core_id);
        ARG_PRINT(worker_core_ids);
        ARG_PRINT(worker_core_id_start);
        ARG_PRINT(worker_core_num);
        ARG_PRINT(reclaimer_core_id);
        ARG_PRINT(reclaimer_start_percent);
        ARG_PRINT(reclaimer_end_percent);
        ARG_PRINT(dp_mode);
        ARG_PRINT(pf_mode);
        ARG_PRINT(sched_mode);
        ARG_PRINT(preemption_cycles);
        ARG_PRINT(app_memsize);
        ARG_PRINT(app_array64_enable);
        ARG_PRINT(app_rocksdb_enable);
        ARG_PRINT(app_memcached_enable);
        ARG_PRINT(app_silo_enable);
        ARG_PRINT(app_num_warehouses);
        ARG_PRINT(app_rocksdb_addon);
        ARG_PRINT(app_rocksdb_path);
        ARG_PRINT(app_key_range);
        ARG_PRINT(app_key_size);
        ARG_PRINT(app_value_size);
        ARG_PRINT(app_faiss_path);
        return ARG_PRINT_END();
    }
};

extern "C" {
int enable_map_ddc = 1;
}

extern "C" {
int do_dispatcher_main(int argc, char **argv) {
    int ret = 0;
    dispatcher::dispatcher_t::init_t config;

    {
        arg_config_t aconfig;
        int parse_ret = aconfig.parse(argc, argv);
        printf("[Configuration]\n\ncmd:%s\n%s\n", argv[0],
               aconfig.str().c_str());

        if (parse_ret) {
            auto usage = arg_config_t::usage(argc, argv);
            printf("\n\nUsage: %s\n", usage.c_str());
            return 1;
        }

        config.dev.eth_name = aconfig.eth_dev_name;
        config.dev.ib_name = aconfig.ib_dev_name;

        config.eth.port_num = aconfig.eth_port;

        config.eth.config.ip_addr = aconfig._eth_ip;
        config.eth.config.mac_addr = aconfig._eth_mac;
        if (aconfig.pf_mode == dispatcher::dispatcher_t::PF_MODE_NONE ||
            aconfig.pf_mode ==
                dispatcher::dispatcher_t::PF_MODE_SYNC_SJK2_NONE) {
            config.mem_server.disable = true;
        }

        config.mem_server.rdma_port_num = aconfig.ib_port;
        config.mem_server.sgid_idx = aconfig.ib_gid_index;
        config.mem_server.hostname = aconfig.server_hostname;
        config.mem_server.port = aconfig.server_port;

        config.dispatcher.core_id = aconfig.dispatcher_core_id;
        for (auto &core_id : aconfig.worker_core_ids) {
            config.worker.core_ids.push_back(core_id);
        }
        config.reclaimer.core_id = aconfig.reclaimer_core_id;
        config.reclaimer.start_percent = aconfig.reclaimer_start_percent;
        config.reclaimer.end_percent = aconfig.reclaimer_end_percent;

        config.pf_mode = aconfig.pf_mode;
        config.dp_mode = aconfig.dp_mode;
        config.sched_mode = aconfig.sched_mode;
        config.preemption_cycles = aconfig.preemption_cycles;

        if (aconfig.pf_mode != dispatcher::dispatcher_t::PF_MODE_NONE &&
            aconfig.pf_mode !=
                dispatcher::dispatcher_t::PF_MODE_SYNC_SJK2_NONE) {
            config.dispatcher.rdma_sync_depth = max_req_init;
            config.worker.rdma_async_depth = max_req_init;
            config.worker.rdma_sync_depth = max_req_init;
        } else {
            config.dispatcher.rdma_sync_depth = 0;
            config.worker.rdma_async_depth = 0;
            config.worker.rdma_sync_depth = 0;
        }

        config.app.app_mem_size = aconfig.app_memsize << 30;
        config.app.array64_enable = aconfig.app_array64_enable;
        config.app.rocksdb_enable = aconfig.app_rocksdb_enable;
        config.app.memcached_enable = aconfig.app_memcached_enable;
        config.app.silo_enable = aconfig.app_silo_enable;
        config.app.num_warehouses = aconfig.app_num_warehouses;
        config.app.num_workers = aconfig.worker_core_ids.size();
        config.app.rocksdb_addon_path = aconfig.app_rocksdb_addon;
        config.app.rocksdb_path = aconfig.app_rocksdb_path;
        config.app.key_range = aconfig.app_key_range;
        config.app.key_size = aconfig.app_key_size;
        config.app.value_size = aconfig.app_value_size;

        config.app.faiss_path = aconfig.app_faiss_path;
    }

    {
        auto phys = dispatcher::list_phys();
        if (phys.empty()) {
            printf("list_phys failed\n");
            return 1;
        }

        config.mm.addrs = phys;
    }

    ret = dispatcher::default_dispatcher.init(config);
    if (ret) {
        printf("ERR INIT\n");
        return ret;
    }

    ret = dispatcher::default_dispatcher.run();

    return ret;
}
}