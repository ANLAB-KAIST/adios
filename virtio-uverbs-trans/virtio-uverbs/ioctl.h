#ifndef VIRTIO_UVERBS_TRANS_IOCTL_H
#define VIRTIO_UVERBS_TRANS_IOCTL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#ifndef LIBUVERBS_UPDATE_ATTR
#error LIBUVERBS_UPDATE_ATTR not found
#endif

static int ioctl_translate_uar_obj_alloc(
    struct ib_uverbs_ioctl_hdr *ioctl_hdr) {
    for (unsigned short i = 0; i < ioctl_hdr->num_attrs; ++i) {
        switch (ioctl_hdr->attrs[i].attr_id) {
            case MLX5_IB_ATTR_UAR_OBJ_ALLOC_MMAP_OFFSET:
            case MLX5_IB_ATTR_UAR_OBJ_ALLOC_MMAP_LENGTH:
            case MLX5_IB_ATTR_UAR_OBJ_ALLOC_PAGE_ID:
                LIBUVERBS_UPDATE_ATTR(&ioctl_hdr->attrs[i], data);
                break;
            default:
                break;
        }
    }

    return 0;
}

static int ioctl_translate_uar(struct ib_uverbs_ioctl_hdr *ioctl_hdr) {
    switch (ioctl_hdr->method_id) {
        case MLX5_IB_METHOD_UAR_OBJ_ALLOC:
            return ioctl_translate_uar_obj_alloc(ioctl_hdr);
        default:
            break;
    }
    return 0;
}

static int ioctl_translate(struct ib_uverbs_ioctl_hdr *ioctl_hdr) {
    int err = 0;
    // printf("[before ioctl_hdr] length: %u object_id: %u method_id: %u
    // num_attrs: %u\n",
    //        ioctl_hdr->length, ioctl_hdr->object_id, ioctl_hdr->method_id,
    //        ioctl_hdr->num_attrs);

    // for (unsigned short i = 0; i < ioctl_hdr->num_attrs; ++i) {
    //     printf("  [before ioctl_hdr.attr] attr_id: %u len: %d data: %lx\n",
    //            ioctl_hdr->attrs[i].attr_id, ioctl_hdr->attrs[i].len,
    //            ioctl_hdr->attrs[i].data);
    // }
    switch (ioctl_hdr->object_id) {
        case MLX5_IB_OBJECT_UAR:
            err = ioctl_translate_uar(ioctl_hdr);
        default:
            break;
    }
    // printf("[after ioctl_hdr] length: %u object_id: %u method_id: %u
    // num_attrs: %u\n",
    //        ioctl_hdr->length, ioctl_hdr->object_id, ioctl_hdr->method_id,
    //        ioctl_hdr->num_attrs);

    // for (unsigned short i = 0; i < ioctl_hdr->num_attrs; ++i) {
    //     printf("  [after ioctl_hdr.attr] attr_id: %u len: %d data: %lx\n",
    //            ioctl_hdr->attrs[i].attr_id, ioctl_hdr->attrs[i].len,
    //            ioctl_hdr->attrs[i].data);
    // }

    return err;
}

#ifdef __cplusplus
}
#endif
#endif