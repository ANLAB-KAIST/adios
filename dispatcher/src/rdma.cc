#include <infiniband/verbs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <dispatcher/rdma.hh>
#include <mutex>

namespace dispatcher {

static int send_request(int sock, rdma::request_t &req,
                        rdma::response_t &resp) {
    int wc = 0;
    unsigned long total_write_bytes = 0;
    ssize_t write_bytes = 0;
    while (!wc && total_write_bytes < sizeof(req)) {
        write_bytes = write(sock, &req, sizeof(req) - total_write_bytes);
        if (write_bytes > 0)
            total_write_bytes += write_bytes;
        else if (write_bytes == 0)
            wc = 1;
        else
            wc = write_bytes;
    }

    if (wc > 0) return 1;

    int rc = 0;
    unsigned long total_read_bytes = 0;
    ssize_t read_bytes = 0;
    memset(&resp, 0, sizeof(resp));
    while (!rc && total_read_bytes < sizeof(resp)) {
        read_bytes = read(sock, &resp, sizeof(resp) - total_read_bytes);
        if (read_bytes > 0)
            total_read_bytes += read_bytes;
        else if (read_bytes == 0)
            rc = 1;
        else
            rc = read_bytes;
    }

    if (rc > 0) return 1;

    if (resp.ok != 1) {
        return 1;
    }

    return 0;
}
int rdma_t::connect_mem_server(std::string hostname, std::string port) {
    const std::lock_guard<std::mutex> lock(mtx);
    struct addrinfo *resolved_addr = NULL;
    struct addrinfo *iterator;

    struct addrinfo hints = {.ai_flags = AI_PASSIVE,
                             .ai_family = AF_INET,
                             .ai_socktype = SOCK_STREAM};

    int ret =
        getaddrinfo(hostname.c_str(), port.c_str(), &hints, &resolved_addr);
    if (ret < 0) {
        printf("%s for %s:%s\n", gai_strerror(ret), hostname.c_str(),
               port.c_str());
        return ret;
    }

    sock = -1;

    for (iterator = resolved_addr; iterator; iterator = iterator->ai_next) {
        sock = socket(iterator->ai_family, iterator->ai_socktype,
                      iterator->ai_protocol);
        if (sock >= 0) {
            if ((ret = connect(sock, iterator->ai_addr, iterator->ai_addrlen) ==
                       -1)) {
                fprintf(stdout, "failed connect :%s\n", strerror(errno));
                close(sock);
                sock = -1;
                continue;
            }
            break;
        }
    }
    if (resolved_addr) freeaddrinfo(resolved_addr);

    if (sock < 1) {
        printf("connect failed\n");
        return 1;
    }
    // conected, fetch info

    rdma::request_t req;
    rdma::response_t resp;
    memset(&req, 0, sizeof(req));

    req.type = rdma::request_type::INFO;

    ret = send_request(sock, req, resp);
    if (ret) {
        printf("fail to send request");
        return 1;
    }

    server_lid = resp.info.lid;
    memcpy(server_gid, resp.info.gid, sizeof(server_gid));
    server_addr = resp.info.start_addr;
    server_len = resp.info.len;
    server_rkey = resp.info.rkey;

    return 0;
}

uint32_t rdma_t::new_qp() {
    const std::lock_guard<std::mutex> lock(mtx);

    rdma::request_t req;
    rdma::response_t resp;
    memset(&req, 0, sizeof(req));

    req.type = rdma::request_type::NEW_QP;

    int ret = send_request(sock, req, resp);
    if (ret) {
        printf("fail to send request");
        return 0;
    }

    return resp.new_qp.qp_num;
}
int rdma_t::connect_qp(uint16_t lid, uint8_t gid[16], uint32_t qp_num_client,
                       uint32_t qp_num_server) {
    const std::lock_guard<std::mutex> lock(mtx);

    rdma::request_t req;
    rdma::response_t resp;
    memset(&req, 0, sizeof(req));

    req.type = rdma::request_type::CONNECT_QP;

    req.connect_qp.lid = lid;
    memcpy(req.connect_qp.gid, gid, sizeof(req.connect_qp.gid));
    req.connect_qp.qp_num_client = qp_num_client;
    req.connect_qp.qp_num_server = qp_num_server;

    int ret = send_request(sock, req, resp);
    if (ret) {
        printf("fail to send request");
        return 1;
    }

    return 0;
}

struct ibv_qp *rdma_t::create_qp_init(struct ibv_context *ctx, int port_num,
                                      struct ibv_pd *pd, struct ibv_cq_ex *cq,
                                      struct ibv_qp_cap &cap, int sig_all) {
    int ret = 0;
    struct ibv_qp_init_attr_ex qp_init_attr_ex;
    memset(&qp_init_attr_ex, 0, sizeof(qp_init_attr_ex));
    qp_init_attr_ex.send_ops_flags |=
        IBV_QP_EX_WITH_RDMA_READ | IBV_QP_EX_WITH_RDMA_WRITE;
    qp_init_attr_ex.pd = pd;
    qp_init_attr_ex.comp_mask |=
        IBV_QP_INIT_ATTR_SEND_OPS_FLAGS | IBV_QP_INIT_ATTR_PD;
    qp_init_attr_ex.send_cq = ibv_cq_ex_to_cq(cq);
    qp_init_attr_ex.recv_cq = ibv_cq_ex_to_cq(cq);

    memcpy(&qp_init_attr_ex.cap, &cap, sizeof(qp_init_attr_ex.cap));

    qp_init_attr_ex.qp_type = IBV_QPT_RC;
    qp_init_attr_ex.srq = NULL;
    qp_init_attr_ex.sq_sig_all = sig_all;

    struct ibv_qp *rdma_qp = ibv_create_qp_ex(ctx, &qp_init_attr_ex);
    if (!rdma_qp) {
        perror("Couldn't create rdma QP");
        return NULL;
    }

    struct ibv_qp_attr attr;

    /* Modify QP to INIT */
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = port_num;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                           IBV_ACCESS_REMOTE_WRITE;
    ret = ibv_modify_qp(
        rdma_qp, &attr,
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
    if (ret) {
        printf("Fail to modify RDMA QP\n");
        return NULL;
    }

    /* Modify QP to INIT (END) */

    return rdma_qp;
}

int rdma_t::modify_qp_rtr(struct ibv_qp *rdma_qp, uint32_t dest_qp_num,
                          int port_num, int sgid_idx) {
    struct ibv_qp_attr attr;
    /* Modify QP to RTR */
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_1024;
    attr.dest_qp_num = dest_qp_num;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 16;
    attr.min_rnr_timer = 12;
    attr.ah_attr.dlid = server_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = port_num;
    // Always have GID
    attr.ah_attr.is_global = 1;
    memcpy(&attr.ah_attr.grh.dgid, server_gid, 16);
    attr.ah_attr.grh.flow_label = 0;
    attr.ah_attr.grh.hop_limit = 1;
    attr.ah_attr.grh.sgid_index = sgid_idx;
    attr.ah_attr.grh.traffic_class = 0;

    return ibv_modify_qp(rdma_qp, &attr,
                         IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
                             IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                             IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);

    /* Modify QP to RTR (END) */
}

int rdma_t::modify_qp_rts(struct ibv_qp *rdma_qp) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 16;
    return ibv_modify_qp(rdma_qp, &attr,
                         IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                             IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN |
                             IBV_QP_MAX_QP_RD_ATOMIC);
}
}  // namespace dispatcher
