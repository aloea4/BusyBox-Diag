#include "libdiag/diag_reader.h"
#include "libdiag/diag_common.h"

#include <stdio.h>
#include <string.h>

int diag_read_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp;

    if (path == NULL || buf == NULL || size == 0) {
        return DIAG_ERR_INVALID_ARG;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return DIAG_ERR_IO;
    }

    if (fgets(buf, (int)size, fp) == NULL) {
        fclose(fp);
        return DIAG_ERR_IO;
    }

    fclose(fp);

    return DIAG_OK;
}

int diag_foreach_line(
    const char *path,
    int (*callback)(const char *line, void *user),
    void *user
)
{
    FILE *fp;
    char line[DIAG_LINE_MAX];

    if (path == NULL || callback == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return DIAG_ERR_IO;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int cb_ret = callback(line, user);
        if (cb_ret != 0) {
            fclose(fp);
            return cb_ret;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return DIAG_ERR_IO;
    }

    fclose(fp);
    return DIAG_OK;
}