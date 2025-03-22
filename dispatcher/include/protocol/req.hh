#pragma once

#include <protocol/memcached.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <protocol/cmd.hh>

namespace dispatcher {

constexpr uint64_t CMD_BODY_SIZE_MAX = NET_PAGE_SIZE - sizeof(cmdpkt_t);

#define ASSERT_CMD(cmd)                                        \
    static_assert(sizeof(req_##cmd##_t) < CMD_BODY_SIZE_MAX);  \
    static_assert(sizeof(resp_##cmd##_t) < CMD_BODY_SIZE_MAX); \
    static_assert(sizeof(req_##cmd##_t) >= sizeof(resp_##cmd##_t));

struct req_ping_t {};

struct resp_ping_t {};

ASSERT_CMD(ping)

struct req_array64_t {
    uint64_t num_idx;
    uint64_t idx[];
};

static_assert(sizeof(req_array64_t) == sizeof(uint64_t));

struct resp_array64_t {
    uint64_t num_value;
    uint64_t value[];
};
static_assert(sizeof(resp_array64_t) == sizeof(uint64_t));
static_assert(offsetof(req_array64_t, num_idx) ==
              offsetof(resp_array64_t, num_value));
static_assert(offsetof(req_array64_t, idx) == offsetof(resp_array64_t, value));

ASSERT_CMD(array64)

struct req_warray64_t {
    uint64_t num_idx;
    struct {
        uint64_t idx;
        uint64_t value;
    } payload[];
};

static_assert(sizeof(req_warray64_t) == sizeof(uint64_t));
static_assert(offsetof(req_warray64_t, payload) == sizeof(uint64_t));

struct resp_warray64_t {
    uint64_t num_value;
};
static_assert(sizeof(resp_warray64_t) == sizeof(uint64_t));
static_assert(offsetof(req_warray64_t, num_idx) ==
              offsetof(resp_warray64_t, num_value));

ASSERT_CMD(warray64)

struct req_scan_t {
    uint64_t count;
    uint64_t klen;  // includes NULL
    char key[];
};
constexpr size_t max_scan_key_size = CMD_BODY_SIZE_MAX - sizeof(req_scan_t);
struct resp_scan_t {
    uint64_t count;
    char values[];
    // currently only last value
};

ASSERT_CMD(scan)

using req_get_t = ::req_get_t;
constexpr size_t max_get_key_size = CMD_BODY_SIZE_MAX - sizeof(req_get_t);

using resp_get_t = ::resp_get_t;
ASSERT_CMD(get)

struct req_vsearch_t {
    uint64_t k;
    uint64_t vlen;  // includes NULL
    float vec[];
};
struct resp_vsearch_t {
    int ret;
};
ASSERT_CMD(vsearch)

struct req_tpcc_t {
    uint64_t cmd;
};
struct resp_tpcc_t {
    int ret;
};
ASSERT_CMD(tpcc)

#undef ASSERT_CMD

}  // namespace dispatcher