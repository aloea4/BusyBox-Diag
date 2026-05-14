#include "diagfs.h"
#include <stdio.h>
#include <string.h>

static int is_pseudo_fs(const char *fstype)
{
    return (strcmp(fstype, "proc")       == 0 ||
            strcmp(fstype, "sysfs")      == 0 ||
            strcmp(fstype, "devtmpfs")   == 0 ||
            strcmp(fstype, "cgroup")     == 0 ||
            strcmp(fstype, "cgroup2")    == 0 ||
            strcmp(fstype, "devpts")     == 0 ||
            strcmp(fstype, "mqueue")     == 0 ||
            strcmp(fstype, "hugetlbfs")  == 0 ||
            strcmp(fstype, "tmpfs")      == 0);
}

int scan_all_mounts(diagfs_output_format_t out_format,
                    const diagfs_policy_t *policy,
                    diagfs_scan_filter_t filter)
{
    FILE *fp;
    char line[2048]; /* 擴大讀取緩衝區 */
    diagfs_mount_entry_t entry;
    int dump, pass;
    int is_first = 1;

    fp = fopen("/proc/self/mounts", "r");
    if (!fp) {
        fprintf(stderr, "Error: 無法讀取 /proc/self/mounts\n");
        return EXIT_ERR_RUNTIME;
    }

    if (out_format == DIAGFS_OUT_JSON) {
        printf("{\n  \"filesystems\": [\n");
    } else if (out_format == DIAGFS_OUT_TABLE) {
        printf("diagfs - 掃描所有掛載點\n");
        printf("------------------------------------------\n");
    }

    while (fgets(line, sizeof(line), fp)) {
        /* 修復：限制 options 讀取長度為 1023，對齊 struct 設定，防止溢位 */
        if (sscanf(line, "%255s %255s %63s %1023s %d %d",
                   entry.source, entry.target, entry.fstype, entry.options, &dump, &pass) != 6) {
            continue;
        }

        entry.is_pseudo = is_pseudo_fs(entry.fstype);
        entry.is_remote = 0;

        if (filter == DIAGFS_SCAN_REAL && entry.is_pseudo)
            continue;

        if (strncmp(entry.target, "/proc/", 6) == 0 ||
            strncmp(entry.target, "/sys/",  5) == 0)
            continue;

        diag_fs_info_t fs;
        if (diag_fs_read(entry.target, &fs) != DIAG_OK) {
            /* 修復：不再靜默失敗，給除錯者留下一點線索 */
            fprintf(stderr, "Warning: 無法讀取掛載點 %s，已跳過。\n", entry.target);
            continue;
        }

        diag_fiemap_info_t fiemap;
        int ret_fiemap = diag_fs_read_fiemap(entry.target, &fiemap);
        diagfs_layout_analysis_t layout   = analyze_layout(&fiemap, ret_fiemap, policy);
        diagfs_analysis_t        analysis = analyze_filesystem(&fs, policy, &layout);

        if (out_format == DIAGFS_OUT_JSON) {
            if (!is_first) printf(",\n");
            print_out_json(&fs, &analysis, &layout);
            is_first = 0;
        } else if (out_format == DIAGFS_OUT_RAW) {
            print_out_raw(&fs, &analysis, &layout);
        } else {
            print_out_table(&fs, &analysis, &layout);
        }
    }

    if (out_format == DIAGFS_OUT_JSON)
        printf("\n  ]\n}\n");

    fclose(fp);
    return EXIT_OK;
}