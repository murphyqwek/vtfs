#ifndef VTFS_H
#define VTFS_H

#include <linux/fs.h>
#include "linux/capability.h"
#include "linux/dcache.h"
#include "linux/types.h"

struct dentry *vtfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int falgs);

int vtfs_create(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode,
                bool excl);

int vtfs_mkdir(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode);

int vtfs_unlink(struct inode *parent, struct dentry *dentry);

int vtfs_rmdir(struct inode *parent, struct dentry *dentry);

int vtfs_iterate(struct file *file, struct dir_context *ctx);

int vtfs_link(struct dentry *old_dentry, struct inode *directory, struct dentry *new_dentry);

ssize_t vtfs_write(struct file *filp, const char __user *buffer, size_t len, loff_t *offset);

ssize_t vtfs_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset);
#endif
