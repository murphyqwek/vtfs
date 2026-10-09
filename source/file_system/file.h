#ifndef VTFS_FILE_H
#define VTFS_FILE_H

#include <linux/types.h>

#define MAX_FILE_SIZE \
    (4 * 1024)  // максимальный размер файла, для лабы много места выделять бессмысленно (ну и ещё
                // там не указано это))

#define STANDART_FILE_CAPACITY 256

typedef struct vtfs_file {
    size_t length;
    size_t capacity;

    char *data;
} vtfs_file;

vtfs_file file_init(void);
int file_init_capacity(vtfs_file *file, size_t init_capacity);

void file_free(vtfs_file *file);

#endif
