#include "libdiag/diag_common.h"
#include "libdiag/diag_parse.h"
#include "libdiag/diag_reader.h"

#include <stdio.h>
#include <string.h>

static int test_ulong(void)
{
    unsigned long value = 0;

    if (diag_parse_ulong("123", &value) != DIAG_OK || value != 123UL) {
        printf("test_ulong failed: 123\n");
        return 1;
    }

    if (diag_parse_ulong("abc", &value) != DIAG_ERR_PARSE) {
        printf("test_ulong failed: abc should parse error\n");
        return 1;
    }

    if (diag_parse_ulong(NULL, &value) != DIAG_ERR_INVALID_ARG) {
        printf("test_ulong failed: NULL should invalid arg\n");
        return 1;
    }

    return 0;
}

static int test_hex(void)
{
    unsigned int value = 0;

    if (diag_parse_hex_u32("0A", &value) != DIAG_OK || value != 10U) {
        printf("test_hex failed: 0A\n");
        return 1;
    }

    if (diag_parse_hex_u32("GG", &value) != DIAG_ERR_PARSE) {
        printf("test_hex failed: GG should parse error\n");
        return 1;
    }

    return 0;
}

static int test_ip(void)
{
    char buf[64];

    if (diag_ipv4_hex_le_to_str("0100007F", buf, sizeof(buf)) != DIAG_OK) {
        printf("test_ip failed: convert\n");
        return 1;
    }

    if (strcmp(buf, "127.0.0.1") != 0) {
        printf("test_ip failed: expected 127.0.0.1, got %s\n", buf);
        return 1;
    }

    if (diag_ipv4_hex_le_to_str("00000000", buf, sizeof(buf)) != DIAG_OK) {
        printf("test_ip failed: 0.0.0.0 convert\n");
        return 1;
    }

    if (strcmp(buf, "0.0.0.0") != 0) {
        printf("test_ip failed: expected 0.0.0.0, got %s\n", buf);
        return 1;
    }

    if (diag_ipv4_hex_le_to_str("BAD", buf, sizeof(buf)) != DIAG_ERR_PARSE) {
        printf("test_ip failed: BAD should parse error\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_ulong() != 0) {
        return 1;
    }

    if (test_hex() != 0) {
        return 1;
    }

    if (test_ip() != 0) {
        return 1;
    }

    printf("test_parse: PASS\n");
    return 0;
}