#include <errno.h>
#include <stdlib.h>
//
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>

extern "C" {

int sched_setscheduler(pid_t pid, int sched, const struct sched_param *param) {
    return -ENOSYS;
}
int sync_file_range(int fd, off_t pos, off_t len, unsigned flags) {
    return -ENOSYS;
}
int posix_madvise(void *addr, size_t len, int advice) { return -ENOSYS; }
ssize_t readahead(int fd, off_t pos, size_t len) { return -ENOSYS; }

int malloc_trim(size_t f) {
    // abort();
    // ignore malloc_trim
    return 0;
}

void *mremap(void *old_address, size_t old_size, size_t new_size, int flags,
             ... /* void *new_address */) {
    abort();
    return NULL;
}
}
