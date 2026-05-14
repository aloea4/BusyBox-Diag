#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define EXIT_OK               0
#define EXIT_ERR_RUNTIME      1
#define EXIT_ERR_USAGE        2
#define EXIT_ERR_UNSUPPORTED  3

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"

/* 定義支援的輸出格式 */
typedef enum {
    DIAGFS_OUT_TABLE = 0,
    DIAGFS_OUT_RAW,
    DIAGFS_OUT_JSON
} diagfs_output_format_t;


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

/* Presentation 層：根據健康狀態與輸出裝置決定顏色 */
static const char* get_health_color(diagfs_health_level_t health)
{
    // 檢查 stdout 是否為終端機。如果被 pipe (例如 | tee) 則不輸出顏色碼
    if (!isatty(STDOUT_FILENO)) {
        return "";
    }
    
    switch (health) {
        case DIAGFS_HEALTH_CRITICAL: return COLOR_RED;
        case DIAGFS_HEALTH_WARN:     return COLOR_YELLOW;
        case DIAGFS_HEALTH_OK:       return COLOR_GREEN;
        default:                     return "";
    }
}

/* 取得還原顏色的色碼，如果非終端機則回傳空字串 */
static const char* get_color_reset(void)
{
    if (!isatty(STDOUT_FILENO)) {
        return "";
    }
    return COLOR_RESET;
}

/* 給 stderr 專用的顏色判斷 (檢查 STDERR_FILENO) */
static const char* get_health_color_err(diagfs_health_level_t health)
{
    if (!isatty(STDERR_FILENO)) return "";
    switch (health) {
        case DIAGFS_HEALTH_CRITICAL: return COLOR_RED;
        case DIAGFS_HEALTH_WARN:     return COLOR_YELLOW;
        case DIAGFS_HEALTH_OK:       return COLOR_GREEN;
        default:                     return "";
    }
}

