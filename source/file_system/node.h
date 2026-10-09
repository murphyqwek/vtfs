#ifndef VTFS_NODE_H
#define VTFS_NODE_H

#include <linux/refcount.h>

#include "directory.h"
#include "file.h"

typedef enum {
    VTFS_FILE,
    VTFS_FOLDER,
} vtfs_node_type;

typedef struct vtfs_node {
    unsigned long id;

    vtfs_node_type type;
    refcount_t ref_count;

    union {
        vtfs_dir directory;
        vtfs_file file;
    };
} vtfs_node;

#endif
