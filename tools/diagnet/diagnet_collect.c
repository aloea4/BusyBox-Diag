#include "diagnet.h"
#include "libdiag/diag_common.h"

#include <stdio.h>

static int collect_cb(const diag_net_conn_t *raw, void *user)
{
    diagnet_ctx_t *ctx = (diagnet_ctx_t *)user;
    diagnet_conn_t c;
    unsigned int flags;

    diagnet_conn_from_libdiag(raw, &c);

    if (!diagnet_filter_match(&ctx->filter, &c)) {
        return 0;
    }

    flags = diagnet_policy_flags_for(&c, ctx->wl_extra, ctx->wl_extra_len);

    if (ctx->filter.suspicious_only
        && !(flags & DIAGNET_FLAG_SUSPICIOUS_PORT)) {
        return 0;
    }

    diagnet_analyze_account(ctx, &c, flags);

    if (!ctx->stats_only) {
        if (diagnet_records_append(ctx, &c, flags) != 0) {
            return -1;
        }
    }

    return 0;
}

int diagnet_collect_run(diagnet_ctx_t *ctx)
{
    int ret;
    int want_tcp;
    int want_udp;

    want_tcp = !ctx->filter.proto_set || ctx->filter.proto == DIAGNET_PROTO_TCP;
    want_udp = !ctx->filter.proto_set || ctx->filter.proto == DIAGNET_PROTO_UDP;

    if (want_tcp) {
        ret = diag_net_foreach_tcp(collect_cb, ctx);
        if (ret != DIAG_OK) {
            fprintf(stderr, "diagnet: read /proc/net/tcp: %s\n",
                    diag_strerror(ret));
            return -1;
        }
    }
    if (want_udp) {
        ret = diag_net_foreach_udp(collect_cb, ctx);
        if (ret != DIAG_OK) {
            fprintf(stderr, "diagnet: read /proc/net/udp: %s\n",
                    diag_strerror(ret));
            return -1;
        }
    }

    diagnet_policy_update_global_warnings(ctx);
    diagnet_records_sort(ctx);
    return 0;
}
