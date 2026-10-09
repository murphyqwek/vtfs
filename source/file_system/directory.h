#ifndef VTFS_DIRECTORY_H
#define VTFS_DIRECTORY_H

typedef struct vtfs_node vtfs_node;

typedef struct vtfs_dir_entry {
    const char *name;
    vtfs_node *node;

} vtfs_dir_entry;

typedef struct vtfs_dir {
    int length;
    int capacity;

    vtfs_dir_entry *entries;
} vtfs_dir;

#endif
