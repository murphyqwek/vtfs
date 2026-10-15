#include "http_vtfs.h"
#include "asm-generic/errno-base.h"
#include "http_parser.h"
#include "linux/gfp_types.h"
#include "linux/net.h"
#include "linux/slab.h"
#include "linux/socket.h"
#include "linux/stddef.h"
#include "linux/uio.h"
#include "net/sock.h"
#include <linux/string.h>

/**
 * connect_to_server - creates connection to server
 * @server: pointer to vtfs_server_info
 * @socket: result of connect_to_server - created socket
 *
 *
 * Return: 0 if all good else error code
 */
int connect_to_server(vtfs_server_info *server, struct socket **socket) {
    int error;

    if (!server) {
        return -EINVAL;
    }

    if (!server->server_ip) {
        return -EINVAL;
    }

    if (server->server_port == 0) {
        return -EINVAL;
    }

    if (!socket) {
        return -EINVAL;
    }

    error = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, socket);
    if (error < 0) {
        return error;
    }

    sock_set_sndtimeo((*socket)->sk, SEND_TIMOUT);
    WRITE_ONCE((*socket)->sk->sk_rcvtimeo, RECEIVE_TIMOUT * HZ);

    struct sockaddr_in s_addr = {
        .sin_family = AF_INET,
        .sin_addr = {.s_addr = in_aton(server->server_ip)},
        .sin_port = htons(server->server_port),
    };

    error = kernel_connect(*socket, (struct sockaddr *)&s_addr, sizeof(struct sockaddr_in), 0);
    if (error != 0) {
        if (error == EINPROGRESS) {
            error = -ETIMEDOUT;
        }

        sock_release(*socket);

        *socket = NULL;
        return error;
    }

    return 0;
}

/**
 * fill_request_buffer - fills buffer for request
 *
 * Return: written len or error code
 */
static int fill_request_buffer(char *headers, vtfs_http_request *request, size_t len,
                               vtfs_server_info *server) {
    return snprintf(headers, len, headers_format, request->method, request->path, server->server_ip,
                    (unsigned int)server->server_port, server->login, request->body_len);
}

/**
 * build_http_request_headers - creates buffer for filled http headers
 * @server: server vtfs_server_info
 * @request: request
 * @buffer: pointer to headers buffer (the function creates it)
 * @buffer_size: pointer to buffer_size
 *
 *
 * Return: 0 if all good else error code
 */
int build_http_request_headers(vtfs_server_info *server, vtfs_http_request *request, char **buffer,
                               size_t *buffer_size) {
    if (!server || !server->server_ip) {
        return -EINVAL;
    }

    if (server->server_port == 0) {
        return -EINVAL;
    }

    if (!request) {
        return -EINVAL;
    }

    if (request->body_len > 0 && !request->body) {
        return -EINVAL;
    }

    if (!buffer || !buffer_size) {
        return -EINVAL;
    }

    *buffer = NULL;
    *buffer_size = 0;

    int len = fill_request_buffer(NULL, request, 0, server);

    if (len < 0) {
        return len;
    }

    if (len >= MAX_HEADER_SIZE) {
        return -E2BIG;
    }

    char *headers = kmalloc(len + 1, GFP_KERNEL);

    if (!headers) {
        return -ENOMEM;
    }

    int written = fill_request_buffer(headers, request, len + 1, server);

    if (written != len) {
        kfree(headers);
        return -EINVAL;
    }

    *buffer = headers;
    *buffer_size = (size_t)len;

    return 0;
}

/**
 * send_all - sends all data to server
 * @socket: socket
 * @to_send_data: buffer to data
 * @to_send_size: size of the buffer
 *
 * It sends all data, automaticly chunks
 *
 * Return: 0 if all good esle error code
 */
int send_all(struct socket *socket, void *to_send_data, size_t to_send_size) {
    struct msghdr msg = {0};

    struct kvec kvec = {
        .iov_base = to_send_data,
        .iov_len = to_send_size,
    };

    if (!socket) {
        return -EINVAL;
    }

    while (kvec.iov_len != 0) {
        int offset = kernel_sendmsg(socket, &msg, &kvec, 1, kvec.iov_len);

        if (offset < 0) {
            return offset;
        }

        if (offset == 0) {
            return -EPIPE;
        }

        kvec.iov_base = (char *)kvec.iov_base + offset;
        kvec.iov_len -= offset;
    }

    return 0;
}

static char *find_http_separator(char *data, size_t len) {
    size_t i;

    if (len < 4) {
        return NULL;
    }

    for (i = 0; i <= len - 4; i++) {
        if (memcmp(data + i, "\r\n\r\n", 4) == 0) {
            return data + i;
        }
    }

    return NULL;
}

/**
 * receive_headers - receive headers data
 * @socket: socket
 * @buffer: result data
 * @buffer_size: result data length
 * @body_offset: offset between end of data and body data
 *
 * Can get some body data
 *
 * Return: 0 if all good else error code
 */
