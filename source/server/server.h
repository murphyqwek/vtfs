#ifndef VTFS_SERVER_H
#define VTFS_SERVER_H

#include "server_info.h"

char *escape_json_name(const char *name, size_t len);

int server_mount(vtfs_server_info *server, vtfs_server_inode *inode);

int server_lookup(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode);

int server_create(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode);

int server_mkdir(vtfs_server_info *server, const char *name, u64 parent_id,
                 vtfs_server_inode *inode);
#endif  // !VTFS_SERVER_H
