#ifndef VTFS_FILE_H
#define VTFS_FILE_H

typedef struct vtfs_file {
    int length;
    int capacity;

    char *data;
} vtfs_file;

#endif
