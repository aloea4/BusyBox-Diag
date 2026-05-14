#include "diagfs.h"
#include <stdio.h>

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"

static diagfs_color_mode_t g_color_mode = DIAGFS_COLOR_AUTO;

void diagfs_set_color_mode(diagfs_color_mode_t mode)
{
    g_color_mode = mode;
}

static int should_use_color(int fd)
{
    switch (g_color_mode) {
        case DIAGFS_COLOR_ALWAYS: return 1;
        case DIAGFS_COLOR_NEVER:  return 0;
        default:                  return isatty(fd);
    }
}

static const char* get_health_color(diagfs_health_level_t health)
{
    if (!should_use_color(STDOUT_FILENO)) return "";
    switch (health) {
        case DIAGFS_HEALTH_CRITICAL: return COLOR_RED;
        case DIAGFS_HEALTH_WARN:     return COLOR_YELLOW;
        case DIAGFS_HEALTH_OK:       return COLOR_GREEN;
        default:                     return "";
    }
}

static const char* get_color_reset(void)
{
    return should_use_color(STDOUT_FILENO) ? COLOR_RESET : "";
}

static const char* health_to_str(diagfs_health_level_t health)
{
    switch (health) {
        case DIAGFS_HEALTH_CRITICAL: return "critical";
        case DIAGFS_HEALTH_WARN:     return "warn";
        case DIAGFS_HEALTH_OK:       return "ok";
        default:                     return "unknown";
    }
}

static const char* layout_level_str(diagfs_layout_level_t level)
{
    switch (level) {
        case DIAGFS_LAYOUT_LOW_EXTENT:    return "low_extent_count";
        case DIAGFS_LAYOUT_MEDIUM_EXTENT: return "medium_extent_count";
        case DIAGFS_LAYOUT_HIGH_EXTENT:   return "high_extent_count";
        default:                          return "unsupported";
    }
}

/* ── 修復：Raw 輸出補上 extent_count ── */
void print_out_raw(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout)
{
    /* 修復：格式字串最後面從 %s 變成 %s %s，並補上 inode_health */
    printf("%s %s %lu %lu %lu %lu %lu %u %s %s\n",
           fs->path, diag_fs_type_name(fs->fs_type),
           fs->total_kb, fs->used_kb, fs->avail_kb,
           analysis->used_percent, analysis->inode_used_percent,
           layout->fiemap_supported ? layout->extent_count : 0,
           health_to_str(analysis->space_health),
           health_to_str(analysis->inode_health));
}

/* ── 修復：JSON Schema 補上 health.layout ── */
void print_out_json(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout)
{
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
    printf("      \"layout\": {\n");
    printf("        \"fiemap_supported\": %s", layout->fiemap_supported ? "true" : "false");
    if (layout->fiemap_supported) {
        printf(",\n        \"extent_count\": %u,\n", layout->extent_count);
        printf("        \"observation\": \"%s\"\n", layout_level_str(layout->level));
    } else {
        printf("\n");
    }
    printf("      },\n");
    printf("      \"health\": {\n");
    printf("        \"space\": \"%s\",\n", health_to_str(analysis->space_health));
    printf("        \"inode\": \"%s\",\n", health_to_str(analysis->inode_health));
    printf("        \"layout\": \"%s\"\n", health_to_str(analysis->layout_health)); 
    printf("      },\n");

    /* warnings 陣列 */
    printf("      \"warnings\": [");
    int has = 0;
    if (analysis->space_health != DIAGFS_HEALTH_OK) {
        printf("\"space_%s\"", (analysis->space_health == DIAGFS_HEALTH_CRITICAL) ? "critical" : "warning");
        has = 1;
    }
    if (analysis->inode_health != DIAGFS_HEALTH_OK) {
        if (has) printf(", ");
        printf("\"inode_%s\"", (analysis->inode_health == DIAGFS_HEALTH_CRITICAL) ? "critical" : "warning");
        has = 1;
    }
    if (analysis->layout_health == DIAGFS_HEALTH_WARN) {
        if (has) printf(", ");
        printf("\"layout_warning\"");
    }
    printf("]\n");
    printf("    }");
}

/* ── 修復：統一 Table 輸出，讓 --all 也能印出 extent 資訊 ── */
void print_out_table(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout)
{
    printf("路徑        : %s\n", fs->path);
    printf("檔案系統類型: %s\n", diag_fs_type_name(fs->fs_type));
    printf("[空間使用] 已使用 %lu KB / 總計 %lu KB (%s%lu%%%s)\n",
           fs->used_kb, fs->total_kb,
           get_health_color(analysis->space_health),
           analysis->used_percent, get_color_reset());
    printf("[inode 使用] %s%lu%%%s\n",
           get_health_color(analysis->inode_health),
           analysis->inode_used_percent, get_color_reset());
           
    if (layout->fiemap_supported) {
        printf("[extent 觀察] 數量: %u (%s)\n", layout->extent_count, layout_level_str(layout->level));
    }
    printf("------------------------------------------\n");
}