#pragma once
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

static const size_t ROCKSDB_KEYSPACE = 38000000;
static const size_t ROCKSDB_KEYLEN = 50;
static const size_t ROCKSDB_SCAN_VALLEN = 1024;
static const size_t ROCKSDB_FIXED_PREFIX = 16;

static inline void rocksdb_gen_key_init(char *buffer, size_t buffer_size) {
    memset(buffer, 'x', buffer_size - 1);
    buffer[buffer_size - 1] = 0;
}

static inline void rocksdb_gen_key(char *buffer, size_t buffer_size,
                                   size_t key_num) {
    snprintf(buffer, buffer_size, "key%010ld", key_num);
    size_t k = strlen(buffer);
    if (k < buffer_size) {
        buffer[k] = 'x';
    }
}

static inline void rocksdb_gen_value_init(char *buffer, size_t buffer_size) {
    memset(buffer, 'x', buffer_size - 1);
    buffer[buffer_size - 1] = 0;
}

static inline void rocksdb_gen_value(char *buffer, size_t buffer_size,
                                     size_t value_num) {
    snprintf(buffer, buffer_size, "value%010ld", value_num);
    size_t k = strlen(buffer);
    if (k < buffer_size) {
        buffer[k] = 'x';
    }
}
#ifdef __cplusplus
}
#endif