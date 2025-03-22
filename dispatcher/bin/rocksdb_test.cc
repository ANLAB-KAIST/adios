
#include <unistd.h>

#include <dispatcher/app.hh>

extern "C" {
int enable_map_ddc = 1;
}

extern "C" int dp_addon_rocksdb_init(
    const dispatcher::job_adapter_t::init_t &config);
int main() {
    dispatcher::job_adapter_t::init_t config;
    config.rocksdb_path = "/tmp/rocksdb";
    config.key_range = 200000000;
    config.key_size = 16;
    config.value_size = 128;
    int ret = dp_addon_rocksdb_init(config);

    if (ret) {
        printf("ret: %d\n", ret);
        return 1;
    }
    sleep(3600);
    return 0;
}

namespace dispatcher {
void handle_preempt_requested(unsigned &my_cpu_id) {}

unsigned get_cpu_id() { return 0; }
}  // namespace dispatcher