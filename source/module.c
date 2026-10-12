#include "file_system/directory.h"
#include "file_system/file.h"
#include "file_system/node.h"
#include "linux/err.h"
#include "linux/kern_levels.h"
#include "linux/slab.h"
#include "linux/stat.h"
#include "linux/types.h"
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

static const struct file_operations vtfs_file_fops = {
    .owner = THIS_MODULE,
    .llseek = generic_file_llseek,
};

static const struct inode_operations vtfs_dir_operations = {
    .lookup = vtfs_lookup,
    .create = vtfs_create,
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
    printk(KERN_INFO "VTFS starting unmounting...");
    kill_anon_super(sb);

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

    vtfs_init_inode(inode, dir, mode, node, &nop_mnt_idmap);
    return inode;
}

int vtfs_init_inode(struct inode *inode, const struct inode *dir, umode_t mode, vtfs_node *node,
                    struct mnt_idmap *idmap) {
    if (!node) {
        ERR("vtfs_init_inode got null pointer to node");
        return -EINVAL;
    }

    if (!inode) {
        ERR("vtfs_init_inode got null pointer to inode");
        return -EINVAL;
    }

    inode_init_owner(idmap, inode, dir, mode | 0777);

    node_inc_refcount(node);

    inode->i_private = node;
    inode->i_ino = node->id;

    // TODO: привзяать функции evict и i_op и f_op
    if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &vtfs_dir_operations;
        inode->i_fop = &vtfs_file_fops;
    } else {
        inode->i_fop = &vtfs_file_fops;
    }
    return 0;
}

struct dentry *vtfs_lookup(struct inode *inode, struct dentry *dentry, unsigned int flags) {
    vtfs_node *dir = inode->i_private;

    vtfs_node *finded_node = directory_find(dir, dentry->d_name.name, dentry->d_name.len);

    if (!finded_node) {
        d_add(dentry, NULL);
        return NULL;
    }

    umode_t mode = finded_node->type == VTFS_FILE ? S_IFREG : S_IFDIR;

    struct inode *finded_inode = vtfs_get_inode(inode->i_sb, inode, mode, finded_node);

    node_dec_refcount(finded_node);

    if (!finded_inode) {
        return ERR_PTR(-ENOMEM);
    }

    d_add(dentry, finded_inode);
    return NULL;
}

int vtfs_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode,
                bool b) {
    const struct qstr *qname = &dentry->d_name;

    char *name = kmemdup_nul(qname->name, qname->len, GFP_KERNEL);

    if (!name) {
        return -ENOMEM;
    }

    struct inode *inode = new_inode(dir->i_sb);

    if (!inode) {
        kfree(name);
        ERR("vtfs_create couldn't create new inode");
        return -ENOMEM;
    }

    vtfs_node *dir_node = dir->i_private;

    if (!dir_node) {
        ERR("vtfs_create got null pointer to vtfs_node in dir");
        iput(inode);
        kfree(name);
        return -EINVAL;
    }

    vtfs_node *file_node = NULL;
    int code = fs_create_file(dir->i_sb, dir_node, &file_node, name);

    if (code < 0) {
        iput(inode);
        kfree(name);

        return code;
    }

    vtfs_init_inode(inode, dir, mode, file_node, idmap);

    pr_info("VTFS: inode=%px mode=%o ino=%lu\n", inode, inode->i_mode, inode->i_ino);

    pr_info("VTFS: dentry=%px parent=%px root=%px\n", dentry, dentry->d_parent, dir->i_sb->s_root);

    WARN_ON(d_really_is_positive(dentry));
    WARN_ON(S_ISDIR(inode->i_mode));
    d_add(dentry, inode);

    pr_info("VTFS: after instantiate: inode=%px d_inode=%px hashed=%d\n", inode, d_inode(dentry),
            !d_unhashed(dentry));

    kfree(name);

    node_dec_refcount(file_node);

    LOG("vtfs_create created new file");
    return 0;
}

static void vtfs_evict_inode(struct inode *inode) {
    LOG("vtfs_evict_inode called: removing link to node");
    vtfs_node *node = inode->i_private;

    truncate_inode_pages_final(&inode->i_data);
    clear_inode(inode);

    inode->i_private = NULL;

    if (node) {
        node_dec_refcount(node);
    }
}

static void vtfs_put_super(struct super_block *sb) {
    LOG("Starting of put_super: clearing vtfs nodes");
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
