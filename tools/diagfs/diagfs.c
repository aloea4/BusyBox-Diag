#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>
#include <string.h>

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"

/* 健康狀態與 Policy */
/* 定義健康狀態層級 */
typedef enum {
    DIAGFS_HEALTH_OK = 0,
    DIAGFS_HEALTH_WARN,
    DIAGFS_HEALTH_CRITICAL,
    DIAGFS_HEALTH_UNKNOWN
} diagfs_health_level_t;

/* 定義 Policy：什麼數字叫警告？什麼數字叫危險？ */
typedef struct {
    unsigned long space_warn_percent;
    unsigned long space_critical_percent;
    unsigned long inode_warn_percent;
    unsigned long inode_critical_percent;
} diagfs_policy_t;

/* 定義 Analysis：用來裝計算結果與健康診斷 */
typedef struct {
    unsigned long used_percent;
    unsigned long inode_used_percent;
    diagfs_health_level_t space_health;
    diagfs_health_level_t inode_health;
} diagfs_analysis_t;

/* 取得預設的檢查政策 (90% 危險，70% 警告) */
static diagfs_policy_t get_default_policy(void)
{
    diagfs_policy_t policy = {
        .space_warn_percent = 70,
        .space_critical_percent = 90,
        .inode_warn_percent = 70,
        .inode_critical_percent = 90
    };
    return policy;
}

/* Presentation 層：根據健康狀態決定顏色 */
static const char* get_health_color(diagfs_health_level_t health)
{
    switch (health) {
        case DIAGFS_HEALTH_CRITICAL: return COLOR_RED;
        case DIAGFS_HEALTH_WARN:     return COLOR_YELLOW;
        case DIAGFS_HEALTH_OK:       return COLOR_GREEN;
        default:                     return COLOR_RESET;
    }
}

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

/* Analysis & Policy 層：進行運算並根據政策判定健康狀態 */
static diagfs_analysis_t analyze_filesystem(const diag_fs_info_t *fs, const diagfs_policy_t *policy)
{
    diagfs_analysis_t result;

    // 1. 測量與計算 (Measurement & Analysis)
    result.used_percent = calc_used_percent(fs);
    result.inode_used_percent = calc_inode_used_percent(fs);

    // 2. 判斷空間健康狀態 (Policy Decision)
    if (result.used_percent >= policy->space_critical_percent) {
        result.space_health = DIAGFS_HEALTH_CRITICAL;
    } else if (result.used_percent >= policy->space_warn_percent) {
        result.space_health = DIAGFS_HEALTH_WARN;
    } else {
        result.space_health = DIAGFS_HEALTH_OK;
    }

    // 3. 判斷 inode 健康狀態 (Policy Decision)
    if (result.inode_used_percent >= policy->inode_critical_percent) {
        result.inode_health = DIAGFS_HEALTH_CRITICAL;
    } else if (result.inode_used_percent >= policy->inode_warn_percent) {
        result.inode_health = DIAGFS_HEALTH_WARN;
    } else {
        result.inode_health = DIAGFS_HEALTH_OK;
    }

    return result;
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
    // 1. 取得政策 (Policy)
    diagfs_policy_t policy = get_default_policy();

    // 2. 讀取資料 (Measurement)
    ret = diag_fs_read(path, &fs);
    if (ret != DIAG_OK) {
        // 遵守 UNIX 工具契約：錯誤訊息印到 stderr，並回傳錯誤碼 1
        fprintf(stderr, "Error: 無法讀取 %s：%s\n", path, diag_strerror(ret));
        return 1;
    }

    // 3. 執行分析 (Analysis & Policy Decision)
    diagfs_analysis_t analysis = analyze_filesystem(&fs, &policy);

    printf("路徑        : %s\n", fs.path);
    printf("檔案系統類型: %s\n", diag_fs_type_name(fs.fs_type));
    print_separator();

    // 4. 展示空間使用狀態 (Presentation)
    printf("[空間使用]\n");
    printf("  總容量    : %lu KB\n", fs.total_kb);
    printf("  已使用    : %lu KB (%s%lu%%%s)\n", 
           fs.used_kb, 
           get_health_color(analysis.space_health), 
           analysis.used_percent, 
           COLOR_RESET);
    printf("  可用空間  : %lu KB\n", fs.avail_kb);
    print_separator();

    // 5. 展示 inode 使用狀態 (Presentation)
    printf("[inode 使用]\n");
    printf("  inode 總數: %lu\n", fs.files_total);
    printf("  inode 剩餘: %lu\n", fs.files_free);
    printf("  inode 使用: %s%lu%%%s\n", 
           get_health_color(analysis.inode_health), 
           analysis.inode_used_percent, 
           COLOR_RESET);

    // 如果分析結果是危險，印出警告訊息 (使用 stderr)
    if (analysis.inode_health == DIAGFS_HEALTH_CRITICAL) {
        fprintf(stderr, "%s  [警告] inode 使用率已達危險等級（>= %lu%%），可能導致無法建立新檔案！%s\n", 
                COLOR_RED, policy.inode_critical_percent, COLOR_RESET);
    }
    
    // 如果想要，也可以加上空間快滿的警告
    if (analysis.space_health == DIAGFS_HEALTH_CRITICAL) {
        fprintf(stderr, "%s  [警告] 儲存空間已達危險等級（>= %lu%%）！%s\n", 
                COLOR_RED, policy.space_critical_percent, COLOR_RESET);
    }

    print_separator();

    /* ── 碎片化資訊 ── */
    printf("[extent 配置觀察]\n");
    ret = diag_fs_read_fiemap(path, &fiemap);
    if (ret == DIAG_ERR_UNSUPPORTED) {
        printf("  此檔案系統不支援 FIEMAP\n");
    } else if (ret != DIAG_OK) {
        printf("  碎片化資訊讀取失敗：%s\n", diag_strerror(ret));
    } else {
        printf("  extent 數量 : %u\n", fiemap.extent_count);
        printf("  邏輯大小    : %llu bytes\n", fiemap.logical_bytes);
        printf("  實體大小    : %llu bytes\n", fiemap.physical_bytes);

        /* extent layout 觀察指標：不代表整體檔案系統碎片化分數 */
        if (fiemap.extent_count <= 1) {
            printf("  extent 配置觀察: 數量較少，推測該檔案配置較為連續\n");
        } else if (fiemap.extent_count <= 10) {
            printf("  extent 配置觀察: 數量普通，有輕度分散\n");
        } else {
            printf("  extent 配置觀察: 數量偏多，推測檔案實體佈局較分散\n");
        }
    }

    print_separator();
    return 0;
}

