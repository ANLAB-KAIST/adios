#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
//
// FIXUP
#define static_assert(_cond, msg)
#include <infiniband/kern-abi.h>
#include <rdma/mlx5-abi.h>
#include <rdma/mlx5_user_ioctl_cmds.h>
#include <rdma/rdma_user_ioctl_cmds.h>

#include "virtio-uverbs-trans.h"
//
#include <virtio-uverbs/ibverbs.h>
#include <virtio-uverbs/ioctl.h>
#include <virtio-uverbs/mlx5.h>

// out_bytes, in_bytes
// Note: out_bytes: device -> verbs
//       in bytes:  verbs -> device
static int get_io_bytes(bool is_ex, const char *buff, uint64_t *out_bytes,
                        uint64_t *in_bytes) {
    const struct ib_uverbs_cmd_hdr *hdr =
        (const struct ib_uverbs_cmd_hdr *)buff;

    if (is_ex) {
        *in_bytes = hdr->in_words * 8;
        *out_bytes = hdr->out_words * 8;
        const struct ex_hdr *ex_hdr = (const struct ex_hdr *)(buff);
        *in_bytes += ex_hdr->ex_hdr.provider_in_words * 8;
        *out_bytes += ex_hdr->ex_hdr.provider_out_words * 8;

        *in_bytes += sizeof(struct ex_hdr);
    } else {
        *in_bytes = hdr->in_words * 4;
        *out_bytes = hdr->out_words * 4;
    }

    return 0;
}

int virtio_uverbs_translate_ofed(char *buf, void **out_addr,
                                 uint64_t *out_bytes, uint64_t *in_bytes) {
    if (buf == NULL) return -1;

    uint32_t command;
    bool ext = false;
    bool exp = false;

    int err =
        parse_hdr((const struct ib_uverbs_cmd_hdr *)buf, &command, &ext, &exp);
    if (err) return err;

    if (ext)
        ibverbs_translate_ext(command, buf);
    else
        ibverbs_translate_general(command, buf);
    if (ext)
        mlx5_translate_ext(command, buf);
    else
        mlx5_translate_general(command, buf);

    err = get_io_bytes(ext, buf, out_bytes, in_bytes);
    if (err) return err;

    *out_addr = *(void **)(buf + sizeof(struct ib_uverbs_cmd_hdr));

    return 0;
}

int virtio_uverbs_translate_ioctl(struct ib_uverbs_ioctl_hdr *hdr) {
    return ioctl_translate(hdr);
}