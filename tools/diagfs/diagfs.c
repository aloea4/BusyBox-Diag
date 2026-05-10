#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>
#include <string.h>

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"

/* 計算空間使用百分比 */
static unsigned long calc_used_percent(const diag_fs_info_t *fs)
{
    if (fs->total_kb == 0)
        return 0;
    return (fs->used_kb * 100UL) / fs->total_kb;
}

/* 計算 inode 使用百分比 */
static unsigned long calc_inode_used_percent(const diag_fs_info_t *fs)
{
    unsigned long used;
    if (fs->files_total == 0)
        return 0;
    if (fs->files_total < fs->files_free)
        return 0;
    used = fs->files_total - fs->files_free;
    return (used * 100UL + fs->files_total - 1) / fs->files_total;
}

/* 印出分隔線 */
static void print_separator(void)
{
    printf("------------------------------------------\n");
}


static void scan_all_mounts(void)
{
    FILE *fp;
    char line[256];
    char dev[128], path[128], fstype[64], opts[128];
    int dump, pass;

    fp = fopen("/proc/mounts", "r");
    if (!fp) {
        printf("無法讀取 /proc/mounts\n");
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%s %s %s %s %d %d",
               dev, path, fstype, opts, &dump, &pass);

        /* 跳過虛擬檔案系統 */
        if (strcmp(fstype, "proc") == 0 ||
            strcmp(fstype, "sysfs") == 0 ||
            strcmp(fstype, "devtmpfs") == 0 ||
            strcmp(fstype, "cgroup") == 0 ||
            strcmp(fstype, "cgroup2") == 0 ||
            strcmp(fstype, "devpts") == 0 ||
            strcmp(fstype, "mqueue") == 0 ||
            strcmp(fstype, "hugetlbfs") == 0 ||
            strcmp(fstype, "tmpfs") == 0)
            continue;

        /* 跳過檔案路徑（不是目錄的掛載點）*/
        if (strncmp(path, "/proc/", 6) == 0 ||
            strncmp(path, "/sys/", 5) == 0 ||
            strncmp(path, "/etc/", 5) == 0 ||
            strncmp(path, "/home/", 6) == 0)
            continue;

        diag_fs_info_t fs;
        int ret = diag_fs_read(path, &fs);
        if (ret != DIAG_OK)
            continue;

        printf("\n路徑        : %s\n", fs.path);
        printf("檔案系統類型: %s\n", diag_fs_type_name(fs.fs_type));
        printf("[空間使用] 已使用 %lu KB / 總計 %lu KB (%lu%%)\n",
               fs.used_kb, fs.total_kb, calc_used_percent(&fs));
        printf("[inode 使用] %lu%%\n", calc_inode_used_percent(&fs));
        print_separator();
    }

    fclose(fp);
}

int main(int argc, char **argv)
{
    const char *path = "/";   /* 預設掃描根目錄 */
    diag_fs_info_t fs;
    diag_fiemap_info_t fiemap;
    int ret;

    /* 讓使用者可以指定路徑，例如：diagfs /home */
    if (argc >= 2) {
        path = argv[1];
    }

    printf("diagfs - Filesystem Health Checker\n");
    print_separator();

    // 讓使用者可以選擇掃描所有掛載點
    if (argc >= 2 && strcmp(argv[1], "--all") == 0) {
        printf("diagfs - 掃描所有掛載點\n");
        print_separator();
        scan_all_mounts();
        return 0;
    }

    /* ── 空間與 inode 資訊 ── */

    /* 空間使用率顏色 */
    unsigned long used_pct = calc_used_percent(&fs);
    const char *space_color = used_pct >= 90 ? COLOR_RED :
                            used_pct >= 70 ? COLOR_YELLOW : COLOR_GREEN;

    ret = diag_fs_read(path, &fs);
    if (ret != DIAG_OK) {
        printf("Error: 無法讀取 %s：%s\n", path, diag_strerror(ret));
        return 1;
    }

    printf("路徑        : %s\n", fs.path);
    printf("檔案系統類型: %s\n", diag_fs_type_name(fs.fs_type));
    print_separator();

    printf("[空間使用]\n");
    printf("  總容量    : %lu KB\n", fs.total_kb);
    printf("  已使用    : %lu KB (%s%lu%%%s)\n", fs.used_kb, space_color, used_pct, COLOR_RESET);
    printf("  可用空間  : %lu KB\n", fs.avail_kb);
    print_separator();

    /* inode 使用率顏色 */
    unsigned long inode_pct = calc_inode_used_percent(&fs);
    const char *inode_color = inode_pct >= 90 ? COLOR_RED :
                            inode_pct >= 70 ? COLOR_YELLOW : COLOR_GREEN;

    printf("[inode 使用]\n");
    printf("  inode 總數: %lu\n", fs.files_total);
    printf("  inode 剩餘: %lu\n", fs.files_free);
    printf("  inode 使用: %s%lu%%%s\n", inode_color, inode_pct, COLOR_RESET);

    /* 警告 inode 快滿 */
    if (calc_inode_used_percent(&fs) >= 90) {
        printf("  [警告] inode 使用率超過 90%%，可能導致無法建立新檔案！\n");
    }

    print_separator();

    /* ── 碎片化資訊 ── */
    printf("[碎片化分析]\n");
    ret = diag_fs_read_fiemap(path, &fiemap);
    if (ret == DIAG_ERR_UNSUPPORTED) {
        printf("  此檔案系統不支援 FIEMAP\n");
    } else if (ret != DIAG_OK) {
        printf("  碎片化資訊讀取失敗：%s\n", diag_strerror(ret));
    } else {
        printf("  extent 數量 : %u\n", fiemap.extent_count);
        printf("  邏輯大小    : %llu bytes\n", fiemap.logical_bytes);
        printf("  實體大小    : %llu bytes\n", fiemap.physical_bytes);

        /* 碎片化程度判斷：extent 越多代表越分散 */
        if (fiemap.extent_count <= 1) {
            printf("  碎片化程度  : 良好（連續存放）\n");
        } else if (fiemap.extent_count <= 10) {
            printf("  碎片化程度  : 普通\n");
        } else {
            printf("  碎片化程度  : 嚴重，建議整理\n");
        }
    }

    print_separator();
    return 0;
}

