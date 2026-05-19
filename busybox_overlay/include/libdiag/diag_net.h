#ifndef LIBDIAG_DIAG_NET_H
#define LIBDIAG_DIAG_NET_H

#include "libdiag/diag_common.h"

typedef struct {
    char proto[8];

    char local_addr[64];
    unsigned int local_port;

    char remote_addr[64];
    unsigned int remote_port;

    char state[32];
} diag_net_conn_t;

int diag_net_parse_tcp_line(const char *line, diag_net_conn_t *out);

int diag_net_foreach_tcp(
    int (*callback)(const diag_net_conn_t *conn, void *user),
    void *user
);

int diag_net_foreach_udp(
    int (*callback)(const diag_net_conn_t *conn, void *user),
    void *user
);

#endif