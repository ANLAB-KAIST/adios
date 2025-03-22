#include <sys/mman.h>

#include <atomic>
#include <config/static-config.hh>
#include <cstdio>
#include <cstdlib>
#include <dispatcher/api.hh>
#include <dispatcher/common.hh>
#include <dispatcher/pagepool.hh>
#include <protocol/net.hh>

extern "C" void debug_early(const char *msg) { printf("%s", msg); }
extern "C" void debug_early_u64(const char *msg, unsigned long long val) {
    printf("%s %llu\n", msg, val);
}

namespace dispatcher {

static unsigned char *linux_mr;
constexpr size_t LINUX_MR_SIZE = NUM_ETH_PKT_PAGES * ETH_BUFFER_SIZE;
int init_driver() {
    linux_mr = (unsigned char *)mmap(
        NULL, LINUX_MR_SIZE, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    printf("[init] linux_mr: %p\n", linux_mr);
    if (linux_mr == MAP_FAILED) {
        printf("Coudln't allocate memory\n");
        return 1;
    }
    return 0;
}

std::vector<std::pair<void *, size_t>> list_phys() {
    std::vector<std::pair<void *, size_t>> result;

    result.emplace_back(linux_mr, LINUX_MR_SIZE);
    return result;
}

int init_os() { return 0; }

int change_pf_mode(int pf_mode) { return 0; }
unsigned get_apic_id() { return 0; }
void send_ipi(unsigned apic_id) {}

bool handle_wait(dispatcher::job_t *, void *ptep) { return true; }

void handle_preempt_requested(unsigned &my_cpu_id) {}

unsigned get_cpu_id() { return 0; }

void disable_preemption() {}
void enable_preemption() {}
void trace_enqueue(uintptr_t jid) {}
void trace_dequeue(uintptr_t jid, uint8_t wid) {}
void trace_yield(uintptr_t jid) {}
void trace_asyncpf(uintptr_t jid, void *ctx) {}
void trace_exit(uintptr_t jid) {}
void trace_asyncpf_fj(uintptr_t jid) {}
void trace_exit_fj(uintptr_t jid) {}
void trace_yield_fj(uintptr_t jid) {}
void trace_reenqueue(uintptr_t jid) {}
void trace_newjob(uintptr_t jid) {}
void trace_resume(uintptr_t jid) {}
void trace_start(uintptr_t jid) {}
void trace_general(int id, uintptr_t token) {}

void trace_current_worker(void *p) {}
void trace_worker_st(uint8_t a, uint8_t b) {}

void trace_fetch(int qp_id, void *token, void *paddr, void *roffset,
                 size_t size, int ret) {}
void trace_push(int qp_id, void *token, void *paddr, void *roffset, size_t size,
                int ret) {}
void trace_polled(void *token) {}
void trace_alloc(uintptr_t tag, uintptr_t addr) {}
void trace_free(uintptr_t tag, uintptr_t addr) {}

}  // namespace dispatcher