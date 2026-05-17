#include "diagnet.h"

#include <stdio.h>
#include <string.h>

static const char *state_display(diagnet_state_t s, diagnet_proto_t p)
{
    if (p == DIAGNET_PROTO_UDP) {
        return "-";
    }
    return diagnet_state_name(s);
}

static void flags_join(unsigned int flags, char *out, size_t out_size)
{
    size_t used = 0;
    int first = 1;

    out[0] = '\0';

    if (flags == 0) {
        snprintf(out, out_size, "-");
        return;
    }

    if (flags & DIAGNET_FLAG_SUSPICIOUS_PORT) {
        used += (size_t)snprintf(out + used, out_size - used, "%s%s",
                                 first ? "" : ",", "suspicious_port");
        first = 0;
    }
    if (flags & DIAGNET_FLAG_PUBLIC_LISTEN) {
        used += (size_t)snprintf(out + used, out_size - used, "%s%s",
                                 first ? "" : ",", "public_listen");
        first = 0;
    }
    (void)used;
    (void)first;
}

static const char *warning_type_name(unsigned int bit)
{
    switch (bit) {
    case DIAGNET_WARN_MANY_TIME_WAIT:  return "many_time_wait";
    case DIAGNET_WARN_MANY_CLOSE_WAIT: return "many_close_wait";
    default:                           return "unknown";
    }
}

static const char *warning_msg(unsigned int bit)
{
    switch (bit) {
    case DIAGNET_WARN_MANY_TIME_WAIT:
        return "many TIME_WAIT sockets observed";
    case DIAGNET_WARN_MANY_CLOSE_WAIT:
        return "many CLOSE_WAIT sockets observed";
    default:
        return "";
    }
}

void diagnet_output_warnings_stderr(const diagnet_ctx_t *ctx)
{
    if (ctx->quiet || ctx->global_warnings == 0) {
        return;
    }

    if (ctx->global_warnings & DIAGNET_WARN_MANY_TIME_WAIT) {
        fprintf(stderr, "diagnet: warning: %s\n",
                warning_msg(DIAGNET_WARN_MANY_TIME_WAIT));
    }
    if (ctx->global_warnings & DIAGNET_WARN_MANY_CLOSE_WAIT) {
        fprintf(stderr, "diagnet: warning: %s\n",
                warning_msg(DIAGNET_WARN_MANY_CLOSE_WAIT));
    }
}

void diagnet_output_table(const diagnet_ctx_t *ctx)
{
    int i;
    const diagnet_record_t *r;
    char flag_buf[64];

    if (!ctx->no_header) {
        printf("%-5s %-16s %-6s %-16s %-6s %-12s %s\n",
               "PROTO", "LOCAL_ADDR", "LPORT",
               "REMOTE_ADDR", "RPORT", "STATE", "FLAGS");
    }

    for (i = 0; i < ctx->count; i++) {
        r = &ctx->records[i];
        flags_join(r->flags, flag_buf, sizeof(flag_buf));
        printf("%-5s %-16s %-6u %-16s %-6u %-12s %s\n",
               diagnet_proto_name(r->conn.proto),
               r->conn.local_addr, r->conn.local_port,
               r->conn.remote_addr, r->conn.remote_port,
               state_display(r->conn.state, r->conn.proto),
               flag_buf);
    }
}

void diagnet_output_raw(const diagnet_ctx_t *ctx)
{
    int i;
    const diagnet_record_t *r;
    char flag_buf[64];

    for (i = 0; i < ctx->count; i++) {
        r = &ctx->records[i];
        flags_join(r->flags, flag_buf, sizeof(flag_buf));
        printf("%s %s %u %s %u %s %s\n",
               diagnet_proto_name(r->conn.proto),
               r->conn.local_addr, r->conn.local_port,
               r->conn.remote_addr, r->conn.remote_port,
               state_display(r->conn.state, r->conn.proto),
               flag_buf);
    }
}

