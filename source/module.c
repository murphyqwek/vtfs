#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kstrtox.h>

#include "server/server.h"
#include "server/server_info.h"
#include "vtfs.h"
#include "file_system/fs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arseny Starikov P3313");
MODULE_DESCRIPTION("VTFS network filesystem");

#define VTFS_MAX_SERVER_LEN 64
#define VTFS_MAX_LOGIN_LEN 64

struct vtfs_mount_options {
    char server[VTFS_MAX_SERVER_LEN];
    u16 port;
    char login[VTFS_MAX_LOGIN_LEN];
};

static void vtfs_put_super(struct super_block *sb);

static const struct super_operations vtfs_super_ops = {
    .put_super = vtfs_put_super,
};

static int vtfs_parse_options(void *data, struct vtfs_mount_options *opts) {
    char *options, *cursor, *token;
    bool has_server = false;
    bool has_port = false;
    bool has_login = false;
    int ret = 0;

    if (!data)
        return -EINVAL;

    memset(opts, 0, sizeof(*opts));

    options = kstrdup(data, GFP_KERNEL);
    if (!options)
        return -ENOMEM;

    cursor = options;

    while ((token = strsep(&cursor, ",")) != NULL) {
        if (!strncmp(token, "server=", 7)) {
            if (has_server || !token[7] ||
                strscpy(opts->server, token + 7, sizeof(opts->server)) < 0) {
                ret = -EINVAL;
                kfree(options);
                return ret;
            }
            has_server = true;

        } else if (!strncmp(token, "port=", 5)) {
            if (has_port || kstrtou16(token + 5, 10, &opts->port) || opts->port == 0) {
                ret = -EINVAL;
                kfree(options);
                return ret;
            }
            has_port = true;

        } else if (!strncmp(token, "login=", 6)) {
            if (has_login || !token[6] ||
                strscpy(opts->login, token + 6, sizeof(opts->login)) < 0) {
                ret = -EINVAL;
                kfree(options);
                return ret;
            }
            has_login = true;

        } else {
            ret = -EINVAL;
            kfree(options);
            return ret;
        }
    }

    if (!has_server || !has_port || !has_login) {
        ret = -EINVAL;
    }

    kfree(options);
    return ret;
}

static int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
    struct vtfs_mount_options opts;
    struct inode *root_inode;

    int ret = vtfs_parse_options(data, &opts);
    if (ret < 0) {
        return ret;
    }
    pr_info("VTFS: server=%s port=%u login=%s\n", opts.server, opts.port, opts.login);

    sb->s_op = &vtfs_super_ops;

    ret = fs_init_info(sb, opts.server, opts.port, opts.login);
    if (ret < 0) {
        return ret;
    }

    vtfs_info *info = sb->s_fs_info;

    vtfs_server_inode inode;

    ret = fs_mount(info->server_info, &inode);

    if (ret < 0) {
        fs_free_info(sb);
        return ret;
    }

    root_inode = new_inode(sb);
    if (!root_inode) {
        ret = -ENOMEM;
        fs_free_info(sb);
        return ret;
    }

    root_inode->i_ino = inode.inode_id;
    root_inode->i_mode = S_IFDIR | 0755;
    set_nlink(root_inode, inode.nlink);
    simple_inode_init_ts(root_inode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        fs_free_info(sb);
        return ret;
    }

    pr_info("VTFS: filesystem mounted\n");
    return 0;
}

static struct dentry *vtfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name,
                                 void *data) {
    return mount_nodev(fs_type, flags, data, vtfs_fill_super);
}

static void vtfs_kill_sb(struct super_block *sb) {
    kill_anon_super(sb);
    pr_info("VTFS: filesystem unmounted\n");
}

static void vtfs_put_super(struct super_block *sb) {
    fs_free_info(sb);
}

static struct file_system_type vtfs_type = {
    .owner = THIS_MODULE,
    .name = "vtfs",
    .mount = vtfs_mount,
    .kill_sb = vtfs_kill_sb,
};

static int __init vtfs_init(void) {
    pr_info("VTFS: module loaded\n");
    return register_filesystem(&vtfs_type);
}

static void __exit vtfs_exit(void) {
    unregister_filesystem(&vtfs_type);
    pr_info("VTFS: module unloaded\n");
}

module_init(vtfs_init);
module_exit(vtfs_exit);
