#include "server.h"
#include "../http_vtfs.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "../parser/parser.h"
#include "../helpers.h"

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
