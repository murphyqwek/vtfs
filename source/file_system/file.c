#include <linux/slab.h>
#include "../helpers.h"
#include "asm-generic/errno-base.h"
#include "linux/gfp_types.h"
#include "file.h"

/**
 * file_init - init vtfs_file structure for a new file
 *
 * Note that you should init capacity using file_init_capacity
 *
 * Return: new vtfs_file structure;
 */
vtfs_file file_init(void) {
    vtfs_file file;

    file.capacity = 0;
    file.length = 0;
    file.data = NULL;

    return file;
}

/**
 * file_init_capacity - init data with init_capacity. WARNING: IT IS USED FOR NEW FILES!!!
 * @init_capacity: capacity for data
 * @file: recently created file
 *
 * Return: return code for memory alloc
 */
int file_init_capacity(vtfs_file *file, size_t init_capacity) {
    if (!file) {
        ERR("file_init_capacity got null pointer to file");
        return -EINVAL;
    }

    // Проверка на то, что файл не слишком большой
    if (init_capacity * sizeof(char) > MAX_FILE_SIZE) {
        WARN("Couldn't create new file because it's size was too big");
        return -EFBIG;
    }

    // Пытаемся выделить память
    char *data = kzalloc(init_capacity * sizeof(char), GFP_KERNEL);

    if (!data) {
        ERR("Couldn't allocate memory for a new file");
        return -ENOMEM;
    }

    file->data = data;
    file->capacity = init_capacity;

    return 0;
}

/**
 * file_free - free memory for data
 * @*file: file to free
 *
 *
 *
 * Return: void
 */
void file_free(vtfs_file *file) {
    if (!file) {
        return;
    }

    kfree(file->data);

    file->length = 0;
    file->capacity = 0;
}
