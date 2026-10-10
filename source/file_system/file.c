#include <linux/slab.h>
#include "../helpers.h"
#include "linux/stddef.h"
#include <linux/errno.h>
#include <linux/gfp.h>
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
    LOG("new file was initialized");
    return file;
}

/**
 * file_init_capacity - init data with init_capacity
 * @init_capacity: capacity for data
 * @file: recently created file
 *
 * Should be used only for recently initialized files
 *
 * Return: return code for memory alloc
 */
int file_init_capacity(vtfs_file *file, size_t init_capacity) {
    if (init_capacity == 0) {
        ERR("init_capacity for file must be greater than 0");
        return -EINVAL;
    }

    if (!file) {
        ERR("file_init_capacity got null pointer to file");
        return -EINVAL;
    }

    // Проверка на то, что файл не слишком большой
    if (init_capacity * sizeof(char) > MAX_FILE_SIZE) {
        WRN("Couldn't create new file because it's size was too big");
        return -EFBIG;
    }

    if (file->capacity || file->length || file->data) {
        ERR("file_init_capacity's argument file must be recently initialized file");
        return -EINVAL;
    }

    // Пытаемся выделить память
    char *data = kzalloc(init_capacity * sizeof(char), GFP_KERNEL);

    if (!data) {
        ERR("Couldn't allocate memory for a new file");
        return -ENOMEM;
    }

    file->data = data;
    file->capacity = init_capacity;
    LOG("new file capacity was initialized");
    return 0;
}

/**
 * file_free - free memory for data
 * @*file: file to free
 *
 * Should be called only node_dec_refcounter
 *
 * Return: void
 */
void file_free(vtfs_file *file) {
    if (!file) {
        return;
    }

    kfree(file->data);

    file->data = NULL;
    file->length = 0;
    file->capacity = 0;

    LOG("file was free");
}
