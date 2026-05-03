#ifndef LIBDIAG_DIAG_PARSE_H
#define LIBDIAG_DIAG_PARSE_H

#include <stddef.h>

int diag_parse_ulong(const char *s, unsigned long *out);
int diag_parse_hex_u32(const char *s, unsigned int *out);
int diag_ipv4_hex_le_to_str(const char *hex, char *out, size_t out_size);

#endif