#ifdef DDC

#include <dispatcher/api.hh>
#include <dispatcher/loader.hh>

extern "C" {

int dispatcher_main(int argc, char **argv) {
    return do_dispatcher_main(argc, argv);
}
}

#endif