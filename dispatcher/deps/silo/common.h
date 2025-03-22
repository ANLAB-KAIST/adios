#pragma once

#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>

void init_ix(int udp);
void init_linux(int n_cpu, int port);
void init_thread(void);
bool process_request(void);
bool process_request_new(uint64_t *worker_idx);
void start_ix_server(int udp);
void start_linux_server(void);
int init_shenango(const char *cfgpath, int port);
int init_global(void);

extern __thread int thread_no;
extern int nr_cpu;

static inline long mytime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
