#ifndef HTTP_VTFS_NEW_H
#define HTTP_VTFS_NEW_H

#include <linux/inet.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/sock.h>
#include <net/net_namespace.h>
#include <linux/err.h>
#include "server/server_info.h"

#define SEND_TIMOUT 3
#define RECEIVE_TIMOUT 15

#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE 4096

typedef struct vtfs_http_request {
    const char *method;
    const char *path;
    void *body;
    size_t body_len;
} vtfs_http_request;

typedef struct vtfs_http_response {
    const void *data;

    int http_result;
    size_t data_length;
} vtfs_http_response;

static const char headers_format[] =
    "%s %s HTTP/1.1\r\n"
    "Host: %s:%u\r\n"
    "X-Login: %s\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n"
    "\r\n";

int connect_to_server(vtfs_server_info *server, struct socket **socket);

int build_http_request_headers(vtfs_server_info *server, vtfs_http_request *request, char **buffer,
                               size_t *buffer_size);

int send_all(struct socket *socket, void *to_send_data, size_t to_send_size);

int receive_headers(struct socket *socket, char **buffer, size_t *buffer_size, size_t *body_offset);

int receive_body(struct socket *socket, const char *headers_buffer, size_t buffer_size,
                 size_t body_offset, size_t content_length, char **body);

int execute_request(vtfs_server_info *server, vtfs_http_request *request,
                    vtfs_http_response *response);

void response_free(vtfs_http_response *response);

#endif  // !HTTP_VTF_NEW_H
