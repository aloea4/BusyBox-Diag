#ifndef DIAGNET_H
#define DIAGNET_H

#include "libdiag/diag_net.h"

typedef enum {
    DIAGNET_PROTO_UNKNOWN = 0,
    DIAGNET_PROTO_TCP     = 1,
    DIAGNET_PROTO_UDP     = 2
} diagnet_proto_t;

typedef enum {
    DIAGNET_STATE_NONE        = 0x00,
    DIAGNET_STATE_ESTABLISHED = 0x01,
    DIAGNET_STATE_SYN_SENT    = 0x02,
    DIAGNET_STATE_SYN_RECV    = 0x03,
    DIAGNET_STATE_FIN_WAIT1   = 0x04,
    DIAGNET_STATE_FIN_WAIT2   = 0x05,
    DIAGNET_STATE_TIME_WAIT   = 0x06,
    DIAGNET_STATE_CLOSE       = 0x07,
    DIAGNET_STATE_CLOSE_WAIT  = 0x08,
    DIAGNET_STATE_LAST_ACK    = 0x09,
    DIAGNET_STATE_LISTEN      = 0x0A,
    DIAGNET_STATE_CLOSING     = 0x0B,
    DIAGNET_STATE_UNKNOWN     = 0xFF
} diagnet_state_t;

typedef struct {
    diagnet_proto_t  proto;
    char             local_addr[64];
    unsigned int     local_port;
    char             remote_addr[64];
    unsigned int     remote_port;
    diagnet_state_t  state;
} diagnet_conn_t;

#define DIAGNET_STATE_NUM 13   /* number of diagnet_state_t values tracked */

#define DIAGNET_FLAG_SUSPICIOUS_PORT   0x01u
#define DIAGNET_FLAG_PUBLIC_LISTEN     0x02u

#define DIAGNET_WARN_MANY_TIME_WAIT    0x01u
#define DIAGNET_WARN_MANY_CLOSE_WAIT   0x02u

#define DIAGNET_MANY_TIME_WAIT_THRESHOLD   50
#define DIAGNET_MANY_CLOSE_WAIT_THRESHOLD  50

#define DIAGNET_OUTPUT_TABLE 0
#define DIAGNET_OUTPUT_RAW   1
#define DIAGNET_OUTPUT_JSON  2

#define DIAGNET_SORT_NONE        0
#define DIAGNET_SORT_STATE       1
#define DIAGNET_SORT_LOCAL_PORT  2
#define DIAGNET_SORT_REMOTE_PORT 3

typedef struct {
    int               proto_set;
    diagnet_proto_t   proto;

    int               state_set;
    diagnet_state_t   state;

    int               listen_only;

    int               lport_set;
    unsigned int      lport;

    int               rport_set;
    unsigned int      rport;

    const char       *laddr;
    const char       *raddr;

    int               suspicious_only;

    int               sort_mode;
} diagnet_filter_t;

typedef struct {
    diagnet_conn_t  conn;
    unsigned int    flags;
} diagnet_record_t;

typedef struct {
    diagnet_filter_t  filter;

    unsigned int     *wl_extra;
    int               wl_extra_len;

    int               output_mode;
    int               stats_only;
    int               no_header;
    int               quiet;

    diagnet_record_t *records;
    int               count;
    int               cap;

    int               tcp_total;
    int               udp_total;
    int               state_counts[DIAGNET_STATE_NUM];
    int               suspicious_count;

    unsigned int      global_warnings;
} diagnet_ctx_t;

/* --- state (diagnet_state.c) --------------------------------------------- */

diagnet_proto_t  diagnet_proto_from_libdiag(const char *s);
const char      *diagnet_proto_name(diagnet_proto_t p);

diagnet_state_t  diagnet_state_from_libdiag(const char *s);
diagnet_state_t  diagnet_state_from_name(const char *name);
const char      *diagnet_state_name(diagnet_state_t s);

void diagnet_conn_from_libdiag(const diag_net_conn_t *src,
                               diagnet_conn_t *dst);

/* --- filter (diagnet_filter.c) ------------------------------------------- */

int diagnet_filter_match(const diagnet_filter_t *f, const diagnet_conn_t *c);

/* --- policy (diagnet_policy.c) ------------------------------------------- */

int  diagnet_policy_is_whitelisted(unsigned int port,
                                   const unsigned int *extra,
                                   int extra_len);
unsigned int diagnet_policy_flags_for(const diagnet_conn_t *c,
                                      const unsigned int *extra,
                                      int extra_len);
int  diagnet_policy_parse_whitelist(const char *s,
                                    unsigned int **out_arr,
                                    int *out_len);
void diagnet_policy_update_global_warnings(diagnet_ctx_t *ctx);

/* --- analyze (diagnet_analyze.c) ----------------------------------------- */

int  diagnet_state_to_idx(diagnet_state_t s);
diagnet_state_t diagnet_state_from_idx(int idx);
void diagnet_analyze_account(diagnet_ctx_t *ctx,
                             const diagnet_conn_t *c,
                             unsigned int flags);
int  diagnet_records_append(diagnet_ctx_t *ctx,
                            const diagnet_conn_t *c,
                            unsigned int flags);
void diagnet_records_sort(diagnet_ctx_t *ctx);

/* --- collect (diagnet_collect.c) ----------------------------------------- */

int diagnet_collect_run(diagnet_ctx_t *ctx);

/* --- output (diagnet_output.c) ------------------------------------------- */

void diagnet_output_table(const diagnet_ctx_t *ctx);
void diagnet_output_raw(const diagnet_ctx_t *ctx);
void diagnet_output_json(const diagnet_ctx_t *ctx);
void diagnet_output_stats(const diagnet_ctx_t *ctx);
void diagnet_output_warnings_stderr(const diagnet_ctx_t *ctx);

#endif
