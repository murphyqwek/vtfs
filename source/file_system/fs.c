#include "fs.h"
#include "linux/fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "../helpers.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/dcache.h"
#include "linux/fs_types.h"
#include "linux/gfp_types.h"
#include "linux/stat.h"
#include "linux/stddef.h"
#include "../server/server.h"
#include "linux/types.h"
#include "linux/uaccess.h"

#include "../http_vtfs.h"
/**
 * fs_init_info - inits a new filesystem information
 * @block: pointer to superblock
 *
 * IMPORTANT: using only once at filesystem initialization
 *
 *
 * Return: 0 if all good, else error code
 */
int fs_init_info(struct super_block *block, const char *server_ip, u16 server_port,
                 const char *login) {
    if (!block) {
        ERR("fs_init_info get null pointer to super_block");
        return -EINVAL;
    }

    if (!server_ip || !login) {
        return -EINVAL;
    }

    vtfs_info *info = kzalloc(sizeof(vtfs_info), GFP_KERNEL);

    if (!info) {
        ERR("Couldn't allocate memory for filesystem info");
        return -ENOMEM;
    }

    vtfs_server_info *server = kzalloc(sizeof(vtfs_server_info), GFP_KERNEL);

    if (!server) {
        kfree(info);
        return -ENOMEM;
    }

    if (strscpy(server->server_ip, server_ip, VTFS_MAX_SERVER_LEN) < 0) {
        kfree(server);
        kfree(info);
        return -ENAMETOOLONG;
    }

    if (strscpy(server->login, login, VTFS_MAX_LOGIN_LEN) < 0) {
        kfree(server);
        kfree(info);
        return -ENAMETOOLONG;
    }

    server->server_port = server_port;
    info->server_info = server;
    block->s_fs_info = info;

    LOG("successfully initialized new filesystem info");

    return 0;
}

/**
 * fs_mount - calls server to mount
 * @server: server
 * @inode: server inode of root
 *
 *
 * Return: 0 if good else error code
 */
int fs_mount(vtfs_server_info *server, vtfs_server_inode *inode) {
    if (!inode) {
        return -EINVAL;
    }

    int code = server_mount(server, inode);

    return code;
}

/**
 * fs_free_info - free the information about filesystem instence
 * @block: pointer to superblock
 *
 *
 * Return: Nothing
 */
void fs_free_info(struct super_block *block) {
    if (!block) {
        return;
    }

    if (!block->s_fs_info) {
        return;
    }

    vtfs_info *info = block->s_fs_info;

    if (!info) {
        return;
    }

    kfree(info->server_info);
    kfree(info);

    block->s_fs_info = NULL;

    LOG("filesystem info was free");
}

/**
 * fs_lookup - realization of lookup
 * @block: super block
 * @parent: parent inode
 * @dentry: dentry
 * @server_inode: result inode
 *
 *
 * Return: 0 if all good else error code
 */
int fs_lookup(struct super_block *block, struct inode *parent, struct dentry *dentry,
              vtfs_server_inode *server_inode) {
    if (!block || !dentry || !server_inode || !parent) {
        return -EINVAL;
    }

    if (!S_ISDIR(parent->i_mode)) {
        return -ENOTDIR;
    }

    vtfs_info *info = block->s_fs_info;

    if (!info) {
        return -EINVAL;
    }

    vtfs_server_info *server = info->server_info;

    if (!server) {
        return -EINVAL;
    }

    char *name = kmemdup_nul(dentry->d_name.name, dentry->d_name.len, GFP_KERNEL);

    if (!name) {
        return -ENOMEM;
    }

    int code = server_lookup(server, name, (u64)parent->i_ino, server_inode);

    kfree(name);

    return code;
}

static int fs_create_init_base(struct super_block *block, struct inode *parent,
                               struct dentry *dentry, vtfs_server_inode *server_inode,
                               char **name) {
    if (!block || !parent || !dentry || !server_inode || !name) {
        return -EINVAL;
    }

    if (!S_ISDIR(parent->i_mode)) {
        return -ENOTDIR;
    }

    vtfs_info *info = block->s_fs_info;

    if (!info || !info->server_info) {
        return -EINVAL;
    }

    if (dentry->d_name.len == 0) {
        return -EINVAL;
    }

    if (dentry->d_name.len > VTFS_MAX_NAME_LEN) {
        return -ENAMETOOLONG;
    }

    *name = kmemdup_nul(dentry->d_name.name, dentry->d_name.len, GFP_KERNEL);

    if (!(*name)) {
        return -ENOMEM;
    }

    return 0;
}

int fs_create(struct super_block *block, struct inode *parent, struct dentry *dentry,
              vtfs_server_inode *server_inode) {
    char *name = NULL;

    int code = fs_create_init_base(block, parent, dentry, server_inode, &name);

    if (code < 0) {
        return code;
    }

    vtfs_info *info = block->s_fs_info;
    code = server_create(info->server_info, name, (u64)parent->i_ino, server_inode);

    kfree(name);

    return code;
}

int fs_mkdir(struct super_block *block, struct inode *parent, struct dentry *dentry,
             vtfs_server_inode *server_inode) {
    char *name = NULL;

    int code = fs_create_init_base(block, parent, dentry, server_inode, &name);

    if (code < 0) {
        return code;
    }

    vtfs_info *info = block->s_fs_info;
    code = server_mkdir(info->server_info, name, (u64)parent->i_ino, server_inode);

    kfree(name);

    return code;
}

