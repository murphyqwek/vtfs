#include "vtfs.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "linux/dcache.h"
#include "linux/mnt_idmapping.h"
#include "linux/sched.h"

#include "helpers.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arseny Starikov P3313");
MODULE_DESCRIPTION("A simple FS kernel module");

struct file_system_type my_vtfs_type = {
    .name = "vtfs", .mount = vtfs_mount, .kill_sb = vtfs_kill_sb};

struct dentry *vtfs_mount(struct file_system_type *fs_type, int flags, const char *token,
                          void *data) {
    struct dentry *ret = mount_nodev(fs_type, flags, data, vtfs_fill_super);
    if (ret == NULL) {
        printk(KERN_ERR "Can't mount file system");
    } else {
        printk(KERN_INFO "Mounted successfuly");
    }

    return ret;
}

int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *inode = vtfs_get_inode(sb, NULL, S_IFDIR, 1000);

    sb->s_root = d_make_root(inode);
    if (sb->s_root == NULL) {
        return -ENOMEM;
    }

    printk(KERN_INFO "Created super block\n");
    return 0;
}

void vtfs_kill_sb(struct super_block *) {
    printk(KERN_INFO "VTFS was unmounted");
}

struct inode *vtfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode,
                             int i_ino) {
    struct inode *inode = new_inode(sb);
    if (inode != NULL) {
        inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
    }

    inode->i_ino = i_ino;
    return inode;
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
