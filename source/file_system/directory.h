#ifndef VTFS_DIRECTORY_H
#define VTFS_DIRECTORY_H

#include <linux/types.h>

#define MAX_DIRECTORY_CAPACITY 25

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

int directory_init_capacity(vtfs_dir *dir, size_t init_capacity);

void directory_free(vtfs_dir *dir);

#endif