int receive_headers(struct socket *socket, char **buffer, size_t *buffer_size,
                    size_t *body_offset) {
    struct msghdr msg = {0};

    if (!socket) {
        return -EINVAL;
    }

    if (!buffer || !buffer_size || !body_offset) {
        return -EINVAL;
    }

    *buffer = NULL;
    *buffer_size = 0;
    *body_offset = 0;

    char *data = kmalloc(MAX_HEADER_SIZE, GFP_KERNEL);

    if (!data) {
        return -ENOMEM;
    }

    size_t received = 0;

    while (received < MAX_HEADER_SIZE) {
        struct kvec kvec = {.iov_base = data + received, .iov_len = MAX_HEADER_SIZE - received};

        int rec = kernel_recvmsg(socket, &msg, &kvec, 1, kvec.iov_len, 0);

        if (rec < 0) {
            kfree(data);
            return rec;
        }

        if (rec == 0) {
            kfree(data);
            return -ECONNRESET;
        }

        received += rec;

        char *separetor = find_http_separator(data, received);

        if (!separetor) {
            continue;
        }

        // Нужно для удобного парсинга
        separetor[3] = '\0';

        *buffer_size = received;
        *body_offset = separetor - data + 4;

        char *reduced_data = krealloc(data, received, GFP_KERNEL);

        if (!reduced_data) {
            kfree(data);
            return -ENOMEM;
        }

        data = reduced_data;
        *buffer = data;

        return 0;
    }

    kfree(data);
    return -E2BIG;
}

/**
 * receive_body - receive body
 * @socket: socket
 * @headers_buffer: headers data
 * @buffer_size: size of headers
 * @body_offset: offset of body
 * @content_length: content length
 * @body: data of body
 *
 *
 * Return: 0 if all good else error code
 */
int receive_body(struct socket *socket, const char *headers_buffer, size_t buffer_size,
                 size_t body_offset, size_t content_length, char **body) {
    struct msghdr msg = {0};

    if (!socket) {
        return -EINVAL;
    }

    if (!headers_buffer) {
        return -EINVAL;
    }

    if (!body) {
        return -EINVAL;
    }

    *body = NULL;

    if (buffer_size < body_offset) {
        return -EINVAL;
    }

    if (content_length > MAX_BODY_SIZE) {
        return -E2BIG;
    }

    size_t recieved = buffer_size - body_offset;

    if (recieved > content_length) {
        return -EPROTO;
    }

    if (content_length == 0) {
        return 0;
    }

    char *data = kvmalloc(content_length, GFP_KERNEL);

    if (!data) {
        return -ENOMEM;
    }

    memcpy(data, headers_buffer + body_offset, recieved);

    while (recieved < content_length) {
        struct kvec kvec = {
            .iov_base = data + recieved,
            .iov_len = content_length - recieved,
        };

        int rec = kernel_recvmsg(socket, &msg, &kvec, 1, kvec.iov_len, 0);

        if (rec < 0) {
            kvfree(data);
            return rec;
        }

        if (rec == 0) {
            kvfree(data);
            return -ECONNRESET;
        }

        recieved += rec;
    }

    *body = data;
    return 0;
}

/**
 * execute_request - send request to server and returns response
 * @server: vtfs_server_info
 * @request: vtfs_http_request
 * @response: response
 *
 * Return: 0 if all good else error code
 */
int execute_request(vtfs_server_info *server, vtfs_http_request *request,
                    vtfs_http_response *response) {
    if (!server || !request || !response) {
        return -EINVAL;
    }

    // Создаем сокет
    struct socket *socket = NULL;

    int code = connect_to_server(server, &socket);

    if (code < 0) {
        return code;
    }

    // Формируем запрос
    char *headers = NULL;
    size_t headers_size = 0;

    code = build_http_request_headers(server, request, &headers, &headers_size);

    if (code < 0) {
        sock_release(socket);
        return code;
    }

    // Отправляем заголовок
    code = send_all(socket, headers, headers_size);

    if (code < 0) {
        sock_release(socket);
        kfree(headers);
        return code;
    }

    // Отправляем тело (если есть)
    if (request->body_len > 0) {
        code = send_all(socket, request->body, request->body_len);

        if (code < 0) {
            sock_release(socket);
            kfree(headers);
            return code;
        }
    }

    // Принимаем заголовок
    size_t body_offset = 0;

    char *response_buffer = NULL;
    size_t response_buffer_size = 0;

    code = receive_headers(socket, &response_buffer, &response_buffer_size, &body_offset);

    if (code < 0) {
        sock_release(socket);
        kfree(headers);
        return code;
    }

    // Парсим заголовок
    vtfs_http_headers_parsed parsed;
    code = parse_http_headers(response_buffer, body_offset, &parsed);

    if (code < 0) {
        sock_release(socket);
        kfree(headers);
        kfree(response_buffer);
        return code;
    }

    // Принимаем тело
    char *body = NULL;
    code = receive_body(socket, response_buffer, response_buffer_size, body_offset,
                        parsed.content_length, &body);

    if (code < 0) {
        sock_release(socket);
        kfree(headers);
        kfree(response_buffer);
        return code;
    }

    response->data = body;
    response->http_result = parsed.http_status_code;
    response->data_length = parsed.content_length;

    sock_release(socket);
    kfree(headers);
    kfree(response_buffer);

    return 0;
}

/**
 * response_free - free data that vtfs_http_response holds
 * @response: pointer to vtfs_http_response
 *
 *
 * Return: Nothing
 */
void response_free(vtfs_http_response *response) {
    if (!response) {
        return;
    }

    if (!response->data) {
        return;
    }

    kvfree(response->data);
    response->http_result = 0;
    response->data = NULL;
    response->data_length = 0;
}
