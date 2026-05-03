#include "libdiag/diag_net.h"
#include "libdiag/diag_common.h"
#include "libdiag/diag_parse.h"
#include "libdiag/diag_reader.h"

#include <stdio.h>
#include <string.h>

static const char *tcp_state_name(unsigned int state)
{
    switch (state) {
    case 0x01: return "ESTABLISHED";
    case 0x02: return "SYN_SENT";
    case 0x03: return "SYN_RECV";
    case 0x04: return "FIN_WAIT1";
    case 0x05: return "FIN_WAIT2";
    case 0x06: return "TIME_WAIT";
    case 0x07: return "CLOSE";
    case 0x08: return "CLOSE_WAIT";
    case 0x09: return "LAST_ACK";
    case 0x0A: return "LISTEN";
    case 0x0B: return "CLOSING";
    default: return "UNKNOWN";
    }
}

static int parse_addr_port(
    const char *field,
    char *addr_out,
    size_t addr_size,
    unsigned int *port_out
)
{
    char ip_hex[9];
    char port_hex[5];

    if (field == NULL || addr_out == NULL || port_out == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    if (sscanf(field, "%8[0-9A-Fa-f]:%4[0-9A-Fa-f]", ip_hex, port_hex) != 2) {
        return DIAG_ERR_PARSE;
    }

    if (diag_ipv4_hex_le_to_str(ip_hex, addr_out, addr_size) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    if (diag_parse_hex_u32(port_hex, port_out) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    return DIAG_OK;
}

static int parse_net_line(const char *line, const char *proto, diag_net_conn_t *out)
{
    char local[80];
    char remote[80];
    char state_hex[8];
    unsigned int state_num;

    if (line == NULL || proto == NULL || out == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    if (sscanf(line, " %*d: %79s %79s %7s", local, remote, state_hex) != 3) {
        return DIAG_ERR_PARSE;
    }

    snprintf(out->proto, sizeof(out->proto), "%s", proto);

    if (parse_addr_port(local, out->local_addr, sizeof(out->local_addr), &out->local_port) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    if (parse_addr_port(remote, out->remote_addr, sizeof(out->remote_addr), &out->remote_port) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    if (diag_parse_hex_u32(state_hex, &state_num) != DIAG_OK) {
        return DIAG_ERR_PARSE;
    }

    snprintf(out->state, sizeof(out->state), "%s", tcp_state_name(state_num));

    return DIAG_OK;
}

int diag_net_parse_tcp_line(const char *line, diag_net_conn_t *out)
{
    return parse_net_line(line, "tcp", out);
}

typedef struct {
    const char *proto;
    int (*callback)(const diag_net_conn_t *conn, void *user);
    void *user;
    int line_no;
} net_foreach_ctx_t;

static int foreach_net_line_cb(const char *line, void *user)
{
    net_foreach_ctx_t *ctx = (net_foreach_ctx_t *)user;
    diag_net_conn_t conn;
    int ret;

    ctx->line_no++;

    if (ctx->line_no == 1) {
        return 0;
    }

    ret = parse_net_line(line, ctx->proto, &conn);
    if (ret != DIAG_OK) {
        return 0;
    }

    return ctx->callback(&conn, ctx->user);
}

static int diag_net_foreach_file(
    const char *path,
    const char *proto,
    int (*callback)(const diag_net_conn_t *conn, void *user),
    void *user
)
{
    net_foreach_ctx_t ctx;

    if (path == NULL || proto == NULL || callback == NULL) {
        return DIAG_ERR_INVALID_ARG;
    }

    ctx.proto = proto;
    ctx.callback = callback;
    ctx.user = user;
    ctx.line_no = 0;

    return diag_foreach_line(path, foreach_net_line_cb, &ctx);
}

int diag_net_foreach_tcp(
    int (*callback)(const diag_net_conn_t *conn, void *user),
    void *user
)
{
    return diag_net_foreach_file("/proc/net/tcp", "tcp", callback, user);
}

int diag_net_foreach_udp(
    int (*callback)(const diag_net_conn_t *conn, void *user),
    void *user
)
{
    return diag_net_foreach_file("/proc/net/udp", "udp", callback, user);
}