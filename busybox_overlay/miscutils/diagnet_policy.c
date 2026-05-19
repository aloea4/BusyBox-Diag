#include "diagnet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const unsigned int WHITELIST_DEFAULT[] = {
    20, 21, 22, 23, 25, 53, 67, 68, 69, 80, 110,
    123, 143, 161, 162, 389, 443, 465, 514, 587,
    636, 993, 995, 3306, 5432, 6379, 27017
};
#define WHITELIST_DEFAULT_LEN \
    (sizeof(WHITELIST_DEFAULT) / sizeof(WHITELIST_DEFAULT[0]))

int diagnet_policy_is_whitelisted(unsigned int port,
                                  const unsigned int *extra,
                                  int extra_len)
{
    size_t i;
    int j;

    for (i = 0; i < WHITELIST_DEFAULT_LEN; i++) {
        if (WHITELIST_DEFAULT[i] == port) {
            return 1;
        }
    }
    for (j = 0; j < extra_len; j++) {
        if (extra[j] == port) {
            return 1;
        }
    }
    return 0;
}

unsigned int diagnet_policy_flags_for(const diagnet_conn_t *c,
                                      const unsigned int *extra,
                                      int extra_len)
{
    unsigned int flags = 0;

    if (c->state == DIAGNET_STATE_LISTEN) {
        if (!diagnet_policy_is_whitelisted(c->local_port, extra, extra_len)) {
            flags |= DIAGNET_FLAG_SUSPICIOUS_PORT;
        }
        if (strcmp(c->local_addr, "0.0.0.0") == 0) {
            flags |= DIAGNET_FLAG_PUBLIC_LISTEN;
        }
    }

    return flags;
}

int diagnet_policy_parse_whitelist(const char *s,
                                   unsigned int **out_arr,
                                   int *out_len)
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

    *out_arr = NULL;
    *out_len = 0;

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

    *out_arr = arr;
    *out_len = len;
    return 0;
}

void diagnet_policy_update_global_warnings(diagnet_ctx_t *ctx)
{
    int tw_idx = diagnet_state_to_idx(DIAGNET_STATE_TIME_WAIT);
    int cw_idx = diagnet_state_to_idx(DIAGNET_STATE_CLOSE_WAIT);

    if (ctx->state_counts[tw_idx] >= DIAGNET_MANY_TIME_WAIT_THRESHOLD) {
        ctx->global_warnings |= DIAGNET_WARN_MANY_TIME_WAIT;
    }
    if (ctx->state_counts[cw_idx] >= DIAGNET_MANY_CLOSE_WAIT_THRESHOLD) {
        ctx->global_warnings |= DIAGNET_WARN_MANY_CLOSE_WAIT;
    }
}
