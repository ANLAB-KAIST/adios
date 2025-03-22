#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
struct ibv_context;
struct ibv_pd;
struct ibv_qp_cap;
struct ibv_cq_ex;
struct ibv_qp_ex;
struct ibv_qp;
}

namespace dispatcher {
class rdma_t {
   public:
    int connect_mem_server(std::string hostname, std::string port);
    uint32_t new_qp();
    int connect_qp(uint16_t lid, uint8_t gid[16], uint32_t qp_num_client,
                   uint32_t qp_num_server);
    struct ibv_qp *create_qp_init(struct ibv_context *ctx, int port_num,
                                  struct ibv_pd *pd, struct ibv_cq_ex *cq,
                                  struct ibv_qp_cap &cap, int sig_all);
    int modify_qp_rtr(struct ibv_qp *rdma_qp, uint32_t dest_qp_num,
                      int port_num, int sgid_idx);
    int modify_qp_rts(struct ibv_qp *qp);

    uint16_t server_lid;
    uint8_t server_gid[16];
    uint32_t server_rkey;
    uint64_t server_addr;
    uint64_t server_len;

   private:
    std::mutex mtx;
    int sock;
};
}  // namespace dispatcher

namespace rdma {

enum class request_type : uint8_t {
    INFO = 1,
    NEW_QP = 2,
    CONNECT_QP = 3,
};

struct request_t {
    request_type type;  // 1: info, 2: new qp, 3: connect qp
    union {
        struct {
            uint16_t lid;
            uint8_t gid[16];
            uint32_t qp_num_client;
            uint32_t qp_num_server;
        } connect_qp;
    };
};

struct response_t {
    uint8_t ok;         // 1: ok, 2: fail
    request_type type;  // 1: info, 2: new qp, 3: connect qp
    union {
        struct {
            uint16_t lid;
            uint8_t gid[16];
            uint32_t rkey;
            uint64_t start_addr;
            uint64_t len;
        } info;
        struct {
            uint32_t qp_num;
        } new_qp;
        struct {
        } connect_qp;
    };
};

}  // namespace rdma
