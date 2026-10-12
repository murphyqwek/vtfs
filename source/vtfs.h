#ifndef VTFS_H
#define VTFS_H

#include <linux/fs.h>
#include "file_system/directory.h"
#include "file_system/node.h"
#include "linux/types.h"

struct dentry *vtfs_mount(struct file_system_type *fs_type, int flags, const char *token,
                          void *data);

int vtfs_fill_super(struct super_block *sb, void *data, int silent);

void vtfs_kill_sb(struct super_block *);

struct inode *vtfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode,
                             vtfs_node *node);

int vtfs_init_inode(struct inode *inode, const struct inode *dir, umode_t mode, vtfs_node *node,
                    struct mnt_idmap *idmap);

struct inode *vtfs_get_inode_inode_no(struct super_block *sb, const struct inode *dir, umode_t mode,
                                      vtfs_node *node, unsigned long id);

struct dentry *vtfs_lookup(struct inode *inode, struct dentry *dentry, unsigned int flags);

int vtfs_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode,
                bool b);

#endif
