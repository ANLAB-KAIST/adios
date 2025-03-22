#include <base/atomic.h>
#include <base/stddef.h>
#include <base/list.h>
#include <base/lock.h>
#include <base/cpu.h>
#include <runtime/preempt.h>
#include <runtime/thread.h>
#undef assert


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>


#include "common.h"
#include "silotpcc.h"

struct worker {
	struct list_node link;
	unsigned int id;
	unsigned int home_thread;
	void *runner;
};

struct percore_worker_list {
	struct list_head hd;
	spinlock_t lock;
} __aligned(CACHE_LINE_SIZE);
BUILD_ASSERT(sizeof(struct percore_worker_list) == CACHE_LINE_SIZE);

static struct percore_worker_list all_workers[NCPU];
static __thread struct worker *curr_worker;

void shenango_preempt_disable(void)
{
	preempt_disable();
}

void shenango_preempt_enable(void)
{
	preempt_enable();
}

void shenango_never_called(void)
{
	abort();
	// BUG();
}

bool shenango_is_preempt_disabled(void)
{
	return !preempt_enabled();
}


static struct worker *alloc_worker_for_core(unsigned int target_thread)
{
	static atomic_t worker_count;
	struct worker *out = NULL;
	struct percore_worker_list *l;
	int wid;

	preempt_disable();

	/* have a cached worker in thread local var */
	if (target_thread == thread_no && curr_worker) {
		swapvars(out, curr_worker);
		goto done;
	}

	/* retrieve a worker from the per-thread list */
	l = &all_workers[target_thread];
	if (!list_empty(&l->hd)) {
		spin_lock(&l->lock);
		out = list_pop(&l->hd, struct worker, link);
		spin_unlock(&l->lock);
		if (out)
			goto done;
	}

	if (atomic_read(&worker_count) >= 512)
		goto done;
	wid = atomic_fetch_and_add(&worker_count, 1);
	if (wid >= 512)
		goto done;

	/* allocate a new worker */
	out = (struct worker *)malloc(sizeof(*out));
	if (out == NULL)
		goto done;

	out->id = wid;
	out->home_thread = target_thread;
	out->runner = new_worker(target_thread);
	if (unlikely(out->runner == NULL)) {
		free(out);
		out = NULL;
	}

done:
	preempt_enable();
	return out;
}

static void free_worker(struct worker *w)
{
	struct percore_worker_list *l;

	preempt_disable();
	/* stash locally if possible */
	if (w->home_thread == thread_no && !curr_worker) {
		curr_worker = w;
		preempt_enable();
		return;
	}

	l = &all_workers[w->home_thread];
	spin_lock(&l->lock);
	list_add(&l->hd, &w->link);
	spin_unlock(&l->lock);
	preempt_enable();
}

bool process_request_new(uint64_t *worker_idx)
{
	bool ret;
	struct worker *w;

	w = alloc_worker_for_core(thread_no);
	if (unlikely(!w))
		return false;

	set_uthread_specific(w->id);

	ret = worker_exec_one(w->runner, worker_idx);
	free_worker(w);
	return ret;

}

int nrcpus;
size_t memory;

int init_global(void)
{
	struct worker *workers[512];
	int i, j;

	silotpcc_init(nrcpus, memory);

	for (i = 0; i < NCPU; i++) {
		spin_lock_init(&all_workers[i].lock);
		list_head_init(&all_workers[i].hd);
	}

	for (i = 0; i < 512; i++) {
		workers[i] = alloc_worker_for_core(i % nrcpus);
		if (!workers[i])
			break;
	}

	for (j = 0; j < i; j++)
		free_worker(workers[j]);

	return 0;
}

void init_thread(void)
{
	silotpcc_init_thread(thread_no);
}

void usage(const char *p)
{
	fprintf(stderr, "usage: %s cfgfile nthreads port memory [wload_desc]\n", p);
}

extern char *txn_desc;

int main(int argc, char *argv[])
{
	int port;

	if (argc < 5) {
		usage(argv[0]);
		return -1;
	}

	nrcpus = atoi(argv[2]);
	port = atoi(argv[3]);  
	memory = atoll(argv[4]);

	if (argc > 5)
		txn_desc = argv[5];

	return init_shenango(argv[1], port);
}
