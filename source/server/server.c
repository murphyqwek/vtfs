#include "server.h"

#include <linux/string.h>
#include "../http_vtfs.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "linux/gfp_types.h"
#include "linux/sprintf.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "../parser/parser.h"
#include "../helpers.h"
#include "server_info.h"

static inline bool request_success(int http_code) {
    return http_code >= 200 && http_code < 300;
}

char *escape_json_name(const char *name, size_t len) {
    static const char hex[] = "0123456789ABCDEF";

    char *result = kmalloc(len * 6 + 1, GFP_KERNEL);
    if (!result) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char sym = (unsigned char)name[i];

        if (sym == '"' || sym == '\\') {
            result[j++] = '\\';
            result[j++] = sym;
            continue;
        }

        if (sym < 0x20) {
            result[j++] = '\\';
            result[j++] = 'u';
            result[j++] = '0';
            result[j++] = '0';
            result[j++] = hex[sym >> 4];
            result[j++] = hex[sym & 0x0F];

            continue;
        }

        result[j++] = sym;
    }

    result[j] = '\0';
    return result;
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

static int server_create_base(vtfs_server_info *server, const char *name, u64 parent_id,
                              vtfs_server_inode *inode, const char *url) {
    if (!server || !name || !inode) {
        return -EINVAL;
    }

    size_t name_len = strnlen(name, VTFS_MAX_NAME_LEN + 1);

    if (name_len > VTFS_MAX_NAME_LEN) {
        return -ENAMETOOLONG;
    }

    char *escaped_name = escape_json_name(name, name_len);
    if (!escaped_name) {
        return -ENOMEM;
    }

    char *body = kasprintf(GFP_KERNEL, "{\"parent_inode_id\":%llu,\"name\":\"%s\"}",
                           (unsigned long long)parent_id, escaped_name);

    if (!body) {
        return -ENOMEM;
    }

    kfree(escaped_name);

    vtfs_http_request request = {
        .method = POST,
        .path = url,
        .body = body,
        .body_len = strlen(body),
    };

    vtfs_http_response response = {0};

    int code = execute_request(server, &request, &response);

    kfree(body);

    if (code < 0) {
        ERR("server_create/mkdir got error on execution request");
        response_free(&response);
        return code;
    }

    if (request_success(response.http_result)) {
        LOG("server_create/mkdirgot response from server");
        code = parse_to_inode(response.data, response.data_length, inode);
    } else {
        LOG("server_create/mkdir got error response from server");
        int error_code = parse_to_error_code(response.data, response.data_length, &code);
        if (error_code < 0) {
            code = error_code;
        }
    }

    response_free(&response);

    return code;
}

int server_create(vtfs_server_info *server, const char *name, u64 parent_id,
                  vtfs_server_inode *inode) {
    LOG("begin server create");

    return server_create_base(server, name, parent_id, inode, "/api/create");
}

int server_mkdir(vtfs_server_info *server, const char *name, u64 parent_id,
                 vtfs_server_inode *inode) {
    LOG("begin server mkdir");

    return server_create_base(server, name, parent_id, inode, "/api/mkdir");
}
