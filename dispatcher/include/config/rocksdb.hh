#pragma once
#include <protocol/rocksdb.h>
#include <rocksdb/db.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>

static rocksdb::Options get_rocksdb_option() {
    rocksdb::Options options;
    rocksdb::PlainTableOptions ptOptions;

    options.allow_mmap_reads = true;
    options.allow_mmap_writes = false;
    ptOptions.user_key_len = 0;
    ptOptions.bloom_bits_per_key = 10;
    ptOptions.hash_table_ratio = 0.75;
    ptOptions.index_sparseness = 3;
    options.table_factory.reset(rocksdb::NewPlainTableFactory(ptOptions));
    options.IncreaseParallelism(0);
    options.create_if_missing = true;
    auto prefix_trans = rocksdb::NewFixedPrefixTransform(ROCKSDB_FIXED_PREFIX);
    options.prefix_extractor.reset(prefix_trans);

    return options;
}