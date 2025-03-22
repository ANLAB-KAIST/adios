#include "../deps/memcached/memcached.h"

#include <dispatcher/likely.h>
#include <protocol/memcached.h>
#include <protocol/rocksdb.h>
#include <stdlib.h>
#include <string.h>

void event_set(struct event *a, evutil_socket_t b, short c,
               void (*d)(evutil_socket_t, short, void *), void *e) {}

int event_add(struct event *ev, const struct timeval *timeout) { return -1; }

int event_del(struct event *a) { return -1; }
int event_base_set(struct event_base *a, struct event *b) { return -1; }

void event_base_free(struct event_base *a) {}
int event_base_loop(struct event_base *a, int b) { return -1; }
const char *event_get_version(void) { return "unimpl"; }
int event_config_set_flag(struct event_config *cfg, int flag) { return -1; }
void event_config_free(struct event_config *cfg) {}

int event_base_loopexit(struct event_base *a, const struct timeval *b) {
    return -1;
}

struct event_base *event_base_new_with_config(const struct event_config *a) {
    return NULL;
}

struct event_config *event_config_new(void) { return NULL; }

static void settings_init(void) {
    settings.use_cas = false;
    settings.access = 0700;
    settings.port = 11211;
    settings.udpport = 0;
    /* By default this string should be NULL for getaddrinfo() */
    settings.inter = NULL;
    settings.maxbytes = 64 * 1024 * 1024; /* default is 64MB */
    settings.maxconns =
        1024; /* to limit connections-related memory to about 5MB */
    settings.verbose = 0;
    settings.oldest_live = 0;
    settings.oldest_cas = 0; /* supplements accuracy of oldest_live */
    settings.evict_to_free =
        1; /* push old items out of cache when memory runs out */
    settings.socketpath = NULL; /* by default, not using a unix socket */
    settings.auth_file =
        NULL; /* by default, not using ASCII authentication tokens */
    settings.factor = 1.25;
    settings.chunk_size = 48; /* space for a modest key and value */
    settings.num_threads = 4; /* N workers */
    settings.num_threads_per_udp = 0;
    settings.prefix_delimiter = ':';
    settings.detail_enabled = 0;
    settings.reqs_per_event = 20;
    settings.backlog = 1024;
    settings.binding_protocol = negotiating_prot;
    settings.item_size_max = 1024 * 1024; /* The famous 1MB upper limit. */
    settings.slab_page_size =
        1024 * 1024; /* chunks are split from 1MB pages. */
    settings.slab_chunk_size_max = settings.slab_page_size / 2;
    settings.sasl = false;
    settings.maxconns_fast = true;
    settings.lru_crawler = false;
    settings.lru_crawler_sleep = 100;
    settings.lru_crawler_tocrawl = 0;
    settings.lru_maintainer_thread = false;
    settings.lru_segmented = false;
    settings.hot_lru_pct = 20;
    settings.warm_lru_pct = 40;
    settings.hot_max_factor = 0.2;
    settings.warm_max_factor = 2.0;
    settings.temp_lru = false;
    settings.temporary_ttl = 61;
    settings.idle_timeout = 0; /* disabled */
    settings.hashpower_init = 0;
    settings.slab_reassign = true;
    settings.slab_automove = 1;
    settings.slab_automove_ratio = 0.8;
    settings.slab_automove_window = 30;
    settings.shutdown_command = false;
    settings.tail_repair_time = TAIL_REPAIR_TIME_DEFAULT;
    settings.flush_enabled = true;
    settings.dump_enabled = true;
    settings.crawls_persleep = 1000;
    settings.logger_watcher_buf_size = LOGGER_WATCHER_BUF_SIZE;
    settings.logger_buf_size = LOGGER_BUF_SIZE;
    settings.drop_privileges = false;
    settings.watch_enabled = true;
    settings.read_buf_mem_limit = 0;
    settings.num_napi_ids = 0;
    settings.memory_file = NULL;
}

__thread LIBEVENT_THREAD th;

