
#include <dispatcher/mm.hh>

namespace dispatcher {

int mm_multi_t::init(const init_t &config, struct ibv_pd *eth_pd,
                     struct ibv_pd *ib_pd) {
    SET_MODULE_NAME("mm");

    int ret = 0;
    for (auto &addr : config.addrs) {
        ret = reg_mr(eth_pd, ib_pd, addr.first, addr.second);

        if (ret) return ret;
    }
    return ret;
}
int mm_multi_t::reg_mr(struct ibv_pd *eth_pd, struct ibv_pd *ib_pd, void *ptr,
                       size_t len) {
    PRINT_LOG("eth_pd: %p ib_pd: %p reg_mr: %p size:0x%lx\n", eth_pd, ib_pd,
              ptr, len);
    auto it = mrs.begin();
    while (it != mrs.end() && it->start != 0) {
        ++it;
    }

    if (it == mrs.end()) {
        return -1;
    }

    ibv_mr *eth_mr = ibv_reg_mr(eth_pd, ptr, len, IBV_ACCESS_LOCAL_WRITE);
    if (eth_mr == nullptr) {
        return -2;
    }
    ibv_mr *ib_mr;
    if (eth_pd == ib_pd) {
        ib_mr = eth_mr;
    } else {
        ib_mr = ibv_reg_mr(ib_pd, ptr, len, IBV_ACCESS_LOCAL_WRITE);
        if (ib_mr == nullptr) {
            return -2;
        }
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    it->start = addr;
    it->end = addr + len;
    it->eth_lkey = eth_mr->lkey;
    it->ib_lkey = ib_mr->lkey;
    it->eth_mr = eth_mr;
    it->ib_mr = ib_mr;
    ++it;
    it->start = 0;
    return 0;
}
int mm_single_t::init(const init_t &config, struct ibv_pd *eth_pd,
                      struct ibv_pd *ib_pd) {
    SET_MODULE_NAME("mm");

    if (config.addrs.size() != 1) {
        return 1;
    }

    int ret =
        reg_mr(eth_pd, ib_pd, config.addrs[0].first, config.addrs[0].second);

    return ret;
}
int mm_single_t::reg_mr(struct ibv_pd *eth_pd, struct ibv_pd *ib_pd, void *ptr,
                        size_t len) {
    PRINT_LOG("eth_pd: %p ib_pd: %p reg_mr: %p size:0x%lx\n", eth_pd, ib_pd,
              ptr, len);

    ibv_mr *eth_mr = ibv_reg_mr(eth_pd, ptr, len, IBV_ACCESS_LOCAL_WRITE);
    if (eth_mr == nullptr) {
        return -2;
    }
    ibv_mr *ib_mr;
    if (eth_pd == ib_pd) {
        ib_mr = eth_mr;
    } else {
        ib_mr = ibv_reg_mr(ib_pd, ptr, len, IBV_ACCESS_LOCAL_WRITE);
        if (ib_mr == nullptr) {
            return -2;
        }
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    mr.start = addr;
    mr.end = addr + len;
    mr.eth_lkey = eth_mr->lkey;
    mr.ib_lkey = ib_mr->lkey;
    mr.eth_mr = eth_mr;
    mr.ib_mr = ib_mr;
    return 0;
}
}  // namespace dispatcher