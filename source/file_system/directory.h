#ifndef VTFS_DIRECTORY_H
#define VTFS_DIRECTORY_H

#include <linux/types.h>

#define MAX_DIRECTORY_CAPACITY 32
#define STANDART_DIRECTORY_CAPACITY 32

typedef struct vtfs_node vtfs_node;

typedef struct vtfs_dir_entry {
    const char *name;
    vtfs_node *node;

} vtfs_dir_entry;

typedef struct vtfs_dir {
    size_t length;
    size_t capacity;

    vtfs_dir_entry *entries;
} vtfs_dir;

vtfs_dir directory_init(void);
void directory_free(vtfs_dir *dir);

int directory_grow(vtfs_dir *dir);

vtfs_node *directory_find(vtfs_node *node, const char *name, size_t name_len);
vtfs_node *directory_find_locked(vtfs_node *node, const char *name, size_t name_len);

int directory_add(vtfs_node *parent, vtfs_node *children, const char *name);
int directory_add_locked(vtfs_node *parent, vtfs_node *children, const char *name);

#endif
