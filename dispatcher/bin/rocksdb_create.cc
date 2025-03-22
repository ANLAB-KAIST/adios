
#include <dispatcher/cycles.h>
#include <protocol/rocksdb.h>
#include <rocksdb/db.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <unistd.h>

#include <config/rocksdb.hh>

static rocksdb::DB *db;
constexpr size_t rocksdb_write_batch_size = 128;

int main() {
    auto options = get_rocksdb_option();

    rocksdb::Status status = rocksdb::DB::Open(options, "/mnt/rocksdb", &db);
    if (!status.ok()) {
        return 1;
    }

    // Put key-value
    rocksdb::WriteOptions writeoptions;

    char key[ROCKSDB_KEYLEN + 1];
    char value[ROCKSDB_SCAN_VALLEN + 1];
    key[ROCKSDB_KEYLEN + 1] = 0;
    value[ROCKSDB_SCAN_VALLEN + 1] = 0;
    rocksdb_gen_key_init(key, ROCKSDB_KEYLEN + 1);
    rocksdb_gen_value_init(value, ROCKSDB_SCAN_VALLEN + 1);

    for (size_t i = 0; i < ROCKSDB_KEYSPACE;) {
        size_t this_end = i + rocksdb_write_batch_size;

        if (ROCKSDB_KEYSPACE < this_end) {
            this_end = ROCKSDB_KEYSPACE;
        }
        rocksdb::WriteBatch batch;
        for (; i < this_end; ++i) {
            if (i % 2000000 == 0) {
                printf("i: %ld\n", i);
            }
            rocksdb_gen_key(key, ROCKSDB_KEYLEN + 1, i);
            rocksdb_gen_value(value, ROCKSDB_SCAN_VALLEN + 1, i);

            auto st = batch.Put(rocksdb::Slice(key, strlen(key)),
                                rocksdb::Slice(value, strlen(value)));
            if (!st.ok()) {
                return 1;
            }
        }

        auto st_write = db->Write(writeoptions, &batch);
        if (!st_write.ok()) {
            return 1;
        }
    }

    db->Close();

    return 0;
}
