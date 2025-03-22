#pragma once

#include <cstdarg>
#include <cstdio>
#include <dispatcher/common.hh>

#define PRINT_LOG(format, ...) do_print_log(format, ##__VA_ARGS__)
#define PRINT_DEBUG_LOG(format, ...) do_print_log(format, ##__VA_ARGS__)
#define SET_MODULE_NAME(ptr) do_set_module_name(ptr)
#define SET_MODULE_NAME_ID(ptr, id) do_set_module_name_id(ptr, id)

namespace dispatcher {

class logger_t {
   protected:
    inline void do_print_log(const char *format, ...) const {
        if (gcc_likely(module_name))
            std::printf("[%s_%d] ", module_name, module_id);

        va_list args;
        va_start(args, format);
        std::vprintf(format, args);
        va_end(args);
    }
    inline void do_set_module_name(const char *ptr) {
        module_name = ptr;
        module_id = 0;
    }
    inline void do_set_module_name_id(const char *ptr, int id) {
        module_name = ptr;
        module_id = id;
    }

   private:
    const char *module_name;
    int module_id;
};

}  // namespace dispatcher