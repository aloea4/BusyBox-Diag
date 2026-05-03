#ifndef LIBDIAG_DIAG_COMMON_H
#define LIBDIAG_DIAG_COMMON_H

#define DIAG_OK 0
#define DIAG_ERR_IO -1
#define DIAG_ERR_PARSE -2
#define DIAG_ERR_INVALID_ARG -3
#define DIAG_ERR_UNSUPPORTED -4
#define DIAG_ERR_BUFFER_TOO_SMALL -5

#define DIAG_PATH_MAX 256
#define DIAG_NAME_MAX 64
#define DIAG_LINE_MAX 512

const char *diag_strerror(int err);

#endif