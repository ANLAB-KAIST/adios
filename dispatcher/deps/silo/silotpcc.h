#pragma once

#ifdef __cplusplus

extern "C"
{

#endif

int silotpcc_init(int number_threads, long numa_memory);
void silotpcc_init_thread(int worker_id);
bool silotpcc_exec_one(int worker_id);

bool worker_exec_one(void *w, size_t *widx);
void *new_worker(unsigned int thread_hint);

#ifdef __cplusplus
}
#endif
