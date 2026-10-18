#include "parser.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/err.h"
#include "linux/gfp_types.h"
#include "linux/slab.h"
#include "net/neighbour.h"
#include <linux/unaligned.h>
#include <linux/types.h>
#include <linux/string.h>

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

static int parse_to_iterate_entry(const u8 *data, size_t len, vtfs_iterate_entry *entry) {
    if (len < 19) {
        return -EPROTO;
    }

    entry->cookie = get_unaligned_be64(data);
    entry->inode_id = get_unaligned_be64(data + 8);
    entry->type = data[16];

    if (entry->type != VTFS_SERVER_FILE_TYPE && entry->type != VTFS_SERVER_DIR_TYEPE) {
        return -EPROTO;
    }

    entry->name_length = get_unaligned_be16(data + 17);

    if (entry->name_length <= 0) {
        return -EPROTO;
    }

    if (entry->name_length > len - 19) {
        return -EPROTO;
    }

    entry->name = kmalloc((size_t)entry->name_length + 1, GFP_KERNEL);

    if (!entry->name) {
        return -ENOMEM;
    }

    memcpy(entry->name, data + 19, entry->name_length);

    entry->name[entry->name_length] = '\0';

    return 0;
}

static void free_iterate_entries(vtfs_iterate_result *result, size_t len) {
    for (size_t i = 0; i < len; i++) {
        kfree(result->entries[i].name);
    }

    result->entries = NULL;
}

int parse_to_iterate_result(const void *data, size_t len, vtfs_iterate_result *result) {
    if (!data || !result) {
        return -EINVAL;
    }

    if (len < 13) {
        return -EINVAL;
    }

    const u8 *buf = data;

    result->next_cookie = get_unaligned_be64(buf);
    result->entry_count = get_unaligned_be32(buf + 8);
    result->end_of_file = buf[12];

    result->entries = kcalloc(result->entry_count, sizeof(vtfs_iterate_entry), GFP_KERNEL);

    if (result->entry_count > 0 && !result->entries) {
        return -ENOMEM;
    }

    const u8 *entries = buf + 13;

    size_t offset = 0;

    for (size_t i = 0; i < result->entry_count; i++) {
        vtfs_iterate_entry entry = {0};

        int code = parse_to_iterate_entry(entries + offset, len - offset - 13, &entry);

        if (code < 0) {
            free_iterate_entries(result, i);
            return code;
        }

        offset += 19 + entry.name_length;

        if (offset + 13 > len) {
            free_iterate_entries(result, i);
            kfree(entry.name);
            return -EINVAL;
        }

        result->entries[i] = entry;
    }

    if (offset != len - 13) {
        free_iterate_entries(result, result->entry_count);
        return -EPROTO;
    }

    return 0;
}
