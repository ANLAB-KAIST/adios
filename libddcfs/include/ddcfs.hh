#pragma once
#include <cstdlib>
#include <map>

namespace ddcfs {

enum vtype {
    VNON,  /* no type */
    VREG,  /* regular file  */
    VDIR,  /* directory */
    VBLK,  /* block device */
    VCHR,  /* character device */
    VLNK,  /* symbolic link */
    VSOCK, /* socks */
    VFIFO, /* FIFO */
    VBAD
};

struct file_segment {
    size_t size;
    char *data;
};

struct node_t {
    struct node_t *rn_next;  /* next node in the same directory */
    struct node_t *rn_child; /* first child node */
    int rn_type;             /* file or directory */
    char *rn_name;           /* name (null-terminated) */
    size_t rn_namelen;       /* length of name not including terminator */
    size_t rn_size;          /* file size */
    uint64_t inode_no;

    /* Holds data for both symlinks and regular files.
     * Each ddcfs_file_segment holds single chunk of file and is keyed
     * in the map by its offset in that file; the first entry will have a key 0
     * and hold the very first chunk of the file. This way as file grows
     * we do not need to free old and allocate new memory buffer, instead
     * we only allocate new file segment for new chunk of the file and add
     * it to the map */
    std::map<off_t, struct file_segment> *rn_file_segments_by_offset;
    /* Sum of sizes of all segments - typically bigger than actual file size
     * We could have iterated over all entries in the map to calculate it
     * but it is faster to cache it as a field */
    size_t rn_total_segments_size;

    struct timespec rn_ctime;
    struct timespec rn_atime;
    struct timespec rn_mtime;

    int rn_mode;
    bool rn_owns_buf;
    int rn_ref_count;
    bool rn_removed;
};

struct file {
    struct node_t *np;
    off_t f_offset = 0;
};

struct node_t *mount();
int unmount(struct node_t *mp);

}  // namespace ddcfs