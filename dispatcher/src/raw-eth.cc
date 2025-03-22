#include <infiniband/verbs.h>

#include <cstdio>
#include <dispatcher/raw-eth.hh>

namespace raweth {

struct ibv_qp *create_qp_raw_rtr(struct ibv_context *ctx, int port_num,
                                 struct ibv_pd *pd, struct ibv_cq_ex *cq,
                                 struct ibv_qp_cap &cap, int sig_all) {
    int ret = 0;

    struct ibv_qp_init_attr_ex qp_init_attr_ex;
    memset(&qp_init_attr_ex, 0, sizeof(qp_init_attr_ex));

    qp_init_attr_ex.pd = pd;
    qp_init_attr_ex.comp_mask |= IBV_QP_INIT_ATTR_PD;
    qp_init_attr_ex.comp_mask |= IBV_QP_INIT_ATTR_SEND_OPS_FLAGS;

    qp_init_attr_ex.send_cq = ibv_cq_ex_to_cq(cq);
    qp_init_attr_ex.recv_cq = ibv_cq_ex_to_cq(cq);

    memcpy(&qp_init_attr_ex.cap, &cap, sizeof(qp_init_attr_ex.cap));

    qp_init_attr_ex.qp_type = IBV_QPT_RAW_PACKET;
    qp_init_attr_ex.srq = NULL;
    qp_init_attr_ex.sq_sig_all = sig_all;

    struct ibv_qp *eth_qp = ibv_create_qp_ex(ctx, &qp_init_attr_ex);
    if (!eth_qp) {
        printf("Couldn't create eth QP\n");
        return NULL;
    }

    struct ibv_qp_attr qp_attr;

    memset(&qp_attr, 0, sizeof(qp_attr));

    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.port_num = port_num;

    ret = ibv_modify_qp(eth_qp, &qp_attr, IBV_QP_STATE | IBV_QP_PORT);
    if (ret < 0) {
        printf("failed modify qp to init\n");
        return NULL;
    }
    memset(&qp_attr, 0, sizeof(qp_attr));
    qp_attr.qp_state = IBV_QPS_RTR;
    ret = ibv_modify_qp(eth_qp, &qp_attr, IBV_QP_STATE);

    if (ret < 0) {
        printf("failed modify qp to receive\n");
        return NULL;
    }
    return eth_qp;
}
int modify_qp_raw_rts(struct ibv_qp *qp) {
    int ret = 0;
    struct ibv_qp_attr qp_attr;
    memset(&qp_attr, 0, sizeof(qp_attr));

    qp_attr.qp_state = IBV_QPS_RTS;

    ret = ibv_modify_qp(qp, &qp_attr, IBV_QP_STATE);
    if (ret < 0) {
        printf("fail to modify qp to rts\n");
        return 1;
    }
    return 0;
}
}  // namespace raweth