#pragma once

#include <dispatcher/ucontext-fast.h>

#include <config/static-config.hh>
#include <dispatcher/app.hh>
#include <dispatcher/common.hh>
#include <dispatcher/context.hh>
#include <protocol/cmd.hh>

namespace dispatcher {

class worker_t;
worker_t &get_current_worker();

class job_t : public job_adapter_t {
   public:
    static constexpr uint8_t STATE_NONE = 255;
    static constexpr uint8_t STATE_NEW = 254;
    static constexpr uint8_t STATE_ANY_CORE = 253;

    static inline job_t *from_pkt(cmdpkt_t *pkt) {
        job_t *job = (job_t *)pkt;
        eth_addr_t _mac = pkt->eth.src;
        job->src_mac = _mac;

        job->state = job_t::STATE_NEW;  // todo option

        // use this for initial or resume // todo: use different
        // location? for cache-locality?

        return job;
    }
    inline cmdpkt_t *to_pkt() { return (cmdpkt_t *)this; }

#ifdef PROTOCOL_MEASURE
    inline uint64_t wr_id() { return to_pkt()->measure.lgen_wr_id; }
#endif

    inline void *to_page() { return (void *)this; }
    template <typename T>
    inline T *payload() {
        return (T *)(((uintptr_t)this) + sizeof(cmdpkt_t));
    }

    int run();
    int sched();
    static void do_run(job_t *jb);
    static void do_run_sjk(job_t *jb);

    job_t *next;  // used by dispatcher, only used when job is queued

    eth_addr_t src_mac;  // saved src_mac for xmit

    // below can be used after IP
    uint8_t state;

    inline context_t *ctx() {
        return (context_t *)(((uintptr_t)this) + NET_MTU_ASSUME);
    }
    inline context_inner_t *uctx() { return ctx()->uctx(); }

   private:
} __attribute__((packed));
static_assert(sizeof(ethhdr_t) >= offsetof(job_t, state));
static_assert(sizeof(ethhdr_t) + offsetof(iphdr_t, version_ihl) ==
              offsetof(job_t, state));
static_assert(sizeof(ethhdr_t) + sizeof(iphdr_t) >= sizeof(job_t));

// reuse ethhdr place to job header

struct job_list_t {
    struct job_t *first;
    struct job_t *last;
    job_list_t() : first(nullptr), last(nullptr) {}

    inline void push_back(job_t *job) {
        job->next = nullptr;
        if (gcc_unlikely(last == nullptr)) {
            first = job;
        } else {
            last->next = job;
        }
        last = job;
    }

    inline void push_back_all(job_list_t *jobs) {
        jobs->last->next = nullptr;
        if (gcc_unlikely(last == nullptr)) {
            first = jobs->first;
        } else {
            last->next = jobs->first;
        }
        last = jobs->last;
    }

    inline job_t *pop() {
        if (gcc_unlikely(first == nullptr)) {
            // no job
            return nullptr;
        }
        if (gcc_unlikely(first->next == nullptr)) {
            // last job
            last = nullptr;
        }
        job_t *job = first;

        first = job->next;
        job->next = nullptr;

        return job;
    }
    inline void reset() {
        first = nullptr;
        last = nullptr;
    }
    inline bool empty() { return first == nullptr; }
};

}  // namespace dispatcher