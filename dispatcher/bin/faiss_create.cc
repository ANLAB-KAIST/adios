#include <faiss/AutoTune.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <type_traits>

#include "faiss_common.h"

constexpr size_t DB_SIZE = 100 * 1000 * 1000;
const char *INDEX_KEY = "IVF4096,Flat";

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf(
            "Usage: faiss-create <bigann directorypath> <faiss index dir>\n");
        return 1;
    }

    std::filesystem::path bigann_path(argv[1]);
    std::filesystem::path faiss_index_dir(argv[2]);

    auto bigann_base = fvecs_mmap(bigann_path, "bigann_base_100M");
    auto bigann_learn = fvecs_mmap(bigann_path, "bigann_learn");
    auto bigann_query = fvecs_mmap(bigann_path, "bigann_query");

    // std::string gnd_name = "idx_";
    // gnd_name += std::to_string(DB_SIZE / 1000000);
    // gnd_name += "M";

    // auto bigann_gnd = fvecs_mmap(bigann_path / "gnd", gnd_name.c_str());

    // printf("[bigann_base] d: %ld, n: %ld\n", bigann_base.d, bigann_base.n);
    // printf("[bigann_learn] d: %ld, n: %ld\n", bigann_learn.d,
    // bigann_learn.n); printf("[bigann_query] d: %ld, n: %ld\n",
    // bigann_query.d, bigann_query.n); printf("[bigann_gnd] gnd: %s d: %ld, n:
    // %ld\n", gnd_name.c_str(),
    //        bigann_gnd.d, bigann_gnd.n);

    assert(bigann_base.d == bigann_learn.d);
    assert(bigann_base.d == bigann_query.d);
    // assert(bigann_base.d == bigann_gnd.d);

    auto db_path = faiss_index_dir / "db.index";

    printf("building index...\n");

    faiss::Index *index;

    index = faiss::index_factory(bigann_learn.d, INDEX_KEY);
    printf("training...\n");
    index->train(bigann_learn.n, bigann_learn.array);
    printf("adding...\n");
    index->add(bigann_base.n, bigann_base.array);
    faiss::write_index(index, db_path.c_str());
    printf("done...\n");
    return 0;
}