void diagnet_output_stats(const diagnet_ctx_t *ctx)
{
    int i;
    int total = ctx->tcp_total + ctx->udp_total;

    printf("Total connections: %d\n", total);
    printf("  tcp: %d\n", ctx->tcp_total);
    printf("  udp: %d\n", ctx->udp_total);
    printf("Suspicious ports:  %d\n", ctx->suspicious_count);
    printf("State distribution:\n");
    for (i = 0; i < DIAGNET_STATE_NUM; i++) {
        if (ctx->state_counts[i] > 0) {
            printf("  %-12s %d\n",
                   diagnet_state_name(diagnet_state_from_idx(i)),
                   ctx->state_counts[i]);
        }
    }
}

static void print_flags_array(unsigned int flags)
{
    int first = 1;
    printf("[");
    if (flags & DIAGNET_FLAG_SUSPICIOUS_PORT) {
        printf("%s\"suspicious_port\"", first ? "" : ", ");
        first = 0;
    }
    if (flags & DIAGNET_FLAG_PUBLIC_LISTEN) {
        printf("%s\"public_listen\"", first ? "" : ", ");
        first = 0;
    }
    (void)first;
    printf("]");
}

void diagnet_output_json(const diagnet_ctx_t *ctx)
{
    int i;
    int first;
    int printed_state;
    int total;
    const diagnet_record_t *r;

    total = ctx->tcp_total + ctx->udp_total;

    printf("{\n");

    /* connections */
    printf("  \"connections\": [");
    first = 1;
    for (i = 0; i < ctx->count; i++) {
        r = &ctx->records[i];
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("\n    {");
        printf("\"protocol\": \"%s\", ",       diagnet_proto_name(r->conn.proto));
        printf("\"local_address\": \"%s\", ",  r->conn.local_addr);
        printf("\"local_port\": %u, ",         r->conn.local_port);
        printf("\"remote_address\": \"%s\", ", r->conn.remote_addr);
        printf("\"remote_port\": %u, ",        r->conn.remote_port);
        printf("\"state\": \"%s\", ",
               (r->conn.proto == DIAGNET_PROTO_UDP)
                   ? "NONE"
                   : diagnet_state_name(r->conn.state));
        printf("\"flags\": ");
        print_flags_array(r->flags);
        printf("}");
    }
    printf("%s],\n", ctx->count ? "\n  " : "");

    /* summary */
    printf("  \"summary\": {\n");
    printf("    \"total\": %d,\n", total);
    printf("    \"tcp\": %d,\n",   ctx->tcp_total);
    printf("    \"udp\": %d,\n",   ctx->udp_total);
    printf("    \"states\": {");
    printed_state = 0;
    for (i = 0; i < DIAGNET_STATE_NUM; i++) {
        if (ctx->state_counts[i] <= 0) {
            continue;
        }
        if (printed_state) {
            printf(",");
        }
        printf("\n      \"%s\": %d",
               diagnet_state_name(diagnet_state_from_idx(i)),
               ctx->state_counts[i]);
        printed_state = 1;
    }
    printf("%s}\n", printed_state ? "\n    " : "");
    printf("  },\n");

    /* warnings */
    printf("  \"warnings\": [");
    first = 1;
    if (ctx->global_warnings & DIAGNET_WARN_MANY_TIME_WAIT) {
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("\n    {\"type\": \"%s\", \"message\": \"%s\"}",
               warning_type_name(DIAGNET_WARN_MANY_TIME_WAIT),
               warning_msg(DIAGNET_WARN_MANY_TIME_WAIT));
    }
    if (ctx->global_warnings & DIAGNET_WARN_MANY_CLOSE_WAIT) {
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("\n    {\"type\": \"%s\", \"message\": \"%s\"}",
               warning_type_name(DIAGNET_WARN_MANY_CLOSE_WAIT),
               warning_msg(DIAGNET_WARN_MANY_CLOSE_WAIT));
    }
    (void)first;
    printf("%s]\n", ctx->global_warnings ? "\n  " : "");

    printf("}\n");
}
