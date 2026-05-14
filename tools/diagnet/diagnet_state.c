#include "diagnet.h"

#include <stdio.h>
#include <string.h>

diagnet_proto_t diagnet_proto_from_libdiag(const char *s)
{
    if (s == NULL) {
        return DIAGNET_PROTO_UNKNOWN;
    }
    if (strcmp(s, "tcp") == 0) return DIAGNET_PROTO_TCP;
    if (strcmp(s, "udp") == 0) return DIAGNET_PROTO_UDP;
    return DIAGNET_PROTO_UNKNOWN;
}

const char *diagnet_proto_name(diagnet_proto_t p)
{
    switch (p) {
    case DIAGNET_PROTO_TCP: return "tcp";
    case DIAGNET_PROTO_UDP: return "udp";
    default:                return "unknown";
    }
}

const char *diagnet_state_name(diagnet_state_t s)
{
    switch (s) {
    case DIAGNET_STATE_NONE:        return "NONE";
    case DIAGNET_STATE_ESTABLISHED: return "ESTABLISHED";
    case DIAGNET_STATE_SYN_SENT:    return "SYN_SENT";
    case DIAGNET_STATE_SYN_RECV:    return "SYN_RECV";
    case DIAGNET_STATE_FIN_WAIT1:   return "FIN_WAIT1";
    case DIAGNET_STATE_FIN_WAIT2:   return "FIN_WAIT2";
    case DIAGNET_STATE_TIME_WAIT:   return "TIME_WAIT";
    case DIAGNET_STATE_CLOSE:       return "CLOSE";
    case DIAGNET_STATE_CLOSE_WAIT:  return "CLOSE_WAIT";
    case DIAGNET_STATE_LAST_ACK:    return "LAST_ACK";
    case DIAGNET_STATE_LISTEN:      return "LISTEN";
    case DIAGNET_STATE_CLOSING:     return "CLOSING";
    default:                        return "UNKNOWN";
    }
}

diagnet_state_t diagnet_state_from_name(const char *name)
{
    if (name == NULL) {
        return DIAGNET_STATE_UNKNOWN;
    }
    if (strcmp(name, "NONE")        == 0) return DIAGNET_STATE_NONE;
    if (strcmp(name, "ESTABLISHED") == 0) return DIAGNET_STATE_ESTABLISHED;
    if (strcmp(name, "SYN_SENT")    == 0) return DIAGNET_STATE_SYN_SENT;
    if (strcmp(name, "SYN_RECV")    == 0) return DIAGNET_STATE_SYN_RECV;
    if (strcmp(name, "FIN_WAIT1")   == 0) return DIAGNET_STATE_FIN_WAIT1;
    if (strcmp(name, "FIN_WAIT2")   == 0) return DIAGNET_STATE_FIN_WAIT2;
    if (strcmp(name, "TIME_WAIT")   == 0) return DIAGNET_STATE_TIME_WAIT;
    if (strcmp(name, "CLOSE")       == 0) return DIAGNET_STATE_CLOSE;
    if (strcmp(name, "CLOSE_WAIT")  == 0) return DIAGNET_STATE_CLOSE_WAIT;
    if (strcmp(name, "LAST_ACK")    == 0) return DIAGNET_STATE_LAST_ACK;
    if (strcmp(name, "LISTEN")      == 0) return DIAGNET_STATE_LISTEN;
    if (strcmp(name, "CLOSING")     == 0) return DIAGNET_STATE_CLOSING;
    return DIAGNET_STATE_UNKNOWN;
}

diagnet_state_t diagnet_state_from_libdiag(const char *s)
{
    return diagnet_state_from_name(s);
}

void diagnet_conn_from_libdiag(const diag_net_conn_t *src, diagnet_conn_t *dst)
{
    dst->proto       = diagnet_proto_from_libdiag(src->proto);
    dst->local_port  = src->local_port;
    dst->remote_port = src->remote_port;

    snprintf(dst->local_addr,  sizeof(dst->local_addr),  "%s", src->local_addr);
    snprintf(dst->remote_addr, sizeof(dst->remote_addr), "%s", src->remote_addr);

    /* UDP has no TCP state machine; ignore libdiag's per-line state byte. */
    if (dst->proto == DIAGNET_PROTO_UDP) {
        dst->state = DIAGNET_STATE_NONE;
    } else {
        dst->state = diagnet_state_from_libdiag(src->state);
    }
}