/* 給 stderr 專用的還原色碼 */
static const char* get_color_reset_err(void)
{
    if (!isatty(STDERR_FILENO)) return "";
    return COLOR_RESET;
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

/* Presentation 層：輸出為 Raw 格式 (適合 shell pipeline) */
static void print_out_raw(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis)
{
    const char *space_status = (analysis->space_health == DIAGFS_HEALTH_CRITICAL) ? "critical" :
                               (analysis->space_health == DIAGFS_HEALTH_WARN) ? "warn" : "ok";
    const char *inode_status = (analysis->inode_health == DIAGFS_HEALTH_CRITICAL) ? "critical" :
                               (analysis->inode_health == DIAGFS_HEALTH_WARN) ? "warn" : "ok";

    // 格式：PATH TYPE TOTAL_KB USED_KB FREE_KB USE% INODE% SPACE_HEALTH INODE_HEALTH
    printf("%s %s %lu %lu %lu %lu %lu %s %s\n",
           fs->path, diag_fs_type_name(fs->fs_type),
           fs->total_kb, fs->used_kb, fs->avail_kb,
           analysis->used_percent, analysis->inode_used_percent,
           space_status, inode_status);
}

/* Presentation 層：輸出為 JSON 格式 (適合 Web API 或 jq 解析) */
/* Presentation 層：輸出單一檔案系統的 JSON 物件 (作為陣列元素) */
static void print_out_json(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diag_fiemap_info_t *fiemap, int fiemap_ret)
{
    const char *space_status = (analysis->space_health == DIAGFS_HEALTH_CRITICAL) ? "critical" :
                               (analysis->space_health == DIAGFS_HEALTH_WARN) ? "warn" : "ok";
    const char *inode_status = (analysis->inode_health == DIAGFS_HEALTH_CRITICAL) ? "critical" :
                               (analysis->inode_health == DIAGFS_HEALTH_WARN) ? "warn" : "ok";

    printf("    {\n");
    printf("      \"path\": \"%s\",\n", fs->path);
    printf("      \"type\": \"%s\",\n", diag_fs_type_name(fs->fs_type));
    printf("      \"usage\": {\n");
    printf("        \"total_kb\": %lu,\n", fs->total_kb);
    printf("        \"used_kb\": %lu,\n", fs->used_kb);
    printf("        \"free_kb\": %lu,\n", fs->avail_kb);
    printf("        \"used_percent\": %lu\n", analysis->used_percent);
    printf("      },\n");
    printf("      \"inode\": {\n");
    printf("        \"total\": %lu,\n", fs->files_total);
    printf("        \"free\": %lu,\n", fs->files_free);
    printf("        \"used_percent\": %lu\n", analysis->inode_used_percent);
    printf("      },\n");
    
    // 加入 layout (FIEMAP extent) 資訊
    printf("      \"layout\": {\n");
    if (fiemap_ret == DIAG_ERR_UNSUPPORTED) {
        printf("        \"fiemap_supported\": false\n");
    } else if (fiemap_ret == DIAG_OK) {
        printf("        \"fiemap_supported\": true,\n");
        printf("        \"extent_count\": %u,\n", fiemap->extent_count);
        if (fiemap->extent_count <= 1) {
            printf("        \"observation\": \"low_extent_count\"\n");
        } else if (fiemap->extent_count <= 10) {
            printf("        \"observation\": \"medium_extent_count\"\n");
        } else {
            printf("        \"observation\": \"high_extent_count\"\n");
        }
    } else {
        printf("        \"fiemap_supported\": false,\n");
        printf("        \"error\": \"read_failed\"\n");
    }
    printf("      },\n");

    printf("      \"health\": {\n");
    printf("        \"space\": \"%s\",\n", space_status);
    printf("        \"inode\": \"%s\"\n", inode_status);
    printf("      },\n"); 

    // 動態產生 warnings 陣列
    // 動態產生 warnings 陣列 (包含 CRITICAL 與 WARN)
    printf("      \"warnings\": [");
    int has_warn = 0;
    
    // 檢查空間狀態
    if (analysis->space_health != DIAGFS_HEALTH_OK) {
        printf("\"space_%s\"", (analysis->space_health == DIAGFS_HEALTH_CRITICAL) ? "critical" : "warning");
        has_warn = 1;
    }
    
    // 檢查 inode 狀態
    if (analysis->inode_health != DIAGFS_HEALTH_OK) {
        if (has_warn) printf(", ");
        printf("\"inode_%s\"", (analysis->inode_health == DIAGFS_HEALTH_CRITICAL) ? "critical" : "warning");
    }
    printf("]\n");
}

/* 掃描所有掛載點並支援多種輸出格式 */
static int scan_all_mounts(diagfs_output_format_t out_format, const diagfs_policy_t *policy)
{
    FILE *fp;
    char line[256];
    char dev[128], path[128], fstype[64], opts[128];
    int dump, pass;
    int is_first = 1;

    // 修正：使用 namespace-aware 的 /proc/self/mounts
    fp = fopen("/proc/self/mounts", "r");
    if (!fp) {
        // 修正：錯誤訊息寫入 stderr
        fprintf(stderr, "Error: 無法讀取 /proc/self/mounts\n");
        return EXIT_ERR_RUNTIME;
    }

    if (out_format == DIAGFS_OUT_JSON) {
        printf("{\n  \"filesystems\": [\n");
    } else if (out_format == DIAGFS_OUT_TABLE) {
        printf("diagfs - 掃描所有掛載點\n");
        print_separator();
    }

    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%s %s %s %s %d %d", dev, path, fstype, opts, &dump, &pass);

        /* 跳過虛擬檔案系統 */
        if (strcmp(fstype, "proc") == 0 || strcmp(fstype, "sysfs") == 0 ||
            strcmp(fstype, "devtmpfs") == 0 || strcmp(fstype, "cgroup") == 0 ||
            strcmp(fstype, "cgroup2") == 0 || strcmp(fstype, "devpts") == 0 ||
            strcmp(fstype, "mqueue") == 0 || strcmp(fstype, "hugetlbfs") == 0 ||
            strcmp(fstype, "tmpfs") == 0)
            continue;

        /* 跳過檔案路徑 */
        if (strncmp(path, "/proc/", 6) == 0 || strncmp(path, "/sys/", 5) == 0 ||
            strncmp(path, "/etc/", 5) == 0 || strncmp(path, "/home/", 6) == 0)
            continue;

        diag_fs_info_t fs;
        if (diag_fs_read(path, &fs) != DIAG_OK)
            continue;

        // 進行分析
        diagfs_analysis_t analysis = analyze_filesystem(&fs, policy);
        diag_fiemap_info_t fiemap;
        int ret_fiemap = diag_fs_read_fiemap(path, &fiemap);

        // 根據格式輸出
        if (out_format == DIAGFS_OUT_JSON) {
            if (!is_first) printf(",\n");
            print_out_json(&fs, &analysis, &fiemap, ret_fiemap);
            is_first = 0;
        } else if (out_format == DIAGFS_OUT_RAW) {
            print_out_raw(&fs, &analysis);
        } else {
            // TABLE 輸出，並套用智慧顏色
            printf("\n路徑        : %s\n", fs.path);
            printf("檔案系統類型: %s\n", diag_fs_type_name(fs.fs_type));
            printf("[空間使用] 已使用 %lu KB / 總計 %lu KB (%s%lu%%%s)\n",
                   fs.used_kb, fs.total_kb, 
                   get_health_color(analysis.space_health), analysis.used_percent, get_color_reset());
            printf("[inode 使用] %s%lu%%%s\n", 
                   get_health_color(analysis.inode_health), analysis.inode_used_percent, get_color_reset());
            print_separator();
        }
    }

    if (out_format == DIAGFS_OUT_JSON) {
        printf("\n  ]\n}\n");
    }

    fclose(fp);
    return EXIT_OK;
}

