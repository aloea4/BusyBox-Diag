#include "libdiag/diag_common.h"
#include "libdiag/diag_net.h"

#include <stdio.h>

static int print_conn(const diag_net_conn_t *c, void *user)
{
    int *count = (int *)user;

    printf(
        "%-4s %-15s:%-5u -> %-15s:%-5u %s\n",
        c->proto,
        c->local_addr,
        c->local_port,
        c->remote_addr,
        c->remote_port,
        c->state
    );

    (*count)++;

    if (*count >= 30) {
        return 1;
    }

    return 0;
}

int main(void)
{
    int count = 0;
    int ret;

    printf("Network connections:\n");

    ret = diag_net_foreach_tcp(print_conn, &count);
    if (ret != DIAG_OK && ret != 1) {
        printf("diag_net_foreach_tcp failed: %s\n", diag_strerror(ret));
        return 1;
    }

    ret = diag_net_foreach_udp(print_conn, &count);
    if (ret != DIAG_OK && ret != 1) {
        printf("diag_net_foreach_udp failed: %s\n", diag_strerror(ret));
        return 1;
    }

    printf("test_net: PASS (%d connections shown)\n", count);
    return 0;
}