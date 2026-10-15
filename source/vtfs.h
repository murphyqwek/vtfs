#ifndef VTFS_H
#define VTFS_H

#include <linux/fs.h>
#include "file_system/directory.h"
#include "file_system/node.h"
#include "linux/dcache.h"
#include "linux/types.h"

struct inode *vtfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode,
                             vtfs_node *node);

int vtfs_init_inode(struct inode *inode, const struct inode *dir, umode_t mode, vtfs_node *node,
                    struct mnt_idmap *idmap);

struct inode *vtfs_get_inode_inode_no(struct super_block *sb, const struct inode *dir, umode_t mode,
                                      vtfs_node *node, unsigned long id);

struct dentry *vtfs_lookup(struct inode *inode, struct dentry *dentry, unsigned int flags);

int vtfs_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode,
                bool b);

int vtfs_iterate(struct file *, struct dir_context *);

int vtfs_unlink(struct inode *parent, struct dentry *child);

#endif
