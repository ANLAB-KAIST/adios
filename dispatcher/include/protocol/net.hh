#pragma once

#include <array>
#include <cstdint>

namespace dispatcher {

using eth_addr_t = std::array<uint8_t, 6>;
using ip_addr_t = std::array<uint8_t, 4>;

constexpr uint64_t NET_PAGE_SIZE = 4096;
constexpr uint64_t NET_PAGE_SHIFT = 12;
constexpr uint64_t NET_MTU_ASSUME = 1536;
constexpr size_t NET_PAGE_BITS = ~(NET_PAGE_SIZE - 1);

constexpr uint16_t ETTYPE_IPV4 = 0x0800;
struct ethhdr_t {
    eth_addr_t dst;
    eth_addr_t src;
    uint16_t ether_type;
} __attribute__((packed));

struct iphdr_t {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    ip_addr_t saddr;
    ip_addr_t daddr;
    /*The options start here. */
} __attribute__((packed));

struct udphdr_t {
    uint16_t uh_sport; /* source port */
    uint16_t uh_dport; /* destination port */
    uint16_t uh_ulen;  /* udp length */
    uint16_t uh_sum;   /* udp checksum */
} __attribute__((packed));

struct udppkt_t {
    struct ethhdr_t eth;
    struct iphdr_t ip;
    struct udphdr_t udp;
} __attribute__((packed));

}  // namespace dispatcher