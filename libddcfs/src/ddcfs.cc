#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cassert>
#include <climits>
#include <cstring>
#include <ddcfs.hh>
#include <mutex>

namespace ddcfs {

static std::mutex ddcfs_lock;
static uint64_t inode_count = 1; /* inode 0 is reserved to root */
static constexpr size_t IO_APPEND = 0x0001;
static constexpr size_t IO_SYNC = 0x0002;

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)
#define round_page(x) (((x) + PAGE_MASK) & ~PAGE_MASK)

enum uio_rw { UIO_READ, UIO_WRITE };

/*
 * Safe default to prevent possible overflows in user code, otherwise could
 * be SSIZE_T_MAX.
 */
#define IOSIZE_MAX INT_MAX

#undef UIO_MAXIOV
#define UIO_MAXIOV 1024

#define UIO_SYSSPACE 0

struct uio {
    struct iovec *uio_iov; /* scatter/gather list */
    int uio_iovcnt;        /* length of scatter/gather list */
    off_t uio_offset;      /* offset in target object */
    ssize_t uio_resid;     /* remaining bytes to process */
    enum uio_rw uio_rw;    /* operation */
};

static int uiomove(void *cp, int n, struct uio *uio) {
    assert(uio->uio_rw == UIO_READ || uio->uio_rw == UIO_WRITE);

    while (n > 0 && uio->uio_resid) {
        struct iovec *iov = uio->uio_iov;
        int cnt = iov->iov_len;
        if (cnt == 0) {
            uio->uio_iov++;
            uio->uio_iovcnt--;
            continue;
        }
        if (cnt > n) cnt = n;

        if (uio->uio_rw == UIO_READ)
            memcpy(iov->iov_base, cp, cnt);
        else
            memcpy(cp, iov->iov_base, cnt);

        iov->iov_base = (char *)iov->iov_base + cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= cnt;
        uio->uio_offset += cnt;
        cp = (char *)cp + cnt;
        n -= cnt;
    }

    return 0;
}

static size_t strlcpy(char *dest, const char *src, size_t size) {
    size_t len = strlen(src);
    if (size > 0) {
        strncpy(dest, src, size - 1);
        dest[size - 1] = '\0';
    }
    return len;
}

static void set_times_to_now(struct timespec *time1,
                             struct timespec *time2 = nullptr,
                             struct timespec *time3 = nullptr) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    if (time1) {
        memcpy(time1, &now, sizeof(struct timespec));
    }
    if (time2) {
        memcpy(time2, &now, sizeof(struct timespec));
    }
    if (time3) {
        memcpy(time3, &now, sizeof(struct timespec));
    }
}

static struct node_t *allocate_node(const char *name, int type) {
    struct node_t *np;

    np = (node_t *)malloc(sizeof(struct node_t));
    if (np == NULL) return NULL;
    memset(np, 0, sizeof(struct node_t));

    np->rn_namelen = strlen(name);
    np->rn_name = (char *)malloc(np->rn_namelen + 1);
    if (np->rn_name == NULL) {
        free(np);
        return NULL;
    }
    strlcpy(np->rn_name, name, np->rn_namelen + 1);
    np->rn_type = type;

    if (type == VDIR)
        np->rn_mode = S_IFDIR | 0777;
    else if (type == VLNK)
        np->rn_mode = S_IFLNK | 0777;
    else
        np->rn_mode = S_IFREG | 0777;

    set_times_to_now(&(np->rn_ctime), &(np->rn_atime), &(np->rn_mtime));
    np->rn_owns_buf = true;
    np->rn_ref_count = 0;
    np->rn_removed = false;

    np->rn_file_segments_by_offset = new std::map<off_t, struct file_segment>();
    np->rn_total_segments_size = 0;

    return np;
}

static void free_node(struct node_t *np) {
    if (!np->rn_removed || np->rn_ref_count > 0) {
        return;
    }

    if (np->rn_file_segments_by_offset != NULL) {
        if (np->rn_owns_buf) {
            for (auto it = np->rn_file_segments_by_offset->begin();
                 it != np->rn_file_segments_by_offset->end(); ++it) {
                munmap(it->second.data, it->second.size);
                it->second.data = NULL;
            }
        }

        delete np->rn_file_segments_by_offset;
    }

    free(np->rn_name);
    free(np);
}

