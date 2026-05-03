#include "libdiag/diag_parse.h"
#include "libdiag/diag_common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *diag_strerror(int err)
{
    switch (err) {
    case DIAG_OK:
        return "OK";
    case DIAG_ERR_IO:
        return "I/O error";
    case DIAG_ERR_PARSE:
        return "parse error";
    case DIAG_ERR_INVALID_ARG:
        return "invalid argument";
    case DIAG_ERR_UNSUPPORTED:
        return "unsupported operation";
    case DIAG_ERR_BUFFER_TOO_SMALL:
        return "buffer too small";
    default:
        return "unknown error";
    }
}

int diag_parse_ulong(const char *s, unsigned long *out)
{
    char *end;
    unsigned long value;

    if (s == NULL || out == NULL || *s == '\0') {
        return DIAG_ERR_INVALID_ARG;
    }

    errno = 0;
    value = strtoul(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0') {
        return DIAG_ERR_PARSE;
    }

    *out = value;
    return DIAG_OK;
}

int diag_parse_hex_u32(const char *s, unsigned int *out)
{
    char *end;
    unsigned long value;

    if (s == NULL || out == NULL || *s == '\0') {
        return DIAG_ERR_INVALID_ARG;
    }

    errno = 0;
    value = strtoul(s, &end, 16);

    if (errno != 0 || end == s || *end != '\0' || value > 0xffffffffUL) {
        return DIAG_ERR_PARSE;
    }

    *out = (unsigned int)value;
    return DIAG_OK;
}

int diag_ipv4_hex_le_to_str(const char *hex, char *out, size_t out_size)
{
    unsigned int value;
    unsigned int b1, b2, b3, b4;
    int n;

    if (hex == NULL || out == NULL || out_size == 0) {
        return DIAG_ERR_INVALID_ARG;
    }

    if (strlen(hex) != 8) {
        return DIAG_ERR_PARSE;
    }

    if (diag_parse_hex_u32(hex, &value) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    b1 = value & 0xff;
    b2 = (value >> 8) & 0xff;
    b3 = (value >> 16) & 0xff;
    b4 = (value >> 24) & 0xff;

    n = snprintf(out, out_size, "%u.%u.%u.%u", b1, b2, b3, b4);

    if (n < 0 || (size_t)n >= out_size) {
        return DIAG_ERR_BUFFER_TOO_SMALL;
    }

    return DIAG_OK;
}