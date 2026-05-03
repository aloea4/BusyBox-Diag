#ifndef LIBDIAG_DIAG_READER_H
#define LIBDIAG_DIAG_READER_H

#include <stddef.h>

int diag_read_first_line(const char *path, char *buf, size_t size);
int diag_foreach_line(
    const char *path,
    int (*callback)(const char *line, void *user),
    void *user
);

#endif