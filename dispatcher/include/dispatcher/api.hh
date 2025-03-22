#pragma once

#include <cstdlib>
#include <vector>
namespace dispatcher {
class job_t;

int init_driver();
std::vector<std::pair<void *, size_t>> list_phys();
int change_pf_mode(int pf_mode);  // enable async pf for this core
unsigned get_apic_id();
void send_ipi(unsigned apic_id);

bool handle_wait(job_t *job, void *ptep);
unsigned get_cpu_id();
void handle_preempt_requested(unsigned &my_cpu_id);

void disable_preemption();
void enable_preemption();

}  // namespace dispatcher