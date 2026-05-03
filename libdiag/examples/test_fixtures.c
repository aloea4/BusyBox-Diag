#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"
#include "libdiag/diag_net.h"

#include <stdio.h>
#include <string.h>

static int read_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return 1;
    }

    if (fgets(buf, (int)size, fp) == NULL) {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

static int test_proc_stat_fixture(void)
{
    char line[DIAG_LINE_MAX];
    diag_proc_info_t p;
    int ret;

    if (read_first_line("tests/fixtures/proc_pid_stat.txt", line, sizeof(line)) != 0) {
        printf("failed to read proc_pid_stat fixture\n");
        return 1;
    }

    ret = diag_proc_parse_stat_line(line, &p);
    if (ret != DIAG_OK) {
        printf("diag_proc_parse_stat_line failed: %s\n", diag_strerror(ret));
        return 1;
    }

    if (strcmp(p.comm, "my test proc") != 0) {
        printf("comm mismatch: %s\n", p.comm);
        return 1;
    }

    if (p.state != 'S') {
        printf("state mismatch: %c\n", p.state);
        return 1;
    }

    if (p.ppid != 1) {
        printf("ppid mismatch: %d\n", p.ppid);
        return 1;
    }

    if (p.utime != 60UL || p.stime != 70UL) {
        printf("cpu tick mismatch: utime=%lu stime=%lu\n", p.utime, p.stime);
        return 1;
    }

    printf("proc stat fixture: PASS\n");
    return 0;
}

static int test_net_tcp_fixture(void)
{
    char line[DIAG_LINE_MAX];
    diag_net_conn_t c;
    int ret;

    if (read_first_line("tests/fixtures/proc_net_tcp.txt", line, sizeof(line)) != 0) {
        printf("failed to read proc_net_tcp fixture\n");
        return 1;
    }

    ret = diag_net_parse_tcp_line(line, &c);
    if (ret != DIAG_OK) {
        printf("diag_net_parse_tcp_line failed: %s\n", diag_strerror(ret));
        return 1;
    }

    if (strcmp(c.proto, "tcp") != 0) {
        printf("proto mismatch: %s\n", c.proto);
        return 1;
    }

    if (strcmp(c.local_addr, "127.0.0.1") != 0 || c.local_port != 22U) {
        printf("local mismatch: %s:%u\n", c.local_addr, c.local_port);
        return 1;
    }

    if (strcmp(c.remote_addr, "0.0.0.0") != 0 || c.remote_port != 0U) {
        printf("remote mismatch: %s:%u\n", c.remote_addr, c.remote_port);
        return 1;
    }

    if (strcmp(c.state, "LISTEN") != 0) {
        printf("state mismatch: %s\n", c.state);
        return 1;
    }

    printf("net tcp fixture: PASS\n");
    return 0;
}

static int test_malformed_input(void)
{
    diag_proc_info_t p;
    diag_net_conn_t c;

    if (diag_proc_parse_stat_line("bad line", &p) != DIAG_ERR_PARSE) {
        printf("malformed proc stat should fail\n");
        return 1;
    }

    if (diag_net_parse_tcp_line("bad line", &c) != DIAG_ERR_PARSE) {
        printf("malformed net tcp should fail\n");
        return 1;
    }

    printf("malformed fixtures: PASS\n");
    return 0;
}

int main(void)
{
    if (test_proc_stat_fixture() != 0) {
        return 1;
    }

    if (test_net_tcp_fixture() != 0) {
        return 1;
    }

    if (test_malformed_input() != 0) {
        return 1;
    }

    printf("test_fixtures: PASS\n");
    return 0;
}