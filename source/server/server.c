#include "server.h"
#include <linux/string.h>
#include "../http_vtfs.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "../parser/parser.h"
#include "../helpers.h"
#include "server_info.h"

static inline bool request_success(int http_code) {
    return http_code >= 200 && http_code < 300;
}

int server_mount(vtfs_server_info *server, vtfs_server_inode *inode) {
    vtfs_http_request request = {
        .method = GET,
        .path = "/api/mount",
        .body = NULL,
        .body_len = 0,
    };

    LOG("calling mount..");

    vtfs_http_response response = {0};

    int code = execute_request(server, &request, &response);

    if (code < 0) {
        response_free(&response);
        LOG("server_mount got error when execute_request");
        return code;
    }

    code = 0;
    int parse_code = 0;

    if (request_success(response.http_result)) {
        LOG("server_mount got response from server");
        code = parse_to_inode(response.data, response.data_length, inode);
    } else {
        LOG("server_mount got error response");
        parse_code = parse_to_error_code(response.data, response.data_length, &code);
    }

    response_free(&response);

    if (parse_code < 0) {
        return parse_code;
    }

    return code;
}

int server_lookup(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode) {
    if (parent_id <= 0) {
        return -EINVAL;
    }

    if (!server || !name || !inode) {
        return -EINVAL;
    }

    size_t name_len = strnlen(name, VTFS_MAX_NAME_LEN + 1);

    if (name_len > VTFS_MAX_NAME_LEN) {
        return -ENAMETOOLONG;
    }

    char url[VTFS_MAX_URL_SIZE];

    int prefix_len = snprintf(
        url, sizeof(url), "/api/lookup?parent_inode_id=%llu&name=", (unsigned long long)parent_id);

    if (prefix_len < 0 || prefix_len >= sizeof(url)) {
        return -EINVAL;
    }

    int encoded_len = url_encode(name, name_len, url + prefix_len, sizeof(url) - prefix_len);

    if (encoded_len < 0) {
        return encoded_len;
    }

    LOG("begin server lookup");

    vtfs_http_request request = {
        .method = GET,
        .path = url,
        .body = NULL,
        .body_len = 0,
    };

    vtfs_http_response response = {0};

    int code = execute_request(server, &request, &response);

    if (code < 0) {
        ERR("server_lookup got error on execution request");
        response_free(&response);
        return code;
    }

    if (request_success(response.http_result)) {
        LOG("server_lookup got response from server");
        code = parse_to_inode(response.data, response.data_length, inode);
    } else {
        LOG("server_lookup got error response from server");
        int error_code = parse_to_error_code(response.data, response.data_length, &code);
        if (error_code < 0) {
            code = error_code;
        }
    }

    response_free(&response);

    return code;
}
