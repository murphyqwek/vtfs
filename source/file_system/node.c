#include "node.h"
#include "asm-generic/errno-base.h"
#include "file.h"
#include "directory.h"
#include <linux/stddef.h>
#include <linux/refcount.h>
#include <linux/slab.h>

#include "../helpers.h"
#include "fs.h"
#include "linux/gfp_types.h"
#include "linux/mutex.h"
#include "linux/refcount_types.h"

static void node_free(vtfs_node *node);

static void node_init_directory(vtfs_node *node);
static void node_init_file(vtfs_node *node);

/**
 * node_inc_refcount - increment refcount of a node
 * @node: pointer to vtfs_node
 *
 * Return: Nothing
 */
void node_inc_refcount(vtfs_node *node) {
    if (!node) {
        return;
    }

    refcount_inc(&node->ref_count);
}

/**
 * node_dec_refcount - decriment node refcount and free the node if refcount is 0
 * @node: pointer to vtfs_node
 *
 *
 * Return: Nothing
 */
void node_dec_refcount(vtfs_node *node) {
    if (!node) {
        return;
    }

    if (refcount_dec_and_test(&node->ref_count)) {
        node_free(node);
    }
}

/**
 * node_free - free node and resource it points
 * @node: pointer to node
 *
 * Call it only when refcount is 0. Otherwise someone will be pointing to free memory
 *
 * Return: Nothing
 */
static void node_free(vtfs_node *node) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case VTFS_FILE:
            file_free(&node->file);
            break;

        case VTFS_FOLDER:
            directory_free(&node->directory);
            break;
    }

    kfree(node);
    LOG("node was free");
}

/**
 * node_init - initialize new vtfs_node
 * @node: pointer to node pointer
 * @type: node type: file or directory
 * @id: inode_no
 *
 *
 * Return: error code or 0 if success
 */
int node_init(vtfs_node **node, vtfs_node_type type, unsigned long id) {
    if (!node) {
        ERR("node_init got null pointer for node");
        return -EINVAL;
    }

    vtfs_node *result;

    result = NULL;

    result = kzalloc(sizeof(vtfs_node), GFP_KERNEL);

    if (result) {
        ERR("node_init couldn't allocate memory for vtfs_node");
        return -ENOMEM;
    }

    result->type = type;
    result->id = id;

    refcount_set(&result->ref_count, 1);
    mutex_init(&result->mutex);

    switch (type) {
        case VTFS_FILE:
            result->file = file_init();
            break;

        case VTFS_FOLDER:
            result->directory = directory_init();
            break;
    }

    *node = result;
    return 0;
}
