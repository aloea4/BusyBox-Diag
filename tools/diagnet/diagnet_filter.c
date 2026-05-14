#include "diagnet.h"

#include <string.h>

int diagnet_filter_match(const diagnet_filter_t *f, const diagnet_conn_t *c)
{
    if (f->proto_set && c->proto != f->proto) {
        return 0;
    }

    if (f->listen_only && c->state != DIAGNET_STATE_LISTEN) {
        return 0;
    }

    if (f->state_set && c->state != f->state) {
        return 0;
    }

    if (f->lport_set && c->local_port != f->lport) {
        return 0;
    }

    if (f->rport_set && c->remote_port != f->rport) {
        return 0;
    }

    if (f->laddr && strcmp(c->local_addr, f->laddr) != 0) {
        return 0;
    }

    if (f->raddr && strcmp(c->remote_addr, f->raddr) != 0) {
        return 0;
    }

    return 1;
}
