#include "diagnet.h"
#include "libdiag/diag_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#define USAGE_ERR 2

static int parse_port(const char *s, unsigned int *out)
{
    char *endp;
    unsigned long v;

    errno = 0;
    v = strtoul(s, &endp, 10);
    if (errno != 0 || *endp != '\0' || v == 0 || v > 65535) {
        return -1;
    }
    *out = (unsigned int)v;
    return 0;
}

static int parse_sort_mode(const char *s, int *out)
{
    if (strcmp(s, "state")        == 0) { *out = DIAGNET_SORT_STATE;       return 0; }
    if (strcmp(s, "local-port")   == 0) { *out = DIAGNET_SORT_LOCAL_PORT;  return 0; }
    if (strcmp(s, "remote-port")  == 0) { *out = DIAGNET_SORT_REMOTE_PORT; return 0; }
    return -1;
}

static void print_help(void)
{
    printf("Usage: diagnet [OPTIONS]\n");
    printf("\n");
    printf("Procfs-based Linux network connection monitoring tool.\n");
    printf("Passively reads TCP/UDP connection metadata from /proc/net/{tcp,udp}.\n");
    printf("\n");
    printf("Filtering:\n");
    printf("  --proto PROTO        Filter by protocol: tcp, udp, all (default all)\n");
    printf("  --state STATE        Filter by TCP state (e.g. ESTABLISHED, LISTEN)\n");
    printf("  --listen             Shortcut for --state LISTEN\n");
    printf("  --local-port PORT    Filter by local port\n");
    printf("  --remote-port PORT   Filter by remote port\n");
    printf("  --local-addr ADDR    Filter by local address\n");
    printf("  --remote-addr ADDR   Filter by remote address\n");
    printf("  --suspicious         Show only suspicious listening ports\n");
    printf("  --whitelist PORTS    Comma-separated extra whitelist ports\n");
    printf("\n");
    printf("Output:\n");
    printf("  --output FORMAT      table (default), raw, or json\n");
    printf("  --sort MODE          state, local-port, or remote-port\n");
    printf("  --no-header          Suppress table/raw header row\n");
    printf("  --stats              Show only state-distribution summary\n");
    printf("  --quiet              Suppress global warnings on stderr\n");
    printf("  -h, --help           Show this help\n");
    printf("\n");
    printf("Exit codes: 0=ok, 1=runtime error, 2=usage error, 3=unsupported\n");
}

int diagnet_main(int argc, char *argv[])
{
    diagnet_ctx_t ctx;
    int rc = 0;
    unsigned int port_v;
    int sort_v;
    diagnet_state_t st_v;

    enum {
        OPT_PROTO = 256,
        OPT_STATE,
        OPT_LISTEN,
        OPT_LOCAL_PORT,
        OPT_REMOTE_PORT,
        OPT_LOCAL_ADDR,
        OPT_REMOTE_ADDR,
        OPT_STATS,
        OPT_SUSPICIOUS,
        OPT_WHITELIST,
        OPT_SORT,
        OPT_OUTPUT,
        OPT_NO_HEADER,
        OPT_QUIET
    };

    static struct option long_opts[] = {
        { "proto",       required_argument, NULL, OPT_PROTO        },
        { "state",       required_argument, NULL, OPT_STATE        },
        { "listen",      no_argument,       NULL, OPT_LISTEN       },
        { "local-port",  required_argument, NULL, OPT_LOCAL_PORT   },
        { "remote-port", required_argument, NULL, OPT_REMOTE_PORT  },
        { "local-addr",  required_argument, NULL, OPT_LOCAL_ADDR   },
        { "remote-addr", required_argument, NULL, OPT_REMOTE_ADDR  },
        { "stats",       no_argument,       NULL, OPT_STATS        },
        { "suspicious",  no_argument,       NULL, OPT_SUSPICIOUS   },
        { "whitelist",   required_argument, NULL, OPT_WHITELIST    },
        { "sort",        required_argument, NULL, OPT_SORT         },
        { "output",      required_argument, NULL, OPT_OUTPUT       },
        { "no-header",   no_argument,       NULL, OPT_NO_HEADER    },
        { "quiet",       no_argument,       NULL, OPT_QUIET        },
        { "help",        no_argument,       NULL, 'h'              },
        { NULL, 0, NULL, 0 }
    };

    memset(&ctx, 0, sizeof(ctx));
    ctx.output_mode = DIAGNET_OUTPUT_TABLE;

    optind = 1;
    for (;;) {
        int opt = getopt_long(argc, argv, "h", long_opts, NULL);
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case OPT_PROTO:
            if (strcmp(optarg, "tcp") == 0) {
                ctx.filter.proto_set = 1;
                ctx.filter.proto = DIAGNET_PROTO_TCP;
            } else if (strcmp(optarg, "udp") == 0) {
                ctx.filter.proto_set = 1;
                ctx.filter.proto = DIAGNET_PROTO_UDP;
            } else if (strcmp(optarg, "all") == 0) {
                ctx.filter.proto_set = 0;
            } else {
                fprintf(stderr, "diagnet: invalid --proto value: %s "
                                "(expected tcp, udp, all)\n", optarg);
                rc = USAGE_ERR;
                goto out;
            }
            break;
        case OPT_STATE:
            st_v = diagnet_state_from_name(optarg);
            if (st_v == DIAGNET_STATE_UNKNOWN) {
                fprintf(stderr, "diagnet: invalid --state value: %s\n", optarg);
                rc = USAGE_ERR;
                goto out;
            }
            if (ctx.filter.listen_only) {
                fprintf(stderr, "diagnet: --state and --listen are mutually exclusive\n");
                rc = USAGE_ERR;
                goto out;
            }
            ctx.filter.state_set = 1;
            ctx.filter.state = st_v;
            break;
        case OPT_LISTEN:
            if (ctx.filter.state_set) {
                fprintf(stderr, "diagnet: --state and --listen are mutually exclusive\n");
                rc = USAGE_ERR;
                goto out;
            }
            ctx.filter.listen_only = 1;
            break;
        case OPT_LOCAL_PORT:
            if (parse_port(optarg, &port_v) != 0) {
                fprintf(stderr, "diagnet: invalid --local-port value: %s\n", optarg);
                rc = USAGE_ERR;
                goto out;
            }
            ctx.filter.lport_set = 1;
            ctx.filter.lport = port_v;
            break;
        case OPT_REMOTE_PORT:
            if (parse_port(optarg, &port_v) != 0) {
                fprintf(stderr, "diagnet: invalid --remote-port value: %s\n", optarg);
                rc = USAGE_ERR;
                goto out;
            }
            ctx.filter.rport_set = 1;
            ctx.filter.rport = port_v;
            break;
        case OPT_LOCAL_ADDR:
            ctx.filter.laddr = optarg;
            break;
        case OPT_REMOTE_ADDR:
            ctx.filter.raddr = optarg;
            break;
        case OPT_STATS:
            ctx.stats_only = 1;
            break;
        case OPT_SUSPICIOUS:
            ctx.filter.suspicious_only = 1;
            break;
        case OPT_WHITELIST:
            free(ctx.wl_extra);
            if (diagnet_policy_parse_whitelist(optarg,
                                               &ctx.wl_extra,
                                               &ctx.wl_extra_len) != 0) {
                rc = USAGE_ERR;
                goto out;
            }
            break;
        case OPT_SORT:
            if (parse_sort_mode(optarg, &sort_v) != 0) {
                fprintf(stderr, "diagnet: invalid --sort value: %s "
                                "(expected state, local-port, remote-port)\n",
                        optarg);
                rc = USAGE_ERR;
                goto out;
            }
            ctx.filter.sort_mode = sort_v;
            break;
        case OPT_OUTPUT:
            if (strcmp(optarg, "table") == 0) {
                ctx.output_mode = DIAGNET_OUTPUT_TABLE;
            } else if (strcmp(optarg, "raw") == 0) {
                ctx.output_mode = DIAGNET_OUTPUT_RAW;
            } else if (strcmp(optarg, "json") == 0) {
                ctx.output_mode = DIAGNET_OUTPUT_JSON;
            } else {
                fprintf(stderr, "diagnet: invalid --output value: %s "
                                "(expected table, raw, or json)\n", optarg);
                rc = USAGE_ERR;
                goto out;
            }
            break;
        case OPT_NO_HEADER:
            ctx.no_header = 1;
            break;
        case OPT_QUIET:
            ctx.quiet = 1;
            break;
        case 'h':
            print_help();
            goto out;
        default:
            rc = USAGE_ERR;
            goto out;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "diagnet: unexpected argument: %s\n", argv[optind]);
        rc = USAGE_ERR;
        goto out;
    }

    if (diagnet_collect_run(&ctx) != 0) {
        rc = 1;
        goto out;
    }

    if (ctx.stats_only) {
        if (ctx.output_mode == DIAGNET_OUTPUT_JSON) {
            diagnet_output_json(&ctx);
        } else {
            diagnet_output_stats(&ctx);
        }
    } else {
        switch (ctx.output_mode) {
        case DIAGNET_OUTPUT_RAW:
            diagnet_output_raw(&ctx);
            break;
        case DIAGNET_OUTPUT_JSON:
            diagnet_output_json(&ctx);
            break;
        case DIAGNET_OUTPUT_TABLE:
        default:
            diagnet_output_table(&ctx);
            break;
        }
    }

    diagnet_output_warnings_stderr(&ctx);

out:
    free(ctx.records);
    free(ctx.wl_extra);
    return rc;
}

#ifndef BUSYBOX_APPLET
int main(int argc, char *argv[])
{
    return diagnet_main(argc, argv);
}
#endif
