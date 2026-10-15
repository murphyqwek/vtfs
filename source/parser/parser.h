#ifndef VTFS_SERVER_PARSER_H
#define VTFS_SERVER_PARSER_H

#include <linux/stddef.h>
#include "../server/server.h"

#define VTFS_INODE_SERVER_DATA_SIZE 49

int parse_to_inode(const void *data, size_t len, vtfs_server_inode *result);

int parse_to_error_code(const void *data, size_t len, int *error_code);

#endif  // !VTFS_SERVER_PARSER_H
