#include "fs.h"
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "../helpers.h"
#include "asm-generic/errno-base.h"
#include "directory.h"
#include "linux/fs_types.h"
#include "linux/mutex.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "node.h"

static int fs_init_new_node(struct super_block *sb, vtfs_node_type type, vtfs_node **node);
static int fs_init_new_node_with_id(unsigned long id, vtfs_node_type type, vtfs_node **node);

static void release_node_mutex_dec_refcount(vtfs_node *node) {
    if (!node) {
        return;
    }

    mutex_unlock(&node->mutex);
    node_dec_refcount(node);
}

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

/**
 * fs_create_file - create new file and add it to directory
 * @sb: pointer to super_block
 * @parent_dir: pointer to parent directory
 * @new_file: pointer to created new file (NULL on failure)
 * @name: pointer to file's name
 *
 *
 * Return: error code or 0
 */
int fs_create_file(struct super_block *sb, vtfs_node *parent_dir, vtfs_node **new_file,
                   const char *name) {
    if (!sb) {
        ERR("fs_create_file got null pointer to super_block");
        return -EINVAL;
    }

    if (!parent_dir) {
        ERR("fs_create_file got null pointer to parent_dir");
        return -EINVAL;
    }

    if (!name) {
        ERR("fs_create_file got null pointer to name");
        return -EINVAL;
    }

    int code = fs_init_file(sb, new_file);

    if (code < 0) {
        ERR("fs_create_file couldn't create new file");
        return code;
    }

    code = directory_add(parent_dir, *new_file, name);

    if (code < 0) {
        node_dec_refcount(*new_file);
        *new_file = NULL;

        ERR("fs_create_file couldn't add new file to directory");
        return code;
    }

    LOG("fs_create_file created new file successfully");
    return 0;
}

/**
 * fs_iterate - terate function for ls
 * @fil: pointer to struct file
 * @ctx: pointer to dir_context
 *
 *
 * Return: 0 or error code
 */
int fs_iterate(struct file *file, struct dir_context *ctx) {
    struct inode *inode = file_inode(file);

    vtfs_node *node = inode->i_private;

    if (!node) {
        ERR("fs_iterate got null pointer to vtfs_node");
        return -EIO;
    }

    if (!node_is_dir(node)) {
        ERR("fs_iterate got file as directory");
        return -ENOTDIR;
    }

    vtfs_dir *directory = &node->directory;

    if (!dir_emit_dots(file, ctx)) {
        return 0;
    }

    mutex_lock(&node->mutex);
    node_inc_refcount(node);

    int code = 0;

    for (loff_t i = ctx->pos - 2; i < (loff_t)directory->length; i++) {
        vtfs_dir_entry *entry = &directory->entries[i];

        if (!entry->node) {
            ERR("during fs_iterate directory entry got null pointer to node");
            code = -EIO;
            break;
        }

        if (!entry->name) {
            ERR("during fs_iterate directory entry got null pointer to name");
            code = -EIO;
            break;
        }

        unsigned int type = node_is_file(node) ? DT_REG : DT_DIR;

        if (!dir_emit(ctx, entry->name, strlen(entry->name), entry->node->id, type)) {
            code = 0;
            break;
        }

        ctx->pos++;
    }

    mutex_unlock(&node->mutex);

    node_dec_refcount(node);

    return code;
}

/**
 * fs_unlink - unlink file from directory
 * @parent_dir: pointer to dir inode
 * @child: pointer to unlinking file dentry
 *
 *
 * Return: 0 or error code
 */
int fs_unlink(struct inode *parent_dir, struct dentry *child) {
    vtfs_node *node = parent_dir->i_private;

    if (!node) {
        ERR("fs_unlink got null pointer to directory node");
        return -EIO;
    }

    if (!node_is_dir(node)) {
        ERR("fs_unlink got file as directory");
        return -EIO;
    }

    vtfs_dir *dir = &node->directory;

    const struct qstr *qstr = &child->d_name;

    char *name = kmemdup_nul(qstr->name, qstr->len, GFP_KERNEL);

    if (!name) {
        ERR("fs_unlink couldn't allocate memory for name");
        return -ENOMEM;
    }

    mutex_lock(&node->mutex);
    node_inc_refcount(node);

    int id = directory_get_file_id_locked(dir, name);

    if (id < 0) {
        ERR("fs_unlink didn't find a unlinkng fiile in directory");

        kfree(name);
        release_node_mutex_dec_refcount(node);

        return -ENOENT;
    }

    int code = directory_unlink_file_locked(dir, id);

    if (code < 0) {
        ERR("fs_unlink got unexpected error when calling directory_unlink_file_locked");
    }

    kfree(name);

    mutex_unlock(&node->mutex);
    node_dec_refcount(node);

    return code;
}
