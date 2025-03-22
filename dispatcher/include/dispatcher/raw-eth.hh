#pragma once

extern "C" {
struct ibv_context;
struct ibv_pd;
struct ibv_qp_cap;
struct ibv_cq_ex;
struct ibv_qp_ex;
struct ibv_qp;
}
namespace raweth {

struct ibv_qp *create_qp_raw_rtr(struct ibv_context *ctx, int port_num,
                                 struct ibv_pd *pd, struct ibv_cq_ex *cq,
                                 struct ibv_qp_cap &cap, int sig_all);
int modify_qp_raw_rts(struct ibv_qp *qp);

}  // namespace raweth