static struct node_t *add_node(struct node_t *dnp, char *name, int type) {
    struct node_t *np, *prev;

    np = allocate_node(name, type);
    if (np == NULL) return NULL;

    ddcfs_lock.lock();
    np->inode_no = inode_count++;

    /* Link to the directory list */
    if (dnp->rn_child == NULL) {
        dnp->rn_child = np;
    } else {
        prev = dnp->rn_child;
        while (prev->rn_next != NULL) prev = prev->rn_next;
        prev->rn_next = np;
    }

    set_times_to_now(&(dnp->rn_mtime), &(dnp->rn_ctime));

    ddcfs_lock.unlock();
    return np;
}

static int remove_node(struct node_t *dnp, struct node_t *np) {
    struct node_t *prev;

    if (dnp->rn_child == NULL) return EBUSY;

    ddcfs_lock.lock();

    /* Unlink from the directory list */
    if (dnp->rn_child == np) {
        dnp->rn_child = np->rn_next;
    } else {
        for (prev = dnp->rn_child; prev->rn_next != np; prev = prev->rn_next) {
            if (prev->rn_next == NULL) {
                ddcfs_lock.unlock();
                return ENOENT;
            }
        }
        prev->rn_next = np->rn_next;
    }

    np->rn_removed = true;
    if (np->rn_ref_count <= 0) {
        free_node(np);
    }

    set_times_to_now(&(dnp->rn_mtime), &(dnp->rn_ctime));

    ddcfs_lock.unlock();
    return 0;
}

static int rename_node(struct node_t *np, char *name) {
    size_t len;
    char *tmp;

    len = strlen(name);
    if (len > NAME_MAX) {
        return ENAMETOOLONG;
    }
    if (len <= np->rn_namelen) {
        /* Reuse current name buffer */
        strlcpy(np->rn_name, name, np->rn_namelen + 1);
    } else {
        /* Expand name buffer */
        tmp = (char *)malloc(len + 1);
        if (tmp == NULL) return ENOMEM;
        strlcpy(tmp, name, len + 1);
        free(np->rn_name);
        np->rn_name = tmp;
    }
    np->rn_namelen = len;
    set_times_to_now(&(np->rn_ctime));
    return 0;
}

static int lookup(struct node_t *dnp, char *name, struct node_t **npp) {
    struct node_t *np;
    size_t len;
    int found;

    *npp = NULL;

    if (*name == '\0') return ENOENT;

    ddcfs_lock.lock();

    len = strlen(name);

    found = 0;
    for (np = dnp->rn_child; np != NULL; np = np->rn_next) {
        if (np->rn_namelen == len && memcmp(name, np->rn_name, len) == 0) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        ddcfs_lock.unlock();
        return ENOENT;
    }

    ddcfs_lock.unlock();

    *npp = np;

    return 0;
}

static int ddcfs_mkdir(struct node_t *dnp, char *name, mode_t mode) {
    struct node_t *np;

    if (strlen(name) > NAME_MAX) {
        return ENAMETOOLONG;
    }

    if (!S_ISDIR(mode)) return EINVAL;

    np = add_node(dnp, name, VDIR);
    if (np == NULL) return ENOMEM;
    np->rn_size = 0;

    return 0;
}

static int ddcfs_symlink(struct node_t *dnp, char *name, char *link) {
    if (strlen(name) > NAME_MAX) {
        return ENAMETOOLONG;
    }
    auto np = add_node(dnp, name, VLNK);
    if (np == NULL) return ENOMEM;
    // Save the link target without the final null, as readlink() wants it.
    size_t len = strlen(link);
    np->rn_size = len;

    file_segment new_segment = {
        .size = len,
        .data = strndup(link, len)  // this is only for link do not ddc
    };
    (*np->rn_file_segments_by_offset)[0] = new_segment;
    np->rn_total_segments_size = len;

    return 0;
}