static int fs_remove_base(struct inode *parent, struct dentry *dentry, bool directory) {
    if (!parent || !dentry) {
        return -EINVAL;
    }

    if (!S_ISDIR(parent->i_mode)) {
        return -ENOTDIR;
    }

    vtfs_info *info = parent->i_sb->s_fs_info;

    if (!info || !info->server_info) {
        return -EINVAL;
    }

    if (dentry->d_name.len == 0) {
        return -EINVAL;
    }

    if (dentry->d_name.len > VTFS_MAX_NAME_LEN) {
        return -ENAMETOOLONG;
    }

    char *name = kmemdup_nul(dentry->d_name.name, dentry->d_name.len, GFP_KERNEL);

    if (!name) {
        return -ENOMEM;
    }

    int code = 0;
    if (directory) {
        code = server_rmdir(info->server_info, name, (u64)parent->i_ino);
    } else {
        code = server_unlink(info->server_info, name, (u64)parent->i_ino);
    }

    kfree(name);

    return code;
}

int fs_unlink(struct inode *parent, struct dentry *dentry) {
    return fs_remove_base(parent, dentry, false);
}

int fs_rmdir(struct inode *parent, struct dentry *dentry) {
    return fs_remove_base(parent, dentry, true);
}

int fs_iterate(struct inode *directory, struct dir_context *ctx) {
    if (!directory || !ctx) {
        return -EINVAL;
    }

    if (!S_ISDIR(directory->i_mode)) {
        return -ENOTDIR;
    }

    vtfs_info *info = directory->i_sb->s_fs_info;

    if (!info || !info->server_info) {
        return -EINVAL;
    }

    if (ctx->pos < 2) {
        return -EINVAL;
    }

    for (;;) {
        vtfs_iterate_result result = {0};

        u64 cookie = ctx->pos - 2;

        int code = server_iterate(info->server_info, (u64)directory->i_ino, cookie, &result);

        if (code < 0) {
            server_free_iterate_data(&result);
            return code;
        }

        if (result.entry_count == 0) {
            bool eof = (bool)result.end_of_file;

            server_free_iterate_data(&result);

            return eof == true ? 0 : -EPROTO;
        }

        for (size_t i = 0; i < result.entry_count; i++) {
            vtfs_iterate_entry *entry = &result.entries[i];

            if (entry->cookie <= cookie) {
                server_free_iterate_data(&result);
                return -EPROTO;
            }

            unsigned int dtype = entry->type == VTFS_SERVER_FILE_TYPE ? DT_REG : DT_DIR;

            if (!dir_emit(ctx, entry->name, entry->name_length, entry->inode_id, dtype)) {
                server_free_iterate_data(&result);
                return 0;
            }

            ctx->pos = (loff_t)entry->cookie + 2;
        }

        bool eof = (bool)result.end_of_file;

        server_free_iterate_data(&result);

        if (eof) {
            return 0;
        }
    }
}

int fs_link(struct dentry *old_dentry, struct inode *directory, struct dentry *new_dentry) {
    if (!old_dentry || !directory || !new_dentry) {
        return -EINVAL;
    }

    if (!S_ISDIR(directory->i_mode)) {
        return -EPERM;
    }

    struct inode *inode = d_inode(old_dentry);

    if (!inode) {
        return -EINVAL;
    }

    vtfs_info *info = inode->i_sb->s_fs_info;

    if (!info || !info->server_info) {
        return -EINVAL;
    }

    char *name = kmemdup_nul(new_dentry->d_name.name, new_dentry->d_name.len, GFP_KERNEL);

    if (!name) {
        return -ENOMEM;
    }

    int code = server_link(info->server_info, (u64)directory->i_ino, (u64)inode->i_ino, name);

    kfree(name);

    return code;
}

ssize_t fs_write(struct file *filp, const char __user *buffer, size_t len, loff_t *offset) {
    if (!filp || !buffer || !offset) {
        return -EINVAL;
    }

    if (*offset < 0) {
        return -EINVAL;
    }

    if (len == 0) {
        return 0;
    }

    struct inode *inode = file_inode(filp);

    if (!inode) {
        return -EINVAL;
    }

    if (S_ISDIR(inode->i_mode)) {
        return -EPERM;
    }

    vtfs_info *info = inode->i_sb->s_fs_info;

    if (!info || !info->server_info) {
        return -EINVAL;
    }

    char *send_buffer = kcalloc(MAX_BODY_SIZE, sizeof(char), GFP_KERNEL);

    if (!send_buffer) {
        return -ENOMEM;
    }

    size_t sent_data_count = 0;

    while (sent_data_count < len) {
        size_t toCopy =
            len - sent_data_count > MAX_BODY_SIZE ? MAX_BODY_SIZE : len - sent_data_count;

        if (copy_from_user(send_buffer, buffer + sent_data_count, toCopy)) {
            kfree(send_buffer);
            return sent_data_count > 0 ? (ssize_t)sent_data_count : -EFAULT;
        }

        int code = server_write(info->server_info, (u64)inode->i_ino, send_buffer, *offset, toCopy);

        if (code < 0) {
            kfree(send_buffer);
            return sent_data_count > 0 ? (ssize_t)sent_data_count : code;
        }

        *offset += (loff_t)toCopy;
        sent_data_count += toCopy;

        if (*offset > i_size_read(inode)) {
            i_size_write(inode, *offset);
        }
    }

    kfree(send_buffer);

    return (ssize_t)sent_data_count;
}
