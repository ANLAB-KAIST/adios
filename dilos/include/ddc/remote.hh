#pragma once
#include <stdint.h>
#include <stdlib.h>

#include <atomic>
#include <boost/circular_buffer.hpp>
#include <initializer_list>
#include <vector>
// Note: Remote moudle would include and implement this (not remote server)

namespace ddc {

void remote_init();
class remote_queue {
   public:
    remote_queue(std::initializer_list<int> fetch_sizes,
                 std::initializer_list<int> push_sizes,
                 std::initializer_list<int> raw_rx_sizes = {},
                 std::initializer_list<int> raw_tx_sizes = {})
        : cq(NULL), fetch_total(0), push_total(0) {
        for (auto &_size : fetch_sizes) {
            fetch_qp.push_back({NULL, 0, _size});
        }
        for (auto &_size : push_sizes) {
            push_qp.push_back({NULL, 0, _size});
        }
        for (auto &_size : raw_rx_sizes) {
            raw_rx_qp.push_back({NULL, 0, _size});
        }
        for (auto &_size : raw_tx_sizes) {
            raw_tx_qp.push_back({NULL, 0, _size});
        }
    }
    void setup();
    ~remote_queue();
    bool fetch(int qp_id, uintptr_t token, void *paddr, uintptr_t offset,
               size_t size);
    bool push(int qp_id, uintptr_t token, void *paddr, uintptr_t offset,
              size_t size);

    // support only single frame
    bool fetch_vec(int qp_id, uintptr_t token, void *paddr, uintptr_t offset,
                   uintptr_t bitmask);
    bool push_vec(int qp_id, uintptr_t token, void *paddr, uintptr_t offset,
                  uintptr_t bitmask);

    int poll(uintptr_t tokens[], int len);

    void get_stat(size_t &fetch_total, size_t &push_total) {
        fetch_total = this->fetch_total;
        push_total = this->push_total;
    }

   private:
    void *cq;
    struct entry_t {
        uintptr_t token;
        uint64_t cycle;
    };

    struct qp_t {
        void *inner;
        int current;
        int max;
    };

    std::vector<qp_t> fetch_qp;
    std::vector<qp_t> push_qp;
    std::vector<qp_t> raw_rx_qp;
    std::vector<qp_t> raw_tx_qp;
    boost::circular_buffer<entry_t> complete;
    std::atomic_size_t fetch_total;
    std::atomic_size_t push_total;
};  // namespace ddc

size_t remote_reserve_size();
};  // namespace ddc