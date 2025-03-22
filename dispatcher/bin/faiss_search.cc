#include <faiss/AutoTune.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <type_traits>

#include "faiss_common.h"

extern "C" {
int enable_map_ddc = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf(
            "Usage: faiss-create <bigann directorypath> <faiss index dir>\n");
        return 1;
    }

    std::filesystem::path bigann_path(argv[1]);
    std::filesystem::path faiss_index_dir(argv[2]);

    auto db_path = faiss_index_dir / "db.index";

    faiss::Index *index;

    index = faiss::read_index(db_path.c_str(),
                              faiss::IO_FLAG_READ_ONLY | faiss::IO_FLAG_MMAP);
    printf("index:%p\n", index);

    auto bigann_query = fvecs_mmap(bigann_path, "bigann_query");
    size_t d = bigann_query.d;

    size_t k = 10;
    std::vector<float> new_D(k);
    std::vector<faiss::idx_t> new_I(k);
    for (size_t i = 0; i < bigann_query.n; ++i) {
        // bigann_query.array[i * d];

        std::chrono::high_resolution_clock::time_point start =
            std::chrono::high_resolution_clock::now();
        index->search(1, &bigann_query.array[i * d], k, new_D.data(),
                      new_I.data());
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();
        auto tspan = std::chrono::duration_cast<std::chrono::duration<double>>(
            end - start);
        printf("%ld: %lf\n", i, tspan.count());
    }

    printf("done...\n");
    return 0;
}