#include <protocol/rocksdb.h>
#include <rocksdb/db.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <string.h>

#include <config/rocksdb.hh>
#include <dispatcher/api.hh>
#include <dispatcher/app.hh>
#include <dispatcher/common.hh>

static rocksdb::DB *db;
constexpr size_t rocksdb_write_batch_size = 128;

extern "C" {

int dp_addon_rocksdb_init(const dispatcher::job_adapter_t::init_t &config) {
    auto options = get_rocksdb_option();
    db = nullptr;
    char key[ROCKSDB_KEYLEN + 1];
    char value[ROCKSDB_SCAN_VALLEN + 1];
    key[ROCKSDB_KEYLEN + 1] = 0;
    value[ROCKSDB_SCAN_VALLEN + 1] = 0;
    rocksdb_gen_key_init(key, ROCKSDB_KEYLEN + 1);
    rocksdb_gen_value_init(value, ROCKSDB_SCAN_VALLEN + 1);

    printf("config.rocksdb_path: %s\n", config.rocksdb_path.c_str());
    rocksdb::Status status =
        rocksdb::DB::OpenForReadOnly(options, config.rocksdb_path.c_str(), &db);
    if (!status.ok()) {
        return 1;
    }

    //
    uint64_t after = get_cycles();
    printf("CHECKING...\n");

    rocksdb::ReadOptions readoptions;
    size_t i = 0;
    auto it = db->NewIterator(readoptions);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (i % 2000000 == 0) {
            printf("i: %ld\n", i);
        }
        rocksdb_gen_key(key, ROCKSDB_KEYLEN + 1, i);
        rocksdb_gen_value(value, ROCKSDB_SCAN_VALLEN + 1, i);
        auto k = it->key();
        auto v = it->value();
        if (strncmp(key, k.data(), k.size()) != 0) {
            printf(" key mismatch: %s vs %s\n", key, k.data());
            return 1;
        }
        if (strncmp(value, v.data(), v.size()) != 0) {
            printf(" value mismatch: %s vs %s\n", value, v.data());
            return 1;
        }
        ++i;
    }
    delete it;

    uint64_t last = get_cycles();
    printf("checking: %lu\n", last - after);

    return 0;
}
int dp_addon_rocksdb_scan(dispatcher::req_scan_t *req, size_t len) {
#ifdef DILOS_ROCKSDB_CHECK_PREEMPT
    unsigned my_cpu_id = dispatcher::get_cpu_id();
#endif
    int num_iter = req->count;

    uint64_t klen = req->klen;
    char key[klen];
    memcpy(key, req->key, klen);

    size_t last_vlen = 0;
    dispatcher::resp_scan_t *resp = (dispatcher::resp_scan_t *)req;
    rocksdb::ReadOptions readoptions;
    auto it = db->NewIterator(readoptions);

    for (it->Seek(rocksdb::Slice(key, klen)); it->Valid(); it->Next()) {
        auto v = it->value();
        size_t vlen = v.size();
        memcpy(resp->values, v.data(), vlen);  // read value for PF
        resp->values[vlen] = 0;                // null term;
        vlen += 1;
        last_vlen = vlen;
        --num_iter;
        if (0 >= num_iter) break;
#ifdef DILOS_ROCKSDB_CHECK_PREEMPT
        dispatcher::handle_preempt_requested(my_cpu_id);
#endif
    }
    delete it;

    resp->count = req->count;

    return sizeof(dispatcher::resp_scan_t) + last_vlen;
}
}