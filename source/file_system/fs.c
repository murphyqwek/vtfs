#include "fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "../helpers.h"

/**
 * fs_init_info - inits a new filesystem information
 * @block: pointer to superblock
 *
 * IMPORTANT: using only once at filesystem initialization
 *
 *
 * Return: 0 if all good, else error code
 */
int fs_init_info(struct super_block *block) {
    if (!block) {
        ERR("fs_init_info get null pointer to super_block");
        return -EINVAL;
    }

    vtfs_info *info = kzalloc(sizeof(vtfs_info), GFP_KERNEL);

    if (!info) {
        ERR("Couldn't allocate memory for filesystem info");
        return -ENOMEM;
    }

    atomic64_set(&info->next_inode_no, 1);

    block->s_fs_info = info;

    LOG("successfully initialized new filesystem info");

    return 0;
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

    kfree(block->s_fs_info);
    block->s_fs_info = NULL;

    LOG("filesystem info was free");
}

/**
 * fs_get_next_inode_no - get new unique inode number
 * @block: pointer to superblock
 *
 *
 * Return: new unique number or zero in error case (because root folder has 0 inode_no)
 */
unsigned long fs_get_next_inode_no(struct super_block *super_block) {
    if (!super_block) {
        ERR("Couldn't return next inode no because super block was null");
        return 0;
    }

    vtfs_info *info = super_block->s_fs_info;

    if (!info) {
        ERR("Couldn't return next inode no because filesystem information was initiated");
        return 0;
    }

    LOG("get new inode_no");
    unsigned long new_no = atomic64_fetch_inc(&info->next_inode_no);

    return new_no;
}
