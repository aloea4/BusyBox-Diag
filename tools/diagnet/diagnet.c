#include "libdiag/diag_common.h"
#include "libdiag/diag_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>

static const unsigned int WHITELIST_DEFAULT[] = {
    20, 21, 22, 23, 25, 53, 67, 68, 69, 80, 110,
    123, 143, 161, 162, 389, 443, 465, 514, 587,
    636, 993, 995, 3306, 5432, 6379, 27017
};
#define WHITELIST_DEFAULT_LEN \
    (sizeof(WHITELIST_DEFAULT) / sizeof(WHITELIST_DEFAULT[0]))

static const char *const STATE_NAMES[] = {
    "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1",
    "FIN_WAIT2", "TIME_WAIT", "CLOSE", "CLOSE_WAIT",
    "LAST_ACK", "LISTEN", "CLOSING", "UNKNOWN"
};
#define STATE_NAMES_LEN (sizeof(STATE_NAMES) / sizeof(STATE_NAMES[0]))

struct conn_record {
    diag_net_conn_t conn;
    int suspicious;
};

struct diagnet_ctx {
    int proto;                  /* 0=all, 1=tcp, 2=udp */
    const char *state_filter;
    int suspicious_only;

    unsigned int *wl_extra;
    int wl_extra_len;

    int output_json;
    int stats_only;

    struct conn_record *entries;
    int count;
    int cap;

    int state_counts[STATE_NAMES_LEN];
    int suspicious_count;
};

static int is_suspicious_port(unsigned int port, const struct diagnet_ctx *ctx)
{
    size_t i;
    int j;

    for (i = 0; i < WHITELIST_DEFAULT_LEN; i++) {
        if (WHITELIST_DEFAULT[i] == port) {
            return 0;
        }
    }
    for (j = 0; j < ctx->wl_extra_len; j++) {
        if (ctx->wl_extra[j] == port) {
            return 0;
        }
    }
    return 1;
}

static int state_to_idx(const char *state)
{
    size_t i;

    for (i = 0; i < STATE_NAMES_LEN; i++) {
        if (strcmp(state, STATE_NAMES[i]) == 0) {
            return (int)i;
        }
    }
    return (int)STATE_NAMES_LEN - 1; /* UNKNOWN */
}

static int ensure_capacity(struct diagnet_ctx *ctx)
{
    int new_cap;
    struct conn_record *p;

    if (ctx->count < ctx->cap) {
        return 0;
    }
    new_cap = ctx->cap ? ctx->cap * 2 : 64;
    p = (struct conn_record *)realloc(ctx->entries,
                                      (size_t)new_cap * sizeof(*p));
    if (!p) {
        return -1;
    }
    ctx->entries = p;
    ctx->cap = new_cap;
    return 0;
}

static int collect_conn(const diag_net_conn_t *c, void *user)
{
    struct diagnet_ctx *ctx = (struct diagnet_ctx *)user;
    struct conn_record *rec;
    int suspicious;

    if (ctx->proto == 1 && strcmp(c->proto, "tcp") != 0) {
        return 0;
    }
    if (ctx->proto == 2 && strcmp(c->proto, "udp") != 0) {
        return 0;
    }
    if (ctx->state_filter && strcmp(c->state, ctx->state_filter) != 0) {
        return 0;
    }

    suspicious = 0;
    if (strcmp(c->state, "LISTEN") == 0
        && is_suspicious_port(c->local_port, ctx)) {
        suspicious = 1;
    }

    if (ctx->suspicious_only && !suspicious) {
        return 0;
    }

    if (ensure_capacity(ctx) != 0) {
        return -1;
    }

    rec = &ctx->entries[ctx->count];
    rec->conn = *c;
    rec->suspicious = suspicious;

    ctx->state_counts[state_to_idx(c->state)]++;
    if (suspicious) {
        ctx->suspicious_count++;
    }
    ctx->count++;
    return 0;
}

