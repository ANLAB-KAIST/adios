#pragma once
#include <config/static-config.hh>
#include <cstdint>
#include <protocol/net.hh>

namespace dispatcher {

enum class cmd_t : uint8_t {
    none = 0,
    ping = 1,
    array64 = 2,
    scan = 3,
    get = 4,
    tpcc = 5,
    warray64 = 6,
    vsearch = 8,
};

#if defined(PROTOCOL_BREAKDOWN) && !defined(PROTOCOL_MEASURE)
#define PROTOCOL_MEASURE
#endif

#ifdef PROTOCOL_MEASURE
struct measurehdr_t {
    uint64_t lgen_wr_id;
} __attribute__((packed));
#endif

#ifdef PROTOCOL_BREAKDOWN

struct breakdownhdr_t {
    uint64_t queued;
    uint64_t worker;
    uint64_t num_pf;
    uint64_t major_pf;
    uint64_t sum_pf;
    uint64_t polling_pf;
    uint64_t num_pending;
    uint64_t num_enqueue;
    uint64_t prev_wr_id;
    uint64_t num_pending_local;
    uint8_t worker_id;
} __attribute__((packed));
#endif
struct cmdhdr_t {
    cmd_t cmd;
} __attribute__((packed));

struct cmdpkt_t {
    ethhdr_t eth;
    iphdr_t ip;
    udphdr_t udp;
#ifdef PROTOCOL_MEASURE
    measurehdr_t measure;
#endif
#ifdef PROTOCOL_BREAKDOWN
    breakdownhdr_t breakdown;
#endif
    cmdhdr_t cmd;
} __attribute__((packed));

static_assert(sizeof(cmdpkt_t) < NET_MTU_ASSUME);

}  // namespace dispatcher
