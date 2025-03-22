#pragma once
#include <chrono>
#include <cstdio>

class ME {
   public:
    ME(const char *msg) : msg(msg), total(0) { count = 0; }

    void start() { ss = std::chrono::high_resolution_clock::now(); }
    void end() {
        auto ee = std::chrono::high_resolution_clock::now();
        auto tspan =
            std::chrono::duration_cast<std::chrono::nanoseconds>(ee - ss);
        total += tspan;
        count += 1;

        if ((count % 10000) == 0) {
            printf("%s: %ld\n", msg, total.count() / count);
        }
    }

    const char *msg;
    std::chrono::high_resolution_clock::time_point ss;
    std::chrono::nanoseconds total;
    size_t count;
};