static int parse_whitelist(const char *s, struct diagnet_ctx *ctx)
{
    const char *p = s;
    char buf[16];
    size_t blen;
    char *endp;
    unsigned long val;
    unsigned int *arr = NULL;
    int cap = 0;
    int len = 0;
    unsigned int *grown;

    while (*p) {
        blen = 0;
        while (*p && *p != ',') {
            if (blen >= sizeof(buf) - 1) {
                fprintf(stderr, "diagnet: whitelist token too long\n");
                free(arr);
                return -1;
            }
            buf[blen++] = *p++;
        }
        buf[blen] = '\0';
        if (*p == ',') {
            p++;
        }
        if (blen == 0) {
            continue;
        }

        errno = 0;
        val = strtoul(buf, &endp, 10);
        if (errno != 0 || *endp != '\0' || val == 0 || val > 65535) {
            fprintf(stderr, "diagnet: invalid whitelist port: %s\n", buf);
            free(arr);
            return -1;
        }

        if (len >= cap) {
            cap = cap ? cap * 2 : 8;
            grown = (unsigned int *)realloc(arr,
                                            (size_t)cap * sizeof(*arr));
            if (!grown) {
                fprintf(stderr, "diagnet: out of memory\n");
                free(arr);
                return -1;
            }
            arr = grown;
        }
        arr[len++] = (unsigned int)val;
    }

    free(ctx->wl_extra);
    ctx->wl_extra = arr;
    ctx->wl_extra_len = len;
    return 0;
}

static void print_table(const struct diagnet_ctx *ctx)
{
    int i;
    const struct conn_record *r;

    printf("%-5s %-16s %-6s %-16s %-6s %-12s\n",
           "Proto", "Local Address", "Port",
           "Remote Address", "Port", "State");

    for (i = 0; i < ctx->count; i++) {
        r = &ctx->entries[i];
        printf("%-5s %-16s %-6u %-16s %-6u %-12s%s\n",
               r->conn.proto,
               r->conn.local_addr, r->conn.local_port,
               r->conn.remote_addr, r->conn.remote_port,
               r->conn.state,
               r->suspicious ? "  [WARNING]" : "");
    }
}

static void print_stats(const struct diagnet_ctx *ctx)
{
    size_t i;

    printf("Total connections: %d\n", ctx->count);
    printf("Suspicious ports:  %d\n", ctx->suspicious_count);
    printf("State distribution:\n");
    for (i = 0; i < STATE_NAMES_LEN; i++) {
        if (ctx->state_counts[i] > 0) {
            printf("  %-12s %d\n", STATE_NAMES[i], ctx->state_counts[i]);
        }
    }
}

static void print_json(const struct diagnet_ctx *ctx)
{
    int i;
    const struct conn_record *r;
    int first;
    size_t s;

    printf("{\n");

    /* connections */
    printf("  \"connections\": [");
    first = 1;
    for (i = 0; i < ctx->count; i++) {
        r = &ctx->entries[i];
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("\n    {");
        printf("\"proto\": \"%s\", ", r->conn.proto);
        printf("\"local_addr\": \"%s\", ", r->conn.local_addr);
        printf("\"local_port\": %u, ", r->conn.local_port);
        printf("\"remote_addr\": \"%s\", ", r->conn.remote_addr);
        printf("\"remote_port\": %u, ", r->conn.remote_port);
        printf("\"state\": \"%s\", ", r->conn.state);
        printf("\"suspicious\": %s", r->suspicious ? "true" : "false");
        printf("}");
    }
    printf("%s],\n", ctx->count ? "\n  " : "");

    /* summary */
    printf("  \"summary\": {\n");
    printf("    \"total\": %d", ctx->count);
    for (s = 0; s < STATE_NAMES_LEN; s++) {
        printf(",\n    \"%s\": %d", STATE_NAMES[s], ctx->state_counts[s]);
    }
    printf("\n  },\n");

    /* suspicious_ports */
    printf("  \"suspicious_ports\": [");
    first = 1;
    for (i = 0; i < ctx->count; i++) {
        r = &ctx->entries[i];
        if (!r->suspicious) {
            continue;
        }
        if (!first) {
            printf(",");
        }
        first = 0;
        printf("\n    {");
        printf("\"proto\": \"%s\", ", r->conn.proto);
        printf("\"local_addr\": \"%s\", ", r->conn.local_addr);
        printf("\"local_port\": %u, ", r->conn.local_port);
        printf("\"state\": \"%s\"", r->conn.state);
        printf("}");
    }
    printf("%s]\n", ctx->suspicious_count ? "\n  " : "");

    printf("}\n");
}

static void print_help(void)
{
    printf("Usage: diagnet [OPTIONS]\n");
    printf("\n");
    printf("Show TCP/UDP connections from /proc/net/{tcp,udp}.\n");
    printf("\n");
    printf("Options:\n");
    printf("  --proto PROTO       Filter by protocol: tcp, udp, all (default all)\n");
    printf("  --state STATE       Filter by connection state (e.g. ESTABLISHED, LISTEN)\n");
    printf("  --stats             Show only state-distribution summary\n");
    printf("  --suspicious        Show only suspicious listening ports\n");
    printf("  --whitelist PORTS   Comma-separated extra whitelist ports (e.g. 8888,9090)\n");
    printf("  --output FORMAT     Output format: table (default) or json\n");
    printf("  -h, --help          Show this help\n");
}

int diagnet_main(int argc, char *argv[])
{
    struct diagnet_ctx ctx;
    int ret;
    int rc = 0;

    enum {
        OPT_PROTO = 256,
        OPT_STATE,
        OPT_STATS,
        OPT_SUSPICIOUS,
        OPT_WHITELIST,
        OPT_OUTPUT
    };

    static struct option long_opts[] = {
        { "proto",      required_argument, NULL, OPT_PROTO },
        { "state",      required_argument, NULL, OPT_STATE },
        { "stats",      no_argument,       NULL, OPT_STATS },
        { "suspicious", no_argument,       NULL, OPT_SUSPICIOUS },
        { "whitelist",  required_argument, NULL, OPT_WHITELIST },
        { "output",     required_argument, NULL, OPT_OUTPUT },
        { "help",       no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    memset(&ctx, 0, sizeof(ctx));

    optind = 1;
    for (;;) {
        int opt = getopt_long(argc, argv, "h", long_opts, NULL);
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case OPT_PROTO:
            if (strcmp(optarg, "tcp") == 0) {
                ctx.proto = 1;
            } else if (strcmp(optarg, "udp") == 0) {
                ctx.proto = 2;
            } else if (strcmp(optarg, "all") == 0) {
                ctx.proto = 0;
            } else {
                fprintf(stderr,
                        "diagnet: invalid --proto value: %s "
                        "(expected tcp, udp, all)\n", optarg);
                rc = 1;
                goto out;
            }
            break;
        case OPT_STATE:
            ctx.state_filter = optarg;
            break;
        case OPT_STATS:
            ctx.stats_only = 1;
            break;
        case OPT_SUSPICIOUS:
            ctx.suspicious_only = 1;
            break;
        case OPT_WHITELIST:
            if (parse_whitelist(optarg, &ctx) != 0) {
                rc = 1;
                goto out;
            }
            break;
        case OPT_OUTPUT:
            if (strcmp(optarg, "table") == 0) {
                ctx.output_json = 0;
            } else if (strcmp(optarg, "json") == 0) {
                ctx.output_json = 1;
            } else {
                fprintf(stderr,
                        "diagnet: invalid --output value: %s "
                        "(expected table or json)\n", optarg);
                rc = 1;
                goto out;
            }
            break;
        case 'h':
            print_help();
            goto out;
        default:
            rc = 1;
            goto out;
        }
    }

    if (ctx.proto == 0 || ctx.proto == 1) {
        ret = diag_net_foreach_tcp(collect_conn, &ctx);
        if (ret != DIAG_OK) {
            fprintf(stderr, "diagnet: read /proc/net/tcp: %s\n",
                    diag_strerror(ret));
            rc = 1;
            goto out;
        }
    }
    if (ctx.proto == 0 || ctx.proto == 2) {
        ret = diag_net_foreach_udp(collect_conn, &ctx);
        if (ret != DIAG_OK) {
            fprintf(stderr, "diagnet: read /proc/net/udp: %s\n",
                    diag_strerror(ret));
            rc = 1;
            goto out;
        }
    }

    if (ctx.output_json) {
        print_json(&ctx);
    } else if (ctx.stats_only) {
        print_stats(&ctx);
    } else {
        print_table(&ctx);
    }

out:
    free(ctx.entries);
    free(ctx.wl_extra);
    return rc;
}

#ifndef BUSYBOX_APPLET
int main(int argc, char *argv[])
{
    return diagnet_main(argc, argv);
}
#endif
