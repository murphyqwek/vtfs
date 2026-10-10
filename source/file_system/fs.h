#ifndef VTFS_FILE_SYSTEM_H
#define VTFS_FILE_SYSTEM_H

#include <linux/fs.h>
#include "directory.h"
#include "node.h"

typedef struct vtfs_info {
    vtfs_node *root_dir;
    atomic_long_t next_inode_no;
} vtfs_info;

int fs_init_info(struct super_block *block);
unsigned long fs_get_next_inode_no(struct super_block *super_block);
void fs_free_info(struct super_block *block);

int fs_init_directory(struct super_block *sb, vtfs_node **node);
int fs_init_file(struct super_block *sb, vtfs_node **node);

vtfs_node *fs_init_root_directory(void);

#endif
