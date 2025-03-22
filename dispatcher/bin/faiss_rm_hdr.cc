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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: faiss-rm-hdr <files>\n");
        return 1;
    }

    for (size_t argi = 1; argi < argc; ++argi) {
        std::filesystem::path from(argv[argi]);

        printf("converting %s\n", from.c_str());

        auto ext = from.extension();

        size_t element_size = 0;

        if (ext == ".bvecs") {
            element_size = 1;
        } else if (ext == ".fvecs") {
            element_size = 4;
        } else if (ext == ".ivecs") {
            element_size = 4;
        }
        assert(element_size != 0);

        int from_fd = open(from.c_str(), O_RDONLY);
        if (from_fd == -1) {
            perror("Error opening file");
            abort();
        }

        struct stat fileInfo;
        if (fstat(from_fd, &fileInfo) == -1) {
            perror("Error getting file size");
            close(from_fd);
            abort();
        }
        size_t fsz = fileInfo.st_size;
        void *mappedFile =
            mmap(nullptr, fsz, PROT_READ, MAP_PRIVATE, from_fd, 0);
        if (mappedFile == MAP_FAILED) {
            perror("Error mapping file");
            close(from_fd);
            abort();
        }

        int32_t d = *(int32_t *)mappedFile;
        assert((d > 0 && d < 1000000) || !"unreasonable dimension");
        assert(fsz % (4 + d * element_size) == 0 || !"weird file size");

        auto new_ext = std::string("fvecs") + "." + std::to_string(d);
        auto to = from.replace_extension(new_ext);

        printf("converting to %s\n", to.c_str());

        size_t n = fsz / (4 + d * element_size);

        FILE *to_fd = fopen(to.c_str(), "w");
        if (to_fd == nullptr) {
            perror("Error opening file");
            abort();
        }

        size_t items_written = 0;
        uint8_t *data = (uint8_t *)mappedFile;
        float buffer[d];

        if (ext == ".bvecs") {
            for (size_t i = 0; i < n; ++i) {
                int8_t *buffer_from =
                    (int8_t *)(data + i * (d * element_size + sizeof(int32_t)) +
                               sizeof(int32_t));
                for (size_t j = 0; j < d; ++j) {
                    buffer[j] = (float)buffer_from[j];
                }
                items_written = fwrite(buffer, sizeof(float), d, to_fd);
                assert(items_written == d);
            }
        } else if (ext == ".fvecs") {
            for (size_t i = 0; i < n; ++i) {
                float *buffer_from =
                    (float *)(data + i * (d * element_size + sizeof(int32_t)) +
                              sizeof(int32_t));
                items_written = fwrite(buffer_from, sizeof(float), d, to_fd);
                assert(items_written == d);
            }
        } else if (ext == ".ivecs") {
            for (size_t i = 0; i < n; ++i) {
                int32_t *buffer_from =
                    (int32_t *)(data +
                                i * (d * element_size + sizeof(int32_t)) +
                                sizeof(int32_t));
                for (size_t j = 0; j < d; ++j) {
                    buffer[j] = (float)buffer_from[j];
                }
                items_written = fwrite(buffer, sizeof(float), d, to_fd);
                assert(items_written == d);
            }
        }

        fclose(to_fd);

        int ret = munmap(mappedFile, fsz);
        assert(ret == 0);
        ret = close(from_fd);
        assert(ret == 0);
    }
    return 0;
}