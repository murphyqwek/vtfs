#include "http_parser.h"
#include "http_vtfs.h"

static int get_content_length(const char *headers, size_t *content_length) {
    const char *pos = strstr(headers, "Content-Length:");

    if (!pos) {
        return -EINVAL;
    }

    pos += strlen("Content-Length:");

    unsigned long long value;

    if (sscanf(pos, "%llu", &value) != 1) {
        return -EINVAL;
    }

    if (value > MAX_BODY_SIZE) {
        return -E2BIG;
    }

    *content_length = (size_t)value;
    return 0;
}

static int get_http_code_status(const char *buffer, size_t body_offset) {
    char *line = memchr(buffer, '\n', body_offset);

    if (!line || line == buffer || line[-1] != '\r') {
        return -EINVAL;
    }

    if (line - buffer < 13) {
        return -EINVAL;
    }

    if (memcmp(buffer, "HTTP/1.", 7) != 0) {
        return -EINVAL;
    }

    return (buffer[9] - '0') * 100 + (buffer[10] - '0') * 10 + (buffer[11] - '0');
}

/**
 * parse_http_headers - gets https status code and content length
 * @buffer: headers data
 * @body_offset: offset between and of buffer and body data
 * @result: parsed http status and content length
 *
 *
 * Return: 0 if all good else error code
 */
int parse_http_headers(const char *buffer, size_t body_offset, vtfs_http_headers_parsed *result) {
    if (!buffer) {
        return -EINVAL;
    }

    if (!result) {
        return -EINVAL;
    }

    if (body_offset < 4) {
        return -EINVAL;
    }

    result->http_status_code = 0;
    result->content_length = 0;

    int http_status_code = get_http_code_status(buffer, body_offset);

    if (http_status_code < 0) {
        return http_status_code;
    }

    result->http_status_code = http_status_code;

    size_t content_length = 0;
    int error_code = get_content_length(buffer, &content_length);
    if (error_code < 0) {
        return error_code;
    }

    result->content_length = content_length;

    return 0;
}
