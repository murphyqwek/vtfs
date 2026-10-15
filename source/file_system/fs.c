#include "fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include "../helpers.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/gfp_types.h"
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
