#ifndef VTFS_SERVER_INFO_H
#define VTFS_SERVER_INFO_H

#include <linux/types.h>

#define GET "GET"
#define POST "POST"
#define DELETE "DELETE"

#define VTFS_SERVER_FILE_TYPE 1
#define VTFS_SERVER_DIR_TYEPE 2

#define VTFS_MAX_SERVER_LEN 100
#define VTFS_MAX_LOGIN_LEN 30

#define VTFS_MAX_NAME_LEN 255
#define VTFS_MAX_ENCODED_NAME_LEN (VTFS_MAX_SERVER_LEN * 3)

#define VTFS_MAX_URL_SIZE 1000
typedef struct vtfs_server_inode {
    u64 inode_id;
    u8 type;
    u64 size_bytes;
    u64 nlink;
    s64 created_at;
    s64 modified_at;
    s64 changhed_at;
} vtfs_server_inode;

typedef struct vtfs_server_info {
    char server_ip[VTFS_MAX_SERVER_LEN];
    u16 server_port;
    char login[VTFS_MAX_LOGIN_LEN];
} vtfs_server_info;

typedef struct vtfs_iterate_entry {
    u64 cookie;
    u64 inode_id;
    u8 type;
    u16 name_length;
    char *name;
} vtfs_iterate_entry;

typedef struct vtfs_iterate_result {
    u64 next_cookie;
    u32 entry_count;
    u8 end_of_file;

    vtfs_iterate_entry *entries;
} vtfs_iterate_result;

#endif  // !VTFS_SERVER_INFO_H
