#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>

#ifndef MAP_DDC
#define MAP_DDC 0x400000
#endif
extern "C" {

static std::atomic_uint64_t total_alloc(0);

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset) {
    static void *(*func_mmap)(void *, size_t, int, int, int, off_t) = NULL;

    if (!func_mmap)
        func_mmap = (void *(*)(void *, size_t, int, int, int, off_t))dlsym(
            RTLD_NEXT, "mmap");

    void *ret_addr = MAP_FAILED;
    // printf(
    //     "HOOK: mmap file "
    //     "(addr=%p,length=%lx,prot=%x,flags=%x,fd=%d,offset=%lx) -> %p\n",
    //     addr, length, prot, flags, fd, offset, ret_addr);

    if (flags & MAP_DDC) {
        size_t current = total_alloc.fetch_add(length);

        if (((current - length) >> 30) != (current >> 30)) {
            printf("DDC alloced: %p %ld GB\n", addr, current >> 30);
        }
    }

    if (fd > 0 && (flags & MAP_DDC)) {
        flags = flags & ~MAP_DDC;
        // size_t page_length = page_align_up(length);
        ret_addr = func_mmap(addr, length, prot | PROT_WRITE,
                             flags | MAP_ANONYMOUS, -1, 0);

        if (ret_addr == MAP_FAILED) {
            return MAP_FAILED;
        }

        ssize_t rd;

        size_t total_read = 0;
        off_t offset_old = lseek(fd, 0, SEEK_CUR);
        off_t offset_new = lseek(fd, 0, SEEK_SET);
        if (offset_new != 0) abort();
        char *addr_start = (char *)ret_addr;
        while (total_read < length) {
            rd = read(fd, addr_start + total_read, length - total_read);
            if (rd <= 0) {
                // An error occurred
                abort();
                return MAP_FAILED;
            }
            total_read += rd;
        }

        offset_new = lseek(fd, offset_old, SEEK_SET);

        if (offset_old != offset_new) abort();

        int ret = mprotect(ret_addr, length, prot);

        if (ret) {
            printf("mprotect failed: %s\n", strerror(errno));
            abort();
        }

    } else {
        flags = flags & ~MAP_DDC;
        ret_addr = func_mmap(addr, length, prot, flags, fd, offset);
    }
    return ret_addr;
}
// int munmap(void *addr, size_t length) {
//     static int (*func_munmap)(void *, size_t) = NULL;

//     if (!func_munmap)
//         func_munmap = (int (*)(void *, size_t))dlsym(RTLD_NEXT, "munmap");
//     int ret = func_munmap(addr, length);
//     printf("HOOK: munmap file (addr=%p, length=%lx)\n", addr, length);
//     return ret;
// }
}