#include "directory.h"
#include "asm-generic/errno-base.h"
#include "linux/gfp_types.h"
#include "linux/lockdep.h"
#include "node.h"

#include <linux/gfp.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/mutex.h>
#include <linux/string.h>
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
 * directory_grow - increase directory capacity if it's possible
 * @dir: pointer to directory
 *
 *
 * Return: error code or 0
 */
int directory_grow(vtfs_dir *dir) {
    size_t new_capacity;

    if (dir->capacity >= MAX_DIRECTORY_CAPACITY) {
        ERR("Couldn't grow directory capacity because it's reached it's max capacity");
        return -ENOSPC;
    }

    if (dir->capacity == 0) {
        new_capacity = 4;
    } else if (dir->capacity > MAX_DIRECTORY_CAPACITY / 2) {
        new_capacity = MAX_DIRECTORY_CAPACITY;
    } else {
        new_capacity = dir->capacity * 2;
    }

    if (new_capacity > MAX_DIRECTORY_CAPACITY) {
        new_capacity = MAX_DIRECTORY_CAPACITY;
    }

    vtfs_dir_entry *new_entries =
        krealloc_array(dir->entries, new_capacity, sizeof(*dir->entries), GFP_KERNEL);

    if (!new_entries) {
        ERR("Couldn't grow directory capacity because Couldn't allocate new memory");
        return -ENOMEM;
    }

    dir->entries = new_entries;
    dir->capacity = new_capacity;

    LOG("directory capacity was successfully increased");
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
        kfree_const(dir->entries[i].name);
        node_dec_refcount(dir->entries[i].node);
    }

    // Освобождаем entries
    kfree(dir->entries);

    dir->entries = NULL;
    dir->capacity = 0;
    dir->length = 0;

    LOG("directory was free");
}

/**
 * directory_find - find node with current file name
 * @node: pointer to directory node
 * @name: file name
 * @name_len: length of name
 *
 * This function locks node's mutex
 * Capturing the node link
 *
 * Return: pointer to node with the name or NULL (if error or does not exist);
 */
vtfs_node *directory_find(vtfs_node *node, const char *name, size_t name_len) {
    if (node == NULL) {
        return NULL;
    }

    if (name == NULL) {
        return NULL;
    }

    if (!node_is_dir(node)) {
        mutex_unlock(&node->mutex);
        return NULL;
    }

    mutex_lock(&node->mutex);

    vtfs_node *finded_node = directory_find_locked(node, name, name_len);

    mutex_unlock(&node->mutex);

    return finded_node;
}

/**
 * directory_find_locked - finds node with current file name in directory
 * @node: pointer to directory node
 * @name: file name
 * @name_len: length of name
 *
 * This function requiers that dir_node mutex is locked
 * Capturing the node link
 *
 * Return: pointer to node with the name or NULL (if error or does not exist);
 */
vtfs_node *directory_find_locked(vtfs_node *node, const char *name, size_t name_len) {
    if (node == NULL) {
        return NULL;
    }

    lockdep_assert(&node->mutex);

    if (!node_is_dir(node)) {
        return NULL;
    }

    if (name == NULL) {
        return NULL;
    }

    vtfs_dir *dir = &node->directory;

    if (dir->length == 0) {
        return NULL;
    }

    for (size_t i = 0; i < dir->length; i++) {
        vtfs_dir_entry *entry = &dir->entries[i];

        if (strlen(entry->name) != name_len) {
            continue;
        }

        if (memcmp(entry->name, name, name_len) == 0) {
            vtfs_node *finded_node = entry->node;

            node_inc_refcount(finded_node);

            return finded_node;
        }
    }

    return NULL;
}

/**
 * directory_add - add file to directory
 * @parent: pointer to parent node
 * @children: pointer to children node
 * @name: pointer to name
 *
 * Return: error code or 0
 */
int directory_add(vtfs_node *parent, vtfs_node *children, const char *name) {
    if (!parent) {
        ERR("directory_add got null pointer to directory");
        return -EINVAL;
    }

    if (!node_is_dir(parent)) {
        ERR("directory_add got file as directory");
        return -EINVAL;
    }

    if (!children) {
        ERR("directory_add got null pointer to children");
        return -EINVAL;
    }

    if (!name) {
        ERR("directory_add got null pointer to name");
        return -EINVAL;
    }

    if (strlen(name) == 0) {
        ERR("directory_add got zero len name");
        return -EINVAL;
    }

    mutex_lock(&parent->mutex);

    int code = directory_add_locked(parent, children, name);

    mutex_unlock(&parent->mutex);

    return code;
}

/**
 * directory_add_locked - add file to directory
 * @parent: pointer to parent node
 * @children: pointer to children node
 * @name: pointer to name
 *
 * Expect that parent mutex was captured
 *
 * Return: error code or 0
 */
int directory_add_locked(vtfs_node *parent, vtfs_node *children, const char *name) {
    if (!parent) {
        ERR("directory_add_locked got null pointer to directory");
        return -EINVAL;
    }

    lockdep_assert(&parent->mutex);

    if (!node_is_dir(parent)) {
        ERR("directory_add_locked got file as directory");
        return -EINVAL;
    }

    if (!children) {
        ERR("directory_add_locked got null pointer to children");
        return -EINVAL;
    }

    // TODO: Добавить проверкку на /, . и ..
    if (!name) {
        ERR("directory_add_locked got null pointer to name");
        return -EINVAL;
    }

    if (strlen(name) == 0) {
        ERR("directory_add_locked got zero len name");
        return -EINVAL;
    }

    vtfs_dir *dir = &parent->directory;

    // Проверяем, что файла в директории с таким именем нет
    vtfs_node *existing = directory_find_locked(parent, name, strlen(name));

    if (existing) {
        node_dec_refcount(existing);
        return -EEXIST;
    }

    // Копируем имя
    const char *dup_name = kstrdup_const(name, GFP_KERNEL);

    if (!dup_name) {
        ERR("Couldn't allocate memory for dup_name in directory_add_locked");
        return -ENOMEM;
    }

    // Проверяем, что мы можем ещё добавить один файл в директорию
    if (dir->length >= MAX_DIRECTORY_CAPACITY) {
        ERR("Couldn't add file to directory because it has max capacity");
        return -ENOSPC;
    }

    if (dir->length + 1 > dir->capacity) {
        int code = directory_grow(dir);
        if (code < 0) {
            return code;
        }
    }

    vtfs_dir_entry new_entry;

    new_entry.name = dup_name;
    new_entry.node = children;

    node_inc_refcount(children);

    dir->entries[dir->length] = new_entry;
    dir->length++;

    return 0;
}
