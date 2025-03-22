#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <linux/mman.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <config/arg-config.hh>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dispatcher/rdma.hh>
#include <map>
#include <string>

struct arg_config_t {
    ARG_STR(dev_name, "IB device name. Example: mlx5_0");
    ARG_INT_DEFAULT(tcp_port_number, 12345,
                    "TCP port number for control path. Default: 12345");
    ARG_INT(memory_size_gb, "memory size to serve");
    ARG_INT_DEFAULT(epoll_size, 64, "max epoll size. Default: 64");
    ARG_INT_DEFAULT(ib_port, 1, "IB port number. Default:1");
    ARG_INT_DEFAULT(ib_gid_index, 1, "IB GID index. Default:1");

    int parse(int argc, char **argv) {
        ARG_PARSE_START(argc, argv);
        ARG_PARSE(dev_name);
        ARG_PARSE(tcp_port_number);
        ARG_PARSE(memory_size_gb);
        ARG_PARSE(epoll_size);
        ARG_PARSE(ib_port);
        ARG_PARSE(ib_gid_index);
        ARG_PARSE_END();

        if (dev_name.empty()) {
            printf("No dev_name.\n");
            return 1;
        }
        if (tcp_port_number == 0 || tcp_port_number >= 65536) {
            printf("No tcp_port_number.\n");
            return 1;
        }
        if (memory_size_gb == 0) {
            printf("No memory_size_gb.\n");
            return 1;
        }
        if (epoll_size == 0) {
            printf("No epoll_size.\n");
            return 1;
        }
        if (ib_port == 0) {
            printf("No ib_port.\n");
            return 1;
        }
        if (ib_gid_index == 0) {
            printf("No ib_gid_index.\n");
            return 1;
        }

        return 0;
    }

    static std::string usage(int argc, char **argv) {
        ARG_USAGE_START(argc, argv);
        ARG_USAGE(dev_name);
        ARG_USAGE(tcp_port_number);
        ARG_USAGE(memory_size_gb);
        ARG_USAGE(epoll_size);
        ARG_USAGE(ib_port);
        ARG_USAGE(ib_gid_index);

        return ARG_USAGE_END();
    }
    std::string str() {
        ARG_PRINT_START();
        ARG_PRINT(dev_name);
        ARG_PRINT(tcp_port_number);
        ARG_PRINT(memory_size_gb);
        ARG_PRINT(epoll_size);
        ARG_PRINT(ib_port);
        ARG_PRINT(ib_gid_index);
        return ARG_PRINT_END();
    }
};

