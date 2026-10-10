#ifndef VTFS_NODE_H
#define VTFS_NODE_H

#include <linux/refcount.h>
#include <linux/types.h>
#include <linux/mutex.h>

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

    struct mutex mutex;

    union {
        vtfs_dir directory;
        vtfs_file file;
    };
} vtfs_node;

void node_inc_refcount(vtfs_node *node);
void node_dec_refcount(vtfs_node *node);

int node_init(vtfs_node **node, vtfs_node_type type, unsigned long id);

#endif
