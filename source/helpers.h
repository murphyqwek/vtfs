#ifndef VTFS_HELPERS_H
#define VTFS_HELPERS_H

#include <linux/printk.h>

#define MODULE_NAME "vtfs"

#define LOG(fmt, ...) pr_info("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) pr_err("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) pr_warn("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)

#endif
