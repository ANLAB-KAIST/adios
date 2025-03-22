#include <dlfcn.h>
#include <fcntl.h>
// #include <linux/mman.h>
#include <stdio.h>

#include <dispatcher/api.hh>
#include <dispatcher/dispatcher.hh>
// #include <string>
// #include <vector>

extern "C" {
int do_dispatcher_main(int argc, char **argv);
}

int main(int argc, char **argv) {
    int ret = 0;

    ret = dispatcher::init_driver();

    if (ret) return ret;

    return do_dispatcher_main(argc, argv);
}