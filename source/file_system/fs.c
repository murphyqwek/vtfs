#include "fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "../helpers.h"
#include "directory.h"
#include "linux/stddef.h"
#include "node.h"

static int fs_init_new_node(struct super_block *sb, vtfs_node_type type, vtfs_node **node);
static int fs_init_new_node_with_id(unsigned long id, vtfs_node_type type, vtfs_node **node);

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

    atomic_long_set(&info->next_inode_no, 2);

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

    vtfs_info *info = block->s_fs_info;

    vtfs_node *root_dir = info->root_dir;

    if (root_dir) {
        node_dec_refcount(root_dir);
        info->root_dir = NULL;
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
    unsigned long new_no = atomic_long_fetch_inc(&info->next_inode_no);

    return new_no;
}

/**
 * fs_init_directory - inits a new directroy
 * @sb: pointer to super_block
 * @node: pointer to vtfs_node of a new directroy
 *
 *
 * Return: error code or 0 if all good
 */
int fs_init_directory(struct super_block *sb, vtfs_node **node) {
    return fs_init_new_node(sb, VTFS_FOLDER, node);
}

/**
 * fs_init_file - inits a new file
 * @sb: pointer to super_block
 * @node: pointer to a new file node
 *
 * Return: error code or 0 if all good
 */
int fs_init_file(struct super_block *sb, vtfs_node **node) {
    return fs_init_new_node(sb, VTFS_FILE, node);
}

static int fs_init_new_node(struct super_block *sb, vtfs_node_type type, vtfs_node **node) {
    if (!sb) {
        ERR("filesystem got null pointer to super_block");
        return -EINVAL;
    }

    unsigned long id = fs_get_next_inode_no(sb);

    return fs_init_new_node_with_id(id, type, node);
}

static int fs_init_new_node_with_id(unsigned long id, vtfs_node_type type, vtfs_node **node) {
    if (id == 0) {
        ERR("Couldn't initialized new vtfs_node because couldn't get new inode no");
        return -EINVAL;
    }

    int result = node_init(node, type, id);

    if (result < 0) {
        ERR("Couldn't initialized new node");
    }

    return result;
}

/**
 * fs_init_root_directory - init root directory of filesystem
 *
 *
 * Return: pointer to vtfs_node of root directory
 */
vtfs_node *fs_init_root_directory(void) {
    vtfs_node *root_node = NULL;

    int result = fs_init_new_node_with_id(1, VTFS_FOLDER, &root_node);

    if (result < 0) {
        return NULL;
    }

    return root_node;
}
