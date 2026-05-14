#include "diagfs.h"

/* ── 計算百分比 (內部使用) ── */
static unsigned long calc_used_percent(const diag_fs_info_t *fs)
{
    if (fs->total_kb == 0) return 0;
    return (fs->used_kb * 100UL) / fs->total_kb;
}

static unsigned long calc_inode_used_percent(const diag_fs_info_t *fs)
{
    unsigned long used;
    if (fs->files_total == 0) return 0;
    if (fs->files_total < fs->files_free) return 0;
    used = fs->files_total - fs->files_free;
    
    /* * 採用 Ceiling (無條件進位) 策略。
     * 原因：inode 耗盡會導致無法建立新檔案，屬於嚴重錯誤。
     * 為了系統診斷的保守性，即使只超出些微比例，也寧可進位以提早觸發警告閾值。
     */
    return (used * 100UL + fs->files_total - 1) / fs->files_total;

}

/* ── Layout Analysis ── */
diagfs_layout_analysis_t analyze_layout(const diag_fiemap_info_t *fiemap,
                                        int fiemap_ret,
                                        const diagfs_policy_t *policy)
{
    diagfs_layout_analysis_t result;

    if (fiemap_ret != DIAG_OK) {
        result.fiemap_supported = 0;
        result.extent_count     = 0;
        result.level            = DIAGFS_LAYOUT_UNSUPPORTED;
        return result;
    }

    result.fiemap_supported = 1;
    result.extent_count     = fiemap->extent_count;

    if (fiemap->extent_count <= policy->extent_warn_count)
        result.level = DIAGFS_LAYOUT_LOW_EXTENT;
    else if (fiemap->extent_count <= policy->extent_critical_count)
        result.level = DIAGFS_LAYOUT_MEDIUM_EXTENT;
    else
        result.level = DIAGFS_LAYOUT_HIGH_EXTENT;

    return result;
}

/* ── Filesystem Analysis & Policy ── */
diagfs_analysis_t analyze_filesystem(const diag_fs_info_t *fs,
                                     const diagfs_policy_t *policy,
                                     const diagfs_layout_analysis_t *layout)
{
    diagfs_analysis_t result;
    result.flags = 0;

    result.used_percent       = calc_used_percent(fs);
    result.inode_used_percent = calc_inode_used_percent(fs);

    if (result.used_percent >= policy->space_critical_percent)
        result.space_health = DIAGFS_HEALTH_CRITICAL;
    else if (result.used_percent >= policy->space_warn_percent)
        result.space_health = DIAGFS_HEALTH_WARN;
    else
        result.space_health = DIAGFS_HEALTH_OK;

    if (result.inode_used_percent >= policy->inode_critical_percent)
        result.inode_health = DIAGFS_HEALTH_CRITICAL;
    else if (result.inode_used_percent >= policy->inode_warn_percent)
        result.inode_health = DIAGFS_HEALTH_WARN;
    else
        result.inode_health = DIAGFS_HEALTH_OK;

    switch (layout->level) {
        case DIAGFS_LAYOUT_HIGH_EXTENT:  result.layout_health = DIAGFS_HEALTH_WARN;    break;
        case DIAGFS_LAYOUT_UNSUPPORTED:  result.layout_health = DIAGFS_HEALTH_UNKNOWN; break;
        default:                         result.layout_health = DIAGFS_HEALTH_OK;      break;
    }

    return result;
}