enum store_item_type do_memcached_set(const char *key, size_t klen,
                                      const char *value, size_t vlen) {
    item *it = item_alloc(key, klen, 0, 0, vlen + 1);

    if (it == NULL) {
        return NO_MEMORY;
    }
    char *data = (char *)ITEM_data(it);
    strncpy(data, value, vlen);
    int a = sizeof(io_queue_cb_t);

    uint32_t hv;
    hv = hash(ITEM_key(it), it->nkey);
    uint64_t cas = 0;
    enum store_item_type ret =
        do_store_item(it, NREAD_SET, &th, hv, NULL, &cas, CAS_NO_STALE);
    return ret;
}

int do_memcached_get(const char *key, size_t klen, char *value, size_t vlen) {
    uint32_t hv;
    hv = hash(key, klen);

    item *it = do_item_get(key, klen, hv, &th, 0);

    if (it == NULL) {
        return 1;
    }

    const char *data = (char *)ITEM_data(it);
    strncpy(value, data, vlen);

    return 0;
}

int dp_addon_memcached_init(char *mem_base, size_t mem_size, size_t key_range,
                            size_t key_size, size_t value_size) {
    // int a = do_memcached_set("asdf", 4, "asdf", 4);
    // printf("aa:%ld\n", sizeof(LIBEVENT_THREAD));

    // precalculate hash_power
    int hash_power = 1;
#define HASHPOWER_MAX 32
#define hashsize(n) ((uint64_t)1 << (n))
    while (key_range > (hashsize(hash_power) * 3) / 2 &&
           hash_power < HASHPOWER_MAX) {
        ++hash_power;
    }
#undef hashsize
#undef HASHPOWER_MAX

    memset(&th, 0, sizeof(th));
    memset(&settings, 0, sizeof(settings));

    settings_init();

    settings.hashpower_init = hash_power;

    item_stats_reset();

    hash_init(MURMUR3_HASH);

    assoc_init(settings.hashpower_init);

    settings.maxbytes = mem_size;
    slabs_init(settings.maxbytes, settings.factor, true, NULL, mem_base, 0);

    memcached_thread_init2();
    char key[key_size + 1];
    char value[value_size + 1];
    char value2[value_size + 1];
    key[key_size + 1] = 0;
    value[value_size + 1] = 0;
    value2[value_size + 1] = 0;
    rocksdb_gen_key_init(key, key_size + 1);
    rocksdb_gen_value_init(value, value_size + 1);
    rocksdb_gen_value_init(value2, value_size + 1);

    for (size_t i = 0; i < key_range; ++i) {
        if (i % 2000000 == 0) {
            printf("i: %ld\n", i);
        }
        rocksdb_gen_key(key, key_size + 1, i);
        rocksdb_gen_value(value, value_size + 1, i);

        enum store_item_type ret =
            do_memcached_set(key, key_size, value, value_size);
        if (ret != STORED) {
            printf("i: %ld\n", i);
            return 1;
        }
    }
    printf("Checking values...\n");
    for (size_t i = 0; i < key_range; ++i) {
        if (i % 2000000 == 0) {
            printf("i: %ld\n", i);
        }
        rocksdb_gen_key(key, key_size + 1, i);
        rocksdb_gen_value(value, value_size + 1, i);

        int ret = do_memcached_get(key, key_size, value2, value_size);

        if (ret == 1 || strcmp(value, value2) != 0) {
            printf("%s vs %s\n", value, value2);
            return 1;
        }
    }

    return 0;
}

int dp_addon_memcached_init_per_thread() { memset(&th, 0, sizeof(th)); }

int dp_addon_memcached_get(struct req_get_t *req, size_t len) {
    // per job libevent

    struct resp_get_t *resp = (struct resp_get_t *)req;

    int ret = do_memcached_get(req->key, req->klen - 1, resp->value,
                               len - sizeof(struct req_get_t) - 1);

    if (gcc_unlikely(ret)) {
        resp->vlen = 1;
        resp->value[0] = 0;
    } else {
        resp->value[len - sizeof(struct req_get_t) - 1] =
            0;  // safe null terminal
        resp->vlen = strlen(resp->value) + 1;
    }

    return sizeof(struct resp_get_t) + resp->vlen;
}
