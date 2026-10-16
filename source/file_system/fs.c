#include "fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include "../helpers.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/dcache.h"
#include "linux/device.h"
#include "linux/gfp_types.h"
#include "linux/stat.h"
#include "linux/stddef.h"
#include "../server/server.h"

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