int main(int argc, char **argv)
{
    const char *path = "/";
    diag_fs_info_t fs;
    diag_fiemap_info_t fiemap;
    int ret;
    diagfs_output_format_t out_format = DIAGFS_OUT_TABLE;
    int scan_all = 0;
    int require_fiemap = 0; // 新增：使用者是否明確要求 FIEMAP

    // 簡易參數解析
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            scan_all = 1;
        } else if (strcmp(argv[i], "--fiemap") == 0) {
            // 新增：設定強制要求 FIEMAP 旗標
            require_fiemap = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "raw") == 0) out_format = DIAGFS_OUT_RAW;
                else if (strcmp(argv[i], "json") == 0) out_format = DIAGFS_OUT_JSON;
                else if (strcmp(argv[i], "table") == 0) out_format = DIAGFS_OUT_TABLE;
                else {
                    fprintf(stderr, "Error: 不支援的輸出格式 '%s'\n", argv[i]);
                    return EXIT_ERR_USAGE;
                }
            } else {
                fprintf(stderr, "Error: --output 選項需要指定格式 (table|raw|json)\n");
                return EXIT_ERR_USAGE;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: diagfs [PATH] [--all] [--output table|raw|json] [--fiemap]\n");
            return EXIT_OK;
        } else {
            path = argv[i];
        }
    }

    // 1. 取得政策 (Policy)
    diagfs_policy_t policy = get_default_policy();

    // 如果使用者下了 --all，就把格式傳給 scan_all_mounts 去處理
    if (scan_all) {
        return scan_all_mounts(out_format, &policy);
    }

    // --- 單一掛載點邏輯 ---
    ret = diag_fs_read(path, &fs);
    if (ret != DIAG_OK) {
        fprintf(stderr, "Error: 無法讀取 %s：%s\n", path, diag_strerror(ret));
        return EXIT_ERR_RUNTIME;
    }

    diagfs_analysis_t analysis = analyze_filesystem(&fs, &policy);
    int ret_fiemap = diag_fs_read_fiemap(path, &fiemap);

    // 新增：若使用者明確要求 FIEMAP 但檔案系統不支援，回傳 exit 3
    if (require_fiemap && ret_fiemap == DIAG_ERR_UNSUPPORTED) {
        fprintf(stderr, "Error: %s 所在的檔案系統不支援 FIEMAP\n", path);
        return EXIT_ERR_UNSUPPORTED;
    }

    // 4. 展示層 (Presentation)
    if (out_format == DIAGFS_OUT_RAW) {
        print_out_raw(&fs, &analysis);
    } else if (out_format == DIAGFS_OUT_JSON) {
        printf("{\n  \"filesystems\": [\n");
        print_out_json(&fs, &analysis, &fiemap, ret_fiemap);
        printf("\n  ]\n}\n");
    } else {
        printf("diagfs - Filesystem Health Checker\n");
        print_separator();

        printf("路徑        : %s\n", fs.path);
        printf("檔案系統類型: %s\n", diag_fs_type_name(fs.fs_type));
        print_separator();

        printf("[空間使用]\n");
        printf("  總容量    : %lu KB\n", fs.total_kb);
        printf("  已使用    : %lu KB (%s%lu%%%s)\n",
               fs.used_kb,
               get_health_color(analysis.space_health),
               analysis.used_percent,
               get_color_reset());
        printf("  可用空間  : %lu KB\n", fs.avail_kb);
        print_separator();

        printf("[inode 使用]\n");
        printf("  inode 總數: %lu\n", fs.files_total);
        printf("  inode 剩餘: %lu\n", fs.files_free);
        printf("  inode 使用: %s%lu%%%s\n",
               get_health_color(analysis.inode_health),
               analysis.inode_used_percent,
               get_color_reset());

        if (analysis.inode_health == DIAGFS_HEALTH_CRITICAL) {
            fprintf(stderr, "%s  [警告] inode 使用率已達危險等級（>= %lu%%），可能導致無法建立新檔案！%s\n",
                    get_health_color_err(DIAGFS_HEALTH_CRITICAL), policy.inode_critical_percent, get_color_reset_err());
        }

        print_separator();

        printf("[extent 配置觀察]\n");
        if (ret_fiemap == DIAG_ERR_UNSUPPORTED) {
            printf("  此檔案系統不支援 FIEMAP\n");
        } else if (ret_fiemap != DIAG_OK) {
            printf("  extent 資訊讀取失敗：%s\n", diag_strerror(ret_fiemap));
        } else {
            printf("  extent 數量 : %u\n", fiemap.extent_count);
            printf("  邏輯大小    : %llu bytes\n", fiemap.logical_bytes);
            printf("  實體大小    : %llu bytes\n", fiemap.physical_bytes);

            if (fiemap.extent_count <= 1) {
                printf("  extent 配置觀察: 數量較少，推測該檔案配置較為連續\n");
            } else if (fiemap.extent_count <= 10) {
                printf("  extent 配置觀察: 數量普通，有輕度分散\n");
            } else {
                printf("  extent 配置觀察: 數量偏多，推測檔案實體佈局較分散\n");
            }
        }
        print_separator();
    }

    return EXIT_OK;
}


