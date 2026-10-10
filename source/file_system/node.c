#include "node.h"
#include "file.h"
#include "directory.h"
#include <linux/stddef.h>
#include <linux/refcount.h>
#include <linux/slab.h>

#include "../helpers.h"

static void node_free(vtfs_node *node);

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
