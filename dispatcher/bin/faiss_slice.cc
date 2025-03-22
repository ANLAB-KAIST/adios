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
    if (argc != 4) {
        printf("Usage: faiss-slice <from> <to> <M>\n");
        return 1;
    }

    std::filesystem::path from(argv[1]);
    std::filesystem::path to(argv[2]);
    int M = std::stoi(std::string(argv[3]));

    printf("converting %s to %s (%dM)\n", from.c_str(), to.c_str(), M);

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
    void *mappedFile = mmap(nullptr, fsz, PROT_READ, MAP_PRIVATE, from_fd, 0);
    if (mappedFile == MAP_FAILED) {
        perror("Error mapping file");
        close(from_fd);
        abort();
    }

    int32_t d = *(int32_t *)mappedFile;
    assert((d > 0 && d < 1000000) || !"unreasonable dimension");
    assert(fsz % (4 + d * element_size) == 0 || !"weird file size");

    printf("converting to %s\n", to.c_str());

    size_t n = fsz / (4 + d * element_size);

    FILE *to_fd = fopen(to.c_str(), "w");
    if (to_fd == nullptr) {
        perror("Error opening file");
        abort();
    }

    size_t new_n = M * 1000 * 1000;
    size_t new_fsz = new_n * (4 + d * element_size);
    assert(new_fsz <= fsz);

    size_t items_written = fwrite(mappedFile, 1, new_fsz, to_fd);
    assert(items_written == new_fsz);

    fclose(to_fd);

    int ret = munmap(mappedFile, fsz);
    assert(ret == 0);
    ret = close(from_fd);
    assert(ret == 0);

    return 0;
}