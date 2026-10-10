#include "file_system/directory.h"
#include "file_system/node.h"
#include "linux/err.h"
#include "vtfs.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/mm.h>

#include "linux/dcache.h"
#include "linux/mnt_idmapping.h"
#include "linux/sched.h"

#include "helpers.h"

#include "file_system/fs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arseny Starikov P3313");
MODULE_DESCRIPTION("A simple FS kernel module");

static void vtfs_put_super(struct super_block *sb);
static void vtfs_evict_inode(struct inode *inode);

struct file_system_type my_vtfs_type = {
    .name = "vtfs", .mount = vtfs_mount, .kill_sb = vtfs_kill_sb};

static const struct super_operations vtfs_super_ops = {
    .put_super = vtfs_put_super,
    .evict_inode = vtfs_evict_inode,
};

struct dentry *vtfs_mount(struct file_system_type *fs_type, int flags, const char *token,
                          void *data) {
    struct dentry *ret = mount_nodev(fs_type, flags, data, vtfs_fill_super);
    if (IS_ERR(ret)) {
        printk(KERN_ERR "Can't mount file system");
    } else {
        printk(KERN_INFO "Mounted successfuly");
    }

    return ret;
}

int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
    sb->s_op = &vtfs_super_ops;

    int err = fs_init_info(sb);
    if (err) {
        return err;
    }

    vtfs_node *root_node = fs_init_root_directory();
    if (!root_node) {
        err = -ENOMEM;
        fs_free_info(sb);
        return err;
    }

    struct inode *inode = vtfs_get_inode(sb, NULL, S_IFDIR, root_node);
    if (!inode) {
        err = -ENOMEM;
        node_dec_refcount(root_node);
        fs_free_info(sb);
        return err;
    }

    set_nlink(inode, 2);

    sb->s_root = d_make_root(inode);
    if (!sb->s_root) {
        err = -ENOMEM;
        node_dec_refcount(root_node);
        fs_free_info(sb);
        return err;
    }

    vtfs_info *info = sb->s_fs_info;
    info->root_dir = root_node;

    return 0;
}

void vtfs_kill_sb(struct super_block *sb) {
    kill_litter_super(sb);

    printk(KERN_INFO "VTFS was unmounted");
}

struct inode *vtfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode,
                             vtfs_node *node) {
    if (!node) {
        return NULL;
    }

    struct inode *inode = new_inode(sb);

    if (!inode) {
        return NULL;
    }

    // Систему прав делть не будем, поэтому всегда будем ставить 777
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode | 0777);

    node_inc_refcount(node);
    inode->i_private = node;
    inode->i_ino = node->id;

    // TODO: привзяать функции evict и i_op и f_op
    return inode;
}

static void vtfs_evict_inode(struct inode *inode) {
    vtfs_node *node = inode->i_private;

    truncate_inode_pages_final(&inode->i_data);
    clear_inode(inode);

    inode->i_private = NULL;

    if (node) {
        node_dec_refcount(node);
    }
}

static void vtfs_put_super(struct super_block *sb) {
    fs_free_info(sb);
}

static int __init vtfs_init(void) {
    LOG("VTFS joined the kernel yo\n");
    return register_filesystem(&my_vtfs_type);
}

static void __exit vtfs_exit(void) {
    LOG("VTFS left the kernel out of this house\n");
    unregister_filesystem(&my_vtfs_type);
}

module_init(vtfs_init);
module_exit(vtfs_exit);
