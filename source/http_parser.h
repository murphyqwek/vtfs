#ifndef VTFS_HTTP_PARSER_H
#define VTFS_HTTP_PARSER_H

#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/stddef.h>

typedef struct vtfs_http_headers_parsed {
    int http_status_code;
    size_t content_length;
} vtfs_http_headers_parsed;

int parse_http_headers(const char *buffer, size_t body_offset, vtfs_http_headers_parsed *result);

#endif
