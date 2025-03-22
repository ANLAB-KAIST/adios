#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char **argv) {
    int (*dispatcher_main)(int argc, char **argv) =
        dlsym(RTLD_DEFAULT, "dispatcher_main");

    if (dispatcher_main == NULL) {
        printf("Unable to find dispatcher_main\n");
        return -1;
    }
    return dispatcher_main(argc, argv);
}
