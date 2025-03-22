
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

extern "C" int dp_addon_memcached_init(char *mem_base, size_t mem_size,
                                       size_t key_range, size_t key_size,
                                       size_t value_size);
int main() {
    constexpr size_t mem_size = 40ULL << 30;
    void *app_mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (app_mem == MAP_FAILED) {
        printf("map failed\n");
        return 1;
    }

    int ret =
        dp_addon_memcached_init((char *)app_mem, mem_size, 36200000, 50, 1024);
    if (ret) {
        printf("init failed\n");
        return 1;
    }
    printf("ok\n");
    sleep(3600);
    return 0;
}