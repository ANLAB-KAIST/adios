#include <faiss/index_factory.h>
#include <faiss/index_io.h>

#include <dispatcher/api.hh>
#include <dispatcher/app.hh>
#include <dispatcher/common.hh>
#include <filesystem>
#include <iostream>
#include <vector>

extern "C" {

constexpr size_t faiss_k = 10;

static faiss::Index *faiss_index;

int dp_addon_faiss_init(const std::string path) {
    faiss_index = faiss::read_index(
        path.c_str(), faiss::IO_FLAG_READ_ONLY | faiss::IO_FLAG_MMAP);
    return 0;
}

int dp_addon_faiss_query(const float vec_in[], uint64_t vlen) {
    float distance[faiss_k];
    faiss::idx_t idxs[faiss_k];
    faiss_index->search(1, vec_in, faiss_k, distance, idxs);
    return 0;
}
}