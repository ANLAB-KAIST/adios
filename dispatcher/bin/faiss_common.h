#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>

template <typename Tp>
struct vecs_mmap {
    int fd;
    Tp *array;
    size_t fsz;
    size_t d;
    size_t n;

    void do_mmap(const std::filesystem::path &fpath) {
        assert((d > 0 && d < 1000000) || !"unreasonable dimension");
        fd = open(fpath.c_str(), O_RDONLY);
        if (fd == -1) {
            perror("Error opening file");
            abort();
        }

        struct stat fileInfo;
        if (fstat(fd, &fileInfo) == -1) {
            perror("Error getting file size");
            close(fd);
            abort();
        }
        fsz = fileInfo.st_size;
        void *mappedFile = mmap(nullptr, fsz, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mappedFile == MAP_FAILED) {
            perror("Error mapping file");
            close(fd);
            abort();
        }

        assert(fsz % (d * sizeof(Tp)) == 0 || !"weird file size");
        n = fsz / (d * sizeof(Tp));

        array = (Tp *)mappedFile;
    }

    void do_unmap() {
        int ret = munmap((void *)array, fsz);
        (void)ret;
        assert(ret == 0);
        array = nullptr;
    }

    vecs_mmap(const char *fname) {
        const std::filesystem::path fpath(fname);

        auto stem = fpath.stem();
        auto inner = stem.extension();
        auto outer = fpath.extension();

        if (!std::filesystem::is_regular_file(fpath)) {
            abort();
        }

        if constexpr (std::is_same_v<Tp, int8_t>) {
            if (inner != ".bvecs") {
                abort();
            }
        } else if constexpr (std::is_same_v<Tp, int32_t>) {
            if (inner != ".ivecs") {
                abort();
            }
        } else if constexpr (std::is_same_v<Tp, float>) {
            if (inner != ".fvecs") {
                abort();
            }
        } else {
            abort();
        }
        d = std::stoi(outer.c_str() + 1);

        do_mmap(fpath);
    }

    vecs_mmap(const std::filesystem::path &parent, const char *fname) {
        std::string fpath_str = "";
        for (auto const &dir_entry :
             std::filesystem::directory_iterator{parent}) {
            auto fpath = dir_entry.path();
            auto stem = fpath.stem();
            auto inner = stem.extension();
            auto outer = fpath.extension();
            if (outer.empty()) {
                continue;
            }
            stem = stem.stem();

            if constexpr (std::is_same_v<Tp, int8_t>) {
                if (inner != ".bvecs") {
                    continue;
                }
            } else if constexpr (std::is_same_v<Tp, int32_t>) {
                if (inner != ".ivecs") {
                    continue;
                }
            } else if constexpr (std::is_same_v<Tp, float>) {
                if (inner != ".fvecs") {
                    continue;
                }
            } else {
                abort();
            }

            if (stem == fname) {
                if (!fpath_str.empty()) {
                    printf("ambigious: %s\n", fname);
                    abort();
                }
                fpath_str = fpath.c_str();
                d = std::stoi(outer.c_str() + 1);
            }
            continue;
        }
        if (fpath_str.empty()) {
            perror("Not found");
            abort();
        }

        do_mmap(fpath_str);
    }
    ~vecs_mmap() { do_unmap(); }
};

using bvecs_mmap = vecs_mmap<int8_t>;
using ivecs_mmap = vecs_mmap<int32_t>;
using fvecs_mmap = vecs_mmap<float>;
