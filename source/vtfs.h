#ifndef VTFS_H
#define VTFS_H

#include <linux/fs.h>
#include "linux/dcache.h"

struct dentry *vtfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int falgs);

#endif
