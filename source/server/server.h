#ifndef VTFS_SERVER_H
#define VTFS_SERVER_H

#include "linux/types.h"
#include "server_info.h"

char *escape_json_name(const char *name, size_t len);

int server_mount(vtfs_server_info *server, vtfs_server_inode *inode);

int server_lookup(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode);

int server_create(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode);

int server_mkdir(vtfs_server_info *server, const char *name, u64 parent_id,
                 vtfs_server_inode *inode);

int server_unlink(vtfs_server_info *server, const char *name, u64 parent_id);

int server_rmdir(vtfs_server_info *server, const char *name, u64 parent_id);

int server_iterate(vtfs_server_info *server, u64 parent_id, u64 cookie,
                   vtfs_iterate_result *result);

void server_free_iterate_data(vtfs_iterate_result *iterate_result);

int server_link(vtfs_server_info *server, u64 parent_id, u64 inode_id, const char *name);

int server_write(vtfs_server_info *server, u64 inode_id, char *buffer, loff_t offset, size_t len);

int server_read(vtfs_server_info *server, u64 inode_id, char *buffer, loff_t offset, size_t len);
#endif  // !VTFS_SERVER_H
