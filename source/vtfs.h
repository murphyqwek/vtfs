#ifndef VTFS_H
#define VTFS_H

#include <linux/fs.h>
#include "file_system/directory.h"
#include "file_system/node.h"

struct dentry *vtfs_mount(struct file_system_type *fs_type, int flags, const char *token,
                          void *data);

int vtfs_fill_super(struct super_block *sb, void *data, int silent);

void vtfs_kill_sb(struct super_block *);

struct inode *vtfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode,
                             vtfs_node *node);

struct inode *vtfs_get_inode_inode_no(struct super_block *sb, const struct inode *dir, umode_t mode,
                                      vtfs_node *node, unsigned long id);

#endif
