#ifndef VTFS_FILE_SYSTEM_H
#define VTFS_FILE_SYSTEM_H

#include <linux/fs.h>
#include "directory.h"
#include "linux/dcache.h"
#include "node.h"
#include <linux/dirent.h>
#include "../server/server_info.h"

typedef struct vtfs_info {
    vtfs_node *root_dir;
    atomic_long_t next_inode_no;

    vtfs_server_info *server_info;
} vtfs_info;

int fs_mount(vtfs_server_info *server, vtfs_server_inode *inode);

int fs_init_info(struct super_block *block, const char *server_ip, u16 server_port,
                 const char *login);
void fs_free_info(struct super_block *block);
#endif
