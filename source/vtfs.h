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
#endif
