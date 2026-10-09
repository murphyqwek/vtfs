#include "directory.h"
#include "asm-generic/errno-base.h"
#include "linux/gfp_types.h"
#include "linux/slab.h"
#include "linux/stddef.h"
#include "../helpers.h"

/**
 * directory_init - creates empty struct of vtfs_dir
 *
 * After creating empty struct recommended to init capacity
 * with directory_init_capacity
 *
 * Return: empty vtfs_dir struct
 */
vtfs_dir directory_init(void) {
    vtfs_dir new_dir;

    new_dir.capacity = 0;
    new_dir.length = 0;
    new_dir.entries = NULL;

    return new_dir;
}

/**
 * directory_init_capacity - inits an array of dir_entries
 * @dir: directory
 * @capacity: capacity should be less than MAX_DIRECTORY_CAPACITY
 *
 * You should use this function only on empty vtfs_dir
 *
 * Return: 0 if all okay else error code
 */
int directory_init_capacity(vtfs_dir *dir, size_t init_capacity) {
    if (dir == NULL) {
        ERR("Couldn't init directory capacity because it was NULL");
        return -EINVAL;
    }

    if (init_capacity > MAX_DIRECTORY_CAPACITY) {
        WARN("Couldn't init directory capacity because capacity was too big");
        return -EINVAL;
    }

    vtfs_dir_entry *entries = kzalloc(init_capacity * sizeof(vtfs_dir_entry), GFP_KERNEL);

    if (!entries) {
        ERR("Couldn't allocate memory for directory");
        return -ENOMEM;
    }

    dir->entries = entries;
    dir->capacity = init_capacity;

    return 0;
}

// TODO: implement basic fs function and fs_node operation
/**
 * All we need is iterate throw directory and dec all files refcount
 * by ref count we can easily free node, if refcount equal to zero
 * when we dec all refcounts, we can just free the array peacfully (namaste)
 */
void directory_free(vtfs_dir_entry *dir);
