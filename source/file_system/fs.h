#ifndef VTFS_FILE_SYSTEM_H
#define VTFS_FILE_SYSTEM_H

#include <linux/fs.h>
#include "linux/dcache.h"
#include <linux/dirent.h>
#include "../server/server_info.h"

typedef struct vtfs_info {
    vtfs_server_info *server_info;
} vtfs_info;

int fs_mount(vtfs_server_info *server, vtfs_server_inode *inode);

int fs_init_info(struct super_block *block, const char *server_ip, u16 server_port,
                 const char *login);

void fs_free_info(struct super_block *block);

int fs_lookup(struct super_block *block, struct inode *parent, struct dentry *dentry,
              vtfs_server_inode *server_inode);

int fs_create(struct super_block *block, struct inode *parent, struct dentry *dentry,
              vtfs_server_inode *server_inode);

int fs_mkdir(struct super_block *block, struct inode *parent, struct dentry *dentry,
             vtfs_server_inode *server_inode);

int fs_unlink(struct inode *parent, struct dentry *dentry);

int fs_rmdir(struct inode *parent, struct dentry *dentry);

int fs_iterate(struct inode *directory, struct dir_context *ctx);
#endif