static int ddcfs_readlink(struct node_t *np, struct uio *uio) {
    size_t len;

    if (np->rn_type != VLNK) {
        return EINVAL;
    }
    if (uio->uio_offset < 0) {
        return EINVAL;
    }
    if (uio->uio_resid == 0) {
        return 0;
    }
    if (uio->uio_offset >= (off_t)np->rn_size) return 0;
    if (np->rn_size - uio->uio_offset < uio->uio_resid)
        len = np->rn_size - uio->uio_offset;
    else
        len = uio->uio_resid;

    set_times_to_now(&(np->rn_atime));
    return uiomove((*np->rn_file_segments_by_offset)[0].data + uio->uio_offset,
                   len, uio);
}

/* Remove a directory */
static int rmdir(struct node_t *dnp, struct node_t *np, char *name) {
    return remove_node(dnp, np);
}

/* Remove a file */
static int remove(struct node_t *dnp, struct node_t *np, char *name) {
    // DPRINTF(("remove %s in %s\n", name, dvp->v_path));
    return remove_node(dnp, np);
}

/*
 * Create empty file.
 */
int create(struct node_t *dvp, char *name, mode_t mode) {
    struct node_t *np;
    if (strlen(name) > NAME_MAX) {
        return ENAMETOOLONG;
    }

    if (!S_ISREG(mode)) return EINVAL;

    np = add_node(dvp, name, VREG);
    if (np == NULL) return ENOMEM;
    return 0;
}

static int read_or_write_file_data(
    std::map<off_t, struct file_segment> &file_segments_by_offset,
    struct uio *uio, size_t bytes_to_read_or_write) {
    // Find the segment where we need to start reading from or writing to based
    // on uio_offset
    auto segment_to_read_or_write =
        file_segments_by_offset.lower_bound(uio->uio_offset);
    if (segment_to_read_or_write == file_segments_by_offset.end()) {
        segment_to_read_or_write = --file_segments_by_offset.end();
    } else if (segment_to_read_or_write->first > uio->uio_offset) {
        segment_to_read_or_write--;
    }
    assert(uio->uio_offset >= segment_to_read_or_write->first &&
           uio->uio_offset < (off_t)(segment_to_read_or_write->first +
                                     segment_to_read_or_write->second.size));

    // Simply iterate starting with initial identified segment above to read or
    // write until we have read all bytes or have iterated all segments
    uint64_t file_offset = uio->uio_offset;
    for (; segment_to_read_or_write != file_segments_by_offset.end() &&
           bytes_to_read_or_write > 0;
         segment_to_read_or_write++) {
        // First calculate where we need to start in this segment ...
        auto offset_in_segment = file_offset - segment_to_read_or_write->first;
        // .. then calculate how many bytes to read or write in this segment
        auto maximum_bytes_to_read_or_write_in_segment =
            segment_to_read_or_write->second.size - offset_in_segment;
        auto bytes_to_read_or_write_in_segment = std::min<uint64_t>(
            bytes_to_read_or_write, maximum_bytes_to_read_or_write_in_segment);
        assert(offset_in_segment >= 0 &&
               offset_in_segment < segment_to_read_or_write->second.size);
        auto ret =
            uiomove(segment_to_read_or_write->second.data + offset_in_segment,
                    bytes_to_read_or_write_in_segment, uio);
        if (ret) {
            return ret;
        }
        bytes_to_read_or_write -= bytes_to_read_or_write_in_segment;
        file_offset += bytes_to_read_or_write_in_segment;
    }

    return 0;
}