int main(int argc, char *argv[]) {
    printf("Starting mem_server...\n");
    auto usage = arg_config_t::usage(argc, argv);

    arg_config_t config;
    int parse_ret = config.parse(argc, argv);

    printf("[Configuration]\n\n%s\n", config.str().c_str());

    if (parse_ret) {
        printf("\n\nUsage: %s\n", usage.c_str());
        return 1;
    }

    size_t memory_region_len = config.memory_size_gb << 30;

    int num_devices;

    struct ibv_device *ib_dev = NULL;
    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);

    if (!dev_list) {
        printf("no rdma devices\n");
        return 1;
    }

    if (!num_devices) {
        printf("zero rdma devices\n");
        return 1;
    }

    for (int i = 0; i < num_devices; i++) {
        if (config.dev_name == ibv_get_device_name(dev_list[i])) {
            ib_dev = dev_list[i];
            break;
        }
    }
    if (!ib_dev) {
        printf("no device: %s\n", config.dev_name.c_str());
        return 1;
    }
    struct ibv_context *ib_ctx = NULL;

    ib_ctx = ibv_open_device(ib_dev);
    if (!ib_ctx) {
        printf("fail to open device\n");
        return 1;
    }

    struct ibv_device_attr device_attr;
    memset(&device_attr, 0, sizeof(device_attr));

    int ret = ibv_query_device(ib_ctx, &device_attr);
    if (ret != 0) {
        printf("fail to query device\n");
        return 1;
    }
    ibv_free_device_list(dev_list);
    dev_list = NULL;
    ib_dev = NULL;

    struct ibv_port_attr port_attr;
    memset(&port_attr, 0, sizeof(port_attr));
    if (ibv_query_port(ib_ctx, config.ib_port, &port_attr)) {
        printf("fail to query port\n");
        return 1;
    }

    union ibv_gid gid;
    if (ibv_query_gid(ib_ctx, config.ib_port, config.ib_gid_index, &gid)) {
        printf("fail to query gid\n");
        return 1;
    }

    struct ibv_cq *cq = ibv_create_cq(ib_ctx, 1, NULL, NULL, 0);

    if (cq == NULL) {
        printf("create cq faild\n");
        return -1;
    }

    struct ibv_pd *pd = ibv_alloc_pd(ib_ctx);
    if (pd == NULL) {
        printf("fail to alloc pd\n");
        return -1;
    }

    void *memory_region =
        mmap(NULL, memory_region_len, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB, -1, 0);

    if (memory_region == MAP_FAILED) {
        printf("mmap faild\n");
        return -1;
    }

    struct ibv_mr *mr =
        ibv_reg_mr(pd, memory_region, memory_region_len,
                   IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                       IBV_ACCESS_REMOTE_WRITE);

    if (mr == NULL) {
        printf("reg mr faild\n");
        return -1;
    }

    printf("init done\n");

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int sockopt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &sockopt,
                   sizeof(sockopt)) == -1) {
        printf("Fail to set sock opt\n");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(config.tcp_port_number);
    struct sockaddr *sock_addr = (struct sockaddr *)&server_addr;
    if (bind(listenfd, sock_addr, sizeof(server_addr)) < 0) {
        printf("Bind fail: %s\n", strerror(errno));
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, 5) < 0) {
        printf("Listen fail\n");
        close(listenfd);
        return 1;
    }

    int epoll_fd = epoll_create(config.epoll_size);
    struct epoll_event accept_event;
    accept_event.events = EPOLLIN;
    accept_event.data.fd = listenfd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &accept_event);
    if (ret < 0) {
        printf("epoll_ctl fail\n");
        return ret;
    }
    struct epoll_event *events = (struct epoll_event *)calloc(
        config.epoll_size, sizeof(struct epoll_event));

    std::map<uint32_t, struct ibv_qp *> qps;
    while (1) {
        int nready = epoll_wait(epoll_fd, events, config.epoll_size, -1);
        for (int i = 0; i < nready; i++) {
            if (events[i].events & EPOLLERR) {
                printf("epoll_wait returned EPOLLERR");
                return 1;
            }

            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == listenfd) {
                    // new client
                    int sockfd = -1;
                    sockfd = accept(listenfd, NULL, 0);
                    printf("New client: %d\n", sockfd);
                    if (sockfd < 0) {
                        close(listenfd);
                        printf("Server: accept failed.\n");
                        return 1;
                    }

                    if (sockfd >= config.epoll_size) {
                        printf("sockfd >= epoll_size\n");
                        return 1;
                    }

                    struct epoll_event event;
                    memset(&event, 0, sizeof(event));

                    event.data.fd = sockfd;
                    event.events |= EPOLLIN;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) <
                        0) {
                        printf("new client fail\n");
                    }
                } else {
                    int sockfd = events[i].data.fd;

                    rdma::request_t req;
                    memset(&req, 0, sizeof(req));

                    int rc = 0;
                    size_t total_read_bytes = 0, read_bytes = 0;
                    while (!rc && total_read_bytes < sizeof(req)) {
                        read_bytes =
                            read(sockfd, &req, sizeof(req) - total_read_bytes);
                        if (read_bytes > 0)
                            total_read_bytes += read_bytes;
                        else if (read_bytes == 0)
                            break;
                        else
                            rc = read_bytes;
                        break;
                    }

                    if (total_read_bytes == 0) {
                        continue;
                    }

                    rdma::response_t resp;
                    memset(&resp, 0, sizeof(resp));

                    switch (req.type) {
                        case rdma::request_type::INFO: {
                            printf("Handling INFO for %d\n", sockfd);

                            resp.ok = 1;
                            resp.type = rdma::request_type::INFO;

                            resp.info.lid = port_attr.lid;
                            memcpy(resp.info.gid, &gid, sizeof(gid));
                            resp.info.rkey = mr->rkey;
                            resp.info.start_addr =
                                reinterpret_cast<uintptr_t>(mr->addr);
                            resp.info.len = mr->length;

                            printf("Handling INFO for %d finished\n", sockfd);
                            break;
                        }
                        case rdma::request_type::NEW_QP: {
                            printf("Handling NEW_QP for %d\n", sockfd);
                            struct ibv_qp_init_attr qp_init_attr;
                            memset(&qp_init_attr, 0, sizeof(qp_init_attr));

                            // TODO: review parameters
                            qp_init_attr.qp_type = IBV_QPT_RC;
                            qp_init_attr.sq_sig_all = 0;
                            qp_init_attr.send_cq = cq;
                            qp_init_attr.recv_cq = cq;
                            qp_init_attr.cap.max_send_wr = 1;
                            qp_init_attr.cap.max_recv_wr = 1;
                            qp_init_attr.cap.max_send_sge = 30;
                            qp_init_attr.cap.max_recv_sge = 30;
                            struct ibv_qp *qp =
                                ibv_create_qp(pd, &qp_init_attr);
                            if (!qp) {
                                resp.ok = 2;
                                return false;
                            }

                            auto qp_ret =
                                qps.insert(std::make_pair(qp->qp_num, qp));

                            if (!qp_ret.second) {
                                printf("QP Insert failed\n");
                                return 1;
                            }

                            resp.ok = 1;
                            resp.type = rdma::request_type::NEW_QP;
                            resp.new_qp.qp_num = qp->qp_num;
                            printf("Handling NEW_QP for %d finished\n", sockfd);
                            break;
                        }
                        case rdma::request_type::CONNECT_QP: {
                            printf("Handling CONNECT_QP for %d\n", sockfd);
                            resp.type = rdma::request_type::CONNECT_QP;
                            auto it = qps.find(req.connect_qp.qp_num_server);
                            if (it == qps.end()) {
                                resp.ok = 2;
                                break;
                            }
                            struct ibv_qp_attr attr;
                            memset(&attr, 0, sizeof(attr));
                            attr.qp_state = IBV_QPS_INIT;
                            attr.port_num = config.ib_port;
                            attr.pkey_index = 0;
                            attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE |
                                                   IBV_ACCESS_REMOTE_READ |
                                                   IBV_ACCESS_REMOTE_WRITE;

                            if (ibv_modify_qp(it->second, &attr,
                                              IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                                                  IBV_QP_PORT |
                                                  IBV_QP_ACCESS_FLAGS)) {
                                resp.ok = 2;
                                break;
                            }

                            memset(&attr, 0, sizeof(attr));
                            attr.qp_state = IBV_QPS_RTR;
                            attr.path_mtu = IBV_MTU_1024;
                            attr.dest_qp_num = req.connect_qp.qp_num_client;
                            attr.rq_psn = 0;
                            attr.max_dest_rd_atomic = 16;
                            attr.min_rnr_timer = 12;
                            attr.ah_attr.dlid = req.connect_qp.lid;
                            attr.ah_attr.sl = 0;
                            attr.ah_attr.src_path_bits = 0;
                            attr.ah_attr.port_num = config.ib_port;

                            // Always have GID
                            attr.ah_attr.is_global = 1;
                            memcpy(&attr.ah_attr.grh.dgid, req.connect_qp.gid,
                                   16);
                            attr.ah_attr.grh.flow_label = 0;
                            attr.ah_attr.grh.hop_limit = 1;
                            attr.ah_attr.grh.sgid_index = config.ib_gid_index;
                            attr.ah_attr.grh.traffic_class = 0;

                            if (ibv_modify_qp(
                                    it->second, &attr,
                                    IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
                                        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                                        IBV_QP_MAX_DEST_RD_ATOMIC |
                                        IBV_QP_MIN_RNR_TIMER)) {
                                printf("failed to modify QP, RTR\n");
                                resp.ok = 2;
                                break;
                            }

                            memset(&attr, 0, sizeof(attr));
                            attr.qp_state = IBV_QPS_RTS;
                            attr.timeout = 14;
                            attr.retry_cnt = 7;
                            attr.rnr_retry = 7;
                            attr.sq_psn = 0;
                            attr.max_rd_atomic = 16;

                            if (ibv_modify_qp(it->second, &attr,
                                              IBV_QP_STATE | IBV_QP_TIMEOUT |
                                                  IBV_QP_RETRY_CNT |
                                                  IBV_QP_RNR_RETRY |
                                                  IBV_QP_SQ_PSN |
                                                  IBV_QP_MAX_QP_RD_ATOMIC)) {
                                printf("failed to modify QP, RTS\n");
                                resp.ok = 2;
                                break;
                            }

                            resp.ok = 1;
                            printf("Handling CONNECT_QP for %d finished\n",
                                   sockfd);
                            break;
                        }
                        default:
                            printf("Unknown req %d for %d\n", (int)req.type,
                                   sockfd);
                            break;
                    }

                    int wc = 0;
                    size_t total_write_bytes = 0, write_bytes = 0;
                    while (!wc && total_write_bytes < sizeof(resp)) {
                        write_bytes = write(sockfd, &resp,
                                            sizeof(resp) - total_write_bytes);
                        if (write_bytes > 0)
                            total_write_bytes += write_bytes;
                        else if (write_bytes == 0)
                            wc = 1;
                        else
                            wc = write_bytes;
                    }
                }
            }
        }
    }
    return 0;
}