#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kstrtox.h>

#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/dcache.h"
#include "linux/stat.h"
#include "linux/stddef.h"
#include "linux/time64.h"
#include "server/server_info.h"
#include "file_system/fs.h"

#include "server/server_info.h"

#include "linux/namei.h"

#include "vtfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arseny Starikov P3313");
MODULE_DESCRIPTION("VTFS network filesystem");

struct vtfs_mount_options {
    char server[VTFS_MAX_SERVER_LEN];
    u16 port;
    char login[VTFS_MAX_LOGIN_LEN];
};

static void vtfs_put_super(struct super_block *sb);

static const struct super_operations vtfs_super_ops = {
    .put_super = vtfs_put_super,
};

static struct inode_operations vtfs_dir_inode_operations = {
    .lookup = vtfs_lookup,
    .mkdir = vtfs_mkdir,
    .create = vtfs_create,
    .unlink = vtfs_unlink,
    .rmdir = vtfs_rmdir,
};

static struct file_operations vtfs_dir_operations = {

};

static struct inode_operations vtfs_file_inode_operations = {

};

static struct file_operations vtfs_file_operations = {

};

static int vtfs_d_revalidate(struct dentry *dentry, unsigned int flags) {
    if (flags & LOOKUP_RCU) {
        return -ECHILD;
    }

    return 0;
}

static struct dentry_operations vtfs_dentry_opretations = {
    .d_revalidate = vtfs_d_revalidate,
};

static struct timespec64 vtfs_ms_to_timespec(u64 ms) {
    return (struct timespec64){
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * NSEC_PER_MSEC,
    };
}

static void vtfs_update_inode_locked(struct inode *inode, const vtfs_server_inode *server_inode) {
    if (!inode || !server_inode) {
        return;
    }

    i_size_write(inode, server_inode->size_bytes);

    set_nlink(inode, server_inode->nlink);

    inode_set_mtime_to_ts(inode, vtfs_ms_to_timespec(server_inode->modified_at));
    inode_set_ctime_to_ts(inode, vtfs_ms_to_timespec(server_inode->changhed_at));
}

static struct inode *vtfs_get_inode(struct super_block *block,
                                    const vtfs_server_inode *server_inode) {
    if (!block || !server_inode) {
        return ERR_PTR(-EINVAL);
    }

    if (server_inode->inode_id == 0) {
        return ERR_PTR(-EINVAL);
    }

    struct inode *inode = iget_locked(block, server_inode->inode_id);

    if (!inode) {
        return ERR_PTR(-ENOMEM);
    }

    if (inode->i_state & I_NEW) {
        inode->i_ino = server_inode->inode_id;

        if (server_inode->type == VTFS_SERVER_DIR_TYEPE) {
            inode->i_mode = S_IFDIR | 0777;
            inode->i_op = &vtfs_dir_inode_operations;
            inode->i_fop = &vtfs_dir_operations;
        } else {
            inode->i_mode = S_IFREG | 0777;
            inode->i_op = &vtfs_file_inode_operations;
            inode->i_fop = &vtfs_file_operations;
        }

        vtfs_update_inode_locked(inode, server_inode);
        unlock_new_inode(inode);
        return inode;
    }

    inode_lock(inode);
    vtfs_update_inode_locked(inode, server_inode);
    inode_unlock(inode);

    return inode;
}

static int vtfs_parse_options(void *data, struct vtfs_mount_options *opts) {
    char *options, *cursor, *token;
    bool has_server = false;
    bool has_port = false;
    bool has_login = false;
    int ret = 0;

    if (!data) {
        return -EINVAL;
    }

    memset(opts, 0, sizeof(*opts));

    options = kstrdup(data, GFP_KERNEL);

    if (!options) {
        return -ENOMEM;
    }

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

    root_inode = vtfs_get_inode(sb, &inode);

    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        fs_free_info(sb);
        return ret;
    }

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        fs_free_info(sb);
        return ret;
    }

    sb->s_d_op = &vtfs_dentry_opretations;
    sb->s_op = &vtfs_super_ops;
    pr_info("VTFS: filesystem mounted\n");
    return 0;
}

struct dentry *vtfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int falgs) {
    struct super_block *block = parent->i_sb;

    vtfs_server_inode server_inode = {0};

    int code = fs_lookup(block, parent, dentry, &server_inode);

    if (code == -ENOENT) {
        d_add(dentry, NULL);
        return NULL;
    }

    if (code < 0) {
        return ERR_PTR(code);
    }

    struct inode *inode = vtfs_get_inode(block, &server_inode);

    if (IS_ERR(inode)) {
        return ERR_CAST(inode);
    }

    return d_splice_alias(inode, dentry);
}

int vtfs_create(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode,
                bool excl) {
    vtfs_server_inode server_inode = {0};

    int code = fs_create(parent->i_sb, parent, dentry, &server_inode);

    if (code < 0) {
        return code;
    }

    if (server_inode.type != VTFS_SERVER_FILE_TYPE) {
        return -EPROTO;
    }

    struct inode *inode = vtfs_get_inode(parent->i_sb, &server_inode);
    if (IS_ERR(inode)) {
        return PTR_ERR(inode);
    }

    d_instantiate(dentry, inode);

    return 0;
}

int vtfs_mkdir(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode) {
    vtfs_server_inode server_inode = {0};

    int code = fs_mkdir(parent->i_sb, parent, dentry, &server_inode);

    if (code < 0) {
        return code;
    }

    if (server_inode.type != VTFS_SERVER_DIR_TYEPE) {
        return -EPROTO;
    }

    struct inode *inode = vtfs_get_inode(parent->i_sb, &server_inode);
    if (IS_ERR(inode)) {
        return PTR_ERR(inode);
    }

    d_instantiate(dentry, inode);

    return 0;
}

int vtfs_unlink(struct inode *parent, struct dentry *dentry) {
    struct inode *inode = d_inode(dentry);

    if (!inode) {
        return -ENOENT;
    }

    if (S_ISDIR(inode->i_mode)) {
        return -EISDIR;
    }

    int code = fs_unlink(parent, dentry);

    if (code < 0) {
        return code;
    }

    if (inode->i_nlink > 0) {
        drop_nlink(inode);
    }

    return 0;
}

int vtfs_rmdir(struct inode *parent, struct dentry *dentry) {
    struct inode *inode = d_inode(dentry);

    if (!inode) {
        return -ENOENT;
    }

    if (S_ISREG(inode->i_mode)) {
        return -ENOTDIR;
    }

    int code = fs_rmdir(parent, dentry);

    if (code < 0) {
        return code;
    }

    clear_nlink(inode);

    return 0;
}

int vtfs_rmdir(struct inode *parent, struct dentry *dentry);

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