#define ENLARGE_FACTOR 1.1f
static int enlarge_data_buffer(struct node_t *np, size_t desired_length) {
    // New total size has to be at least greater by the ENLARGE_FACTOR
    auto new_total_segment_size = round_page(std::max<size_t>(
        np->rn_total_segments_size * ENLARGE_FACTOR, desired_length));
    assert(new_total_segment_size >= desired_length);

    auto new_segment_size =
        np->rn_owns_buf ? new_total_segment_size - np->rn_total_segments_size
                        : new_total_segment_size;
    file_segment new_segment = {
        .size = new_segment_size,
        .data = (char *)mmap(nullptr, new_segment_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
    };
    if (!new_segment.data || new_segment.data == MAP_FAILED) return EIO;

    auto new_segment_file_offset = np->rn_total_segments_size;
    if (!np->rn_owns_buf) {
        // This is a file from bootfs and we cannot simply enlarge same way
        // as regular one because because original segment is not be
        // page-aligned. Therefore we need to created new segment and copy
        // content of the old one
        memcpy(new_segment.data, (*np->rn_file_segments_by_offset)[0].data,
               np->rn_size);
        new_segment_file_offset = 0;
    }

    // Insert new segment with the file offset it starts at which happends
    // to be old total segments size
    (*np->rn_file_segments_by_offset)[new_segment_file_offset] = new_segment;
    np->rn_total_segments_size = new_total_segment_size;
    np->rn_owns_buf = true;

    return 0;
}

/* Truncate file */
static int truncate(struct node_t *np, off_t length) {
    // DPRINTF(("truncate %s length=%d\n", vp->v_path, length));
    // np = (ddcfs_node *)vp->v_data;

    // TODO: Shall we free segments id new length < np->total_segments_size
    if (length == 0) {
        if (np->rn_owns_buf) {
            for (auto it = (*np->rn_file_segments_by_offset).begin();
                 it != (*np->rn_file_segments_by_offset).end(); ++it) {
                munmap(it->second.data, it->second.size);
            }
        }

        (*np->rn_file_segments_by_offset).clear();
        np->rn_total_segments_size = 0;
    } else if (size_t(length) > np->rn_total_segments_size) {
        auto ret = enlarge_data_buffer(np, length);
        if (ret) {
            return ret;
        }
    }

    np->rn_size = length;

    set_times_to_now(&(np->rn_mtime), &(np->rn_ctime));
    return 0;
}

static int read(struct node_t *np, struct uio *uio) {
    size_t len;

    if (np->rn_mode == S_IFDIR | 0777) {
        return EISDIR;
    }
    if (np->rn_mode != S_IFREG | 0777) {
        return EINVAL;
    }
    if (uio->uio_offset < 0) {
        return EINVAL;
    }
    if (uio->uio_resid == 0) {
        return 0;
    }

    if (uio->uio_offset >= (off_t)np->rn_size) return 0;

    if (np->rn_size - uio->uio_offset < uio->uio_resid)
        len = np->rn_size - uio->uio_offset;
    else
        len = uio->uio_resid;

    set_times_to_now(&(np->rn_atime));

    return read_or_write_file_data(*(np->rn_file_segments_by_offset), uio, len);
}
static int write(struct node_t *np, struct uio *uio, int ioflag) {
    if (np->rn_mode == S_IFDIR | 0777) {
        return EISDIR;
    }
    if (np->rn_mode != S_IFREG | 0777) {
        return EINVAL;
    }
    if (uio->uio_offset < 0) {
        return EINVAL;
    }
    if (uio->uio_offset >= LONG_MAX) {
        return EFBIG;
    }
    if (uio->uio_resid == 0) {
        return 0;
    }

    if (ioflag & IO_APPEND) uio->uio_offset = np->rn_size;

    if (size_t(uio->uio_offset + uio->uio_resid) > (size_t)np->rn_size) {
        /* Expand the file size before writing to it */
        off_t end_pos = uio->uio_offset + uio->uio_resid;
        if (end_pos > (off_t)np->rn_total_segments_size) {
            auto ret = enlarge_data_buffer(np, end_pos);
            if (ret) {
                return ret;
            }
        }
        np->rn_size = end_pos;
    }

    set_times_to_now(&(np->rn_mtime), &(np->rn_ctime));

    assert(uio->uio_offset + uio->uio_resid <=
           (off_t)(np->rn_total_segments_size));
    return read_or_write_file_data(*(np->rn_file_segments_by_offset), uio,
                                   uio->uio_resid);
}

static int rename(struct node_t *dnp1, struct node_t *np1, char *name1,
                  struct node_t *dnp2, struct node_t *np2, char *name2) {
    struct node_t *np, *old_np;
    int error;

    if (np2) {
        /* Remove destination file, first */
        error = remove_node(dnp2, np2);
        if (error) return error;
    }
    /* Same directory ? */
    if (dnp1 == dnp2) {
        /* Change the name of existing file */
        error = rename_node(np1, name2);
        if (error) return error;
    } else {
        /* Create new file or directory */
        old_np = dnp1;
        np = add_node(dnp2, name2, old_np->rn_type);
        if (np == NULL) return ENOMEM;

        np->rn_size = old_np->rn_size;
        np->rn_file_segments_by_offset = old_np->rn_file_segments_by_offset;
        np->rn_total_segments_size = old_np->rn_total_segments_size;
        np->rn_owns_buf = old_np->rn_owns_buf;

        old_np->rn_file_segments_by_offset = NULL;
        old_np->rn_owns_buf = false;

        /* Remove source file */
        remove_node(dnp1, np1);
    }
    return 0;
}

/*
 * @vp: vnode of the directory.
 */
static int readdir(struct node_t *dnp, struct file *fp, struct dirent *dir) {
    struct node_t *np;
    int i;

    ddcfs_lock.lock();

    set_times_to_now(&(dnp->rn_atime));

    if (fp->f_offset == 0) {
        dir->d_type = DT_DIR;
        strlcpy((char *)&dir->d_name, ".", sizeof(dir->d_name));
    } else if (fp->f_offset == 1) {
        dir->d_type = DT_DIR;
        strlcpy((char *)&dir->d_name, "..", sizeof(dir->d_name));
    } else {
        np = dnp->rn_child;
        if (np == NULL) {
            ddcfs_lock.unlock();
            return ENOENT;
        }

        for (i = 0; i != (fp->f_offset - 2); i++) {
            np = np->rn_next;
            if (np == NULL) {
                ddcfs_lock.unlock();
                return ENOENT;
            }
        }
        if (np->rn_type == VDIR)
            dir->d_type = DT_DIR;
        else if (np->rn_type == VLNK)
            dir->d_type = DT_LNK;
        else
            dir->d_type = DT_REG;
        strlcpy((char *)&dir->d_name, np->rn_name, sizeof(dir->d_name));
    }
    dir->d_fileno = fp->f_offset;
    //	dir->d_namelen = strlen(dir->d_name);

    fp->f_offset++;

    ddcfs_lock.unlock();
    return 0;
}

// static int getattr(struct node_t *np, struct vattr *attr) {
//     attr->va_nodeid = np->inode_no;
//     attr->va_size = np->rn_size;

//     attr->va_type = (vtype)np->rn_type;

//     memcpy(&(attr->va_atime), &(np->rn_atime), sizeof(struct timespec));
//     memcpy(&(attr->va_ctime), &(np->rn_ctime), sizeof(struct timespec));
//     memcpy(&(attr->va_mtime), &(np->rn_mtime), sizeof(struct timespec));

//     attr->va_mode = np->rn_mode;

//     return 0;
// }

// static int setattr(struct node_t *np, struct vattr *attr) {
//     if (attr->va_mask & AT_ATIME) {
//         memcpy(&(np->rn_atime), &(attr->va_atime), sizeof(struct timespec));
//     }

//     if (attr->va_mask & AT_CTIME) {
//         memcpy(&(np->rn_ctime), &(attr->va_ctime), sizeof(struct timespec));
//     }

//     if (attr->va_mask & AT_MTIME) {
//         memcpy(&(np->rn_mtime), &(attr->va_mtime), sizeof(struct timespec));
//     }

//     if (attr->va_mask & AT_MODE) {
//         np->rn_mode = attr->va_mode;
//     }

//     return 0;
// }

struct node_t *mount() { return allocate_node("/", VDIR); }
int unmount(struct node_t *mp) {
    // todo
    return 0;
}

int open(struct file *fp) {
    fp->np->rn_ref_count++;
    return 0;
}

int close(struct vnode *dvp, struct file *file) {
    file->np->rn_ref_count--;
    // free_node(file->np); // why free?
    return 0;
}
}  // namespace ddcfs