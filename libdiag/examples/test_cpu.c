#include "libdiag/diag_common.h"
#include "libdiag/diag_proc.h"

#include <stdio.h>

int main(void)
{
    diag_cpu_snapshot_t cpu;
    unsigned long long total;
    unsigned long long idle;
    int ret;

    ret = diag_cpu_read_snapshot(&cpu);
    if (ret != DIAG_OK) {
        printf("diag_cpu_read_snapshot failed: %s\n", diag_strerror(ret));
        return 1;
    }

    total = diag_cpu_total(&cpu);
    idle = diag_cpu_idle(&cpu);

    printf("CPU snapshot:\n");
    printf("  user    = %llu\n", cpu.user);
    printf("  nice    = %llu\n", cpu.nice);
    printf("  system  = %llu\n", cpu.system);
    printf("  idle    = %llu\n", cpu.idle);
    printf("  iowait  = %llu\n", cpu.iowait);
    printf("  irq     = %llu\n", cpu.irq);
    printf("  softirq = %llu\n", cpu.softirq);
    printf("  steal   = %llu\n", cpu.steal);
    printf("  total   = %llu\n", total);
    printf("  idle+io = %llu\n", idle);

    if (total == 0) {
        printf("test_cpu failed: total is zero\n");
        return 1;
    }

    printf("test_cpu: PASS\n");
    return 0;
}