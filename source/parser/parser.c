#include "parser.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/err.h"
#include <linux/unaligned.h>
#include <linux/types.h>

int parse_to_inode(const void *data, size_t len, vtfs_server_inode *result) {
    if (!data || !result) {
        return -EINVAL;
    }

    if (len != VTFS_INODE_SERVER_DATA_SIZE) {
        return -EPROTO;
    }

    const u8 *buf = data;

    result->inode_id = get_unaligned_be64(buf);
    result->type = buf[8];
    result->size_bytes = get_unaligned_be64(buf + 9);
    result->nlink = get_unaligned_be64(buf + 17);
    result->created_at = (s64)get_unaligned_be64(buf + 25);
    result->modified_at = (s64)get_unaligned_be64(buf + 33);
    result->changhed_at = (s64)get_unaligned_be64(buf + 41);

    if (result->type != VTFS_SERVER_FILE_TYPE && result->type != VTFS_SERVER_DIR_TYEPE) {
        return -EPROTO;
    }

    if (result->inode_id == 0) {
        return -EPROTO;
    }

    return 0;
}

int parse_to_error_code(const void *data, size_t len, int *error_code) {
    if (!data || !error_code) {
        return -EINVAL;
    }

    if (len != 4) {
        return -EPROTO;
    }

    const u8 *buf = data;

    int parsed = (int)get_unaligned_be32(buf);

    if (parsed == 0 || parsed > MAX_ERRNO) {
        return -EPROTO;
    }

    *error_code = -parsed;

    return 0;
}
