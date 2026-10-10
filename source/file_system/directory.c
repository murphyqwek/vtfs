#include "directory.h"
#include "node.h"

#include <linux/gfp.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/stddef.h>
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

    LOG("new directory was initialized");
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
        WRN("Couldn't init directory capacity because capacity was too big");
        return -EINVAL;
    }

    if (dir->entries || dir->capacity || dir->length) {
        ERR("directory_init_capacity should be used only for recently initialized direcotries");
        return -EINVAL;
    }

    vtfs_dir_entry *entries = kzalloc(init_capacity * sizeof(vtfs_dir_entry), GFP_KERNEL);

    if (!entries) {
        ERR("Couldn't allocate memory for directory");
        return -ENOMEM;
    }

    dir->entries = entries;
    dir->capacity = init_capacity;

    LOG("directory capacity was initialized");
    return 0;
}

/**
 * directory_free - free directory and it's children recursivly
 * @dir: pointer to vtfs_dir
 *
 * Should be called only by node_dec_refcount
 *
 * Return: Nothing
 */
void directory_free(vtfs_dir *dir) {
    if (!dir) {
        return;
    }

    // Сначала мы отпускаем ссылки на все дочерние файлы
    for (size_t i = 0; i < dir->length; i++) {
        // Освобождаем имя файла и отпускаем ссылку на файл
        kfree(dir->entries[i].name);
        node_dec_refcount(dir->entries[i].node);
    }

    // Освобождаем entries
    kfree(dir->entries);

    dir->entries = NULL;
    dir->capacity = 0;
    dir->length = 0;

    LOG("directory was free");
}
