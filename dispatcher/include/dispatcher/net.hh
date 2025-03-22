#pragma once

#include <dispatcher/common.hh>
#include <protocol/net.hh>

namespace dispatcher {

struct net_conifg_t {
    eth_addr_t mac_addr;
    ip_addr_t ip_addr;
};
}  // namespace dispatcher