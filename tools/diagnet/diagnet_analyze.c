#include "diagnet.h"

#include <stdlib.h>
#include <string.h>

static const diagnet_state_t STATE_BY_IDX[DIAGNET_STATE_NUM] = {
    DIAGNET_STATE_NONE,
    DIAGNET_STATE_ESTABLISHED,
    DIAGNET_STATE_SYN_SENT,
    DIAGNET_STATE_SYN_RECV,
    DIAGNET_STATE_FIN_WAIT1,
    DIAGNET_STATE_FIN_WAIT2,
    DIAGNET_STATE_TIME_WAIT,
    DIAGNET_STATE_CLOSE,
    DIAGNET_STATE_CLOSE_WAIT,
    DIAGNET_STATE_LAST_ACK,
    DIAGNET_STATE_LISTEN,
    DIAGNET_STATE_CLOSING,
    DIAGNET_STATE_UNKNOWN
};

int diagnet_state_to_idx(diagnet_state_t s)
{
    int i;
    for (i = 0; i < DIAGNET_STATE_NUM; i++) {
        if (STATE_BY_IDX[i] == s) {
            return i;
        }
    }
    return DIAGNET_STATE_NUM - 1;
}

diagnet_state_t diagnet_state_from_idx(int idx)
{
    if (idx < 0 || idx >= DIAGNET_STATE_NUM) {
        return DIAGNET_STATE_UNKNOWN;
    }
    return STATE_BY_IDX[idx];
}

void diagnet_analyze_account(diagnet_ctx_t *ctx,
                             const diagnet_conn_t *c,
                             unsigned int flags)
{
    if (c->proto == DIAGNET_PROTO_TCP) {
        ctx->tcp_total++;
    } else if (c->proto == DIAGNET_PROTO_UDP) {
        ctx->udp_total++;
    }

    ctx->state_counts[diagnet_state_to_idx(c->state)]++;

    if (flags & DIAGNET_FLAG_SUSPICIOUS_PORT) {
        ctx->suspicious_count++;
    }
}

int diagnet_records_append(diagnet_ctx_t *ctx,
                           const diagnet_conn_t *c,
                           unsigned int flags)
{
    int new_cap;
    diagnet_record_t *p;

    if (ctx->count >= ctx->cap) {
        new_cap = ctx->cap ? ctx->cap * 2 : 64;
        p = (diagnet_record_t *)realloc(ctx->records,
                                        (size_t)new_cap * sizeof(*p));
        if (!p) {
            return -1;
        }
        ctx->records = p;
        ctx->cap = new_cap;
    }

    ctx->records[ctx->count].conn  = *c;
    ctx->records[ctx->count].flags = flags;
    ctx->count++;
    return 0;
}

static int cmp_state(const void *a, const void *b)
{
    const diagnet_record_t *ra = (const diagnet_record_t *)a;
    const diagnet_record_t *rb = (const diagnet_record_t *)b;
    if (ra->conn.state < rb->conn.state) return -1;
    if (ra->conn.state > rb->conn.state) return  1;
    return 0;
}

static int cmp_lport(const void *a, const void *b)
{
    const diagnet_record_t *ra = (const diagnet_record_t *)a;
    const diagnet_record_t *rb = (const diagnet_record_t *)b;
    if (ra->conn.local_port < rb->conn.local_port) return -1;
    if (ra->conn.local_port > rb->conn.local_port) return  1;
    return 0;
}

static int cmp_rport(const void *a, const void *b)
{
    const diagnet_record_t *ra = (const diagnet_record_t *)a;
    const diagnet_record_t *rb = (const diagnet_record_t *)b;
    if (ra->conn.remote_port < rb->conn.remote_port) return -1;
    if (ra->conn.remote_port > rb->conn.remote_port) return  1;
    return 0;
}

void diagnet_records_sort(diagnet_ctx_t *ctx)
{
    if (ctx->count <= 1) {
        return;
    }
    switch (ctx->filter.sort_mode) {
    case DIAGNET_SORT_STATE:
        qsort(ctx->records, (size_t)ctx->count, sizeof(*ctx->records), cmp_state);
        break;
    case DIAGNET_SORT_LOCAL_PORT:
        qsort(ctx->records, (size_t)ctx->count, sizeof(*ctx->records), cmp_lport);
        break;
    case DIAGNET_SORT_REMOTE_PORT:
        qsort(ctx->records, (size_t)ctx->count, sizeof(*ctx->records), cmp_rport);
        break;
    default:
        break;
    }
}
