#ifndef DIAGFS_H
#define DIAGFS_H

#include "libdiag/diag_common.h"
#include "libdiag/diag_fs.h"
#include <unistd.h>

/* ── Exit Code Contract ── */
#define EXIT_OK               0
#define EXIT_ERR_RUNTIME      1
#define EXIT_ERR_USAGE        2
#define EXIT_ERR_UNSUPPORTED  3

#define DIAG_PATH_MAX 256

/* ── Analysis Flags 定義 (預留擴充) ── */
#define DIAGFS_FLAG_READONLY  (1U << 0)
#define DIAGFS_FLAG_REMOTE    (1U << 1)

/* ── Enum 定義 ── */
typedef enum {
    DIAGFS_OUT_TABLE = 0,
    DIAGFS_OUT_RAW,
    DIAGFS_OUT_JSON
} diagfs_output_format_t;

typedef enum {
    DIAGFS_COLOR_AUTO = 0,
    DIAGFS_COLOR_ALWAYS,
    DIAGFS_COLOR_NEVER
} diagfs_color_mode_t;

typedef enum {
    DIAGFS_SCAN_REAL = 0,
    DIAGFS_SCAN_PSEUDO
} diagfs_scan_filter_t;

typedef enum {
    DIAGFS_HEALTH_OK = 0,
    DIAGFS_HEALTH_WARN,
    DIAGFS_HEALTH_CRITICAL,
    DIAGFS_HEALTH_UNKNOWN
} diagfs_health_level_t;

typedef enum {
    DIAGFS_LAYOUT_UNSUPPORTED = 0,
    DIAGFS_LAYOUT_LOW_EXTENT,
    DIAGFS_LAYOUT_MEDIUM_EXTENT,
    DIAGFS_LAYOUT_HIGH_EXTENT
} diagfs_layout_level_t;

/* ── Struct 定義 ── */
typedef struct {
    int fiemap_supported;
    unsigned int extent_count;
    diagfs_layout_level_t level;
} diagfs_layout_analysis_t;

typedef struct {
    char source[DIAG_PATH_MAX];
    char target[DIAG_PATH_MAX];
    char fstype[64];
    char options[1024]; /* 擴大容量，避免 sscanf 截斷長度較長的 mount options */
    int is_pseudo;
    int is_remote;
} diagfs_mount_entry_t;

typedef struct {
    unsigned long space_warn_percent;
    unsigned long space_critical_percent;
    unsigned long inode_warn_percent;
    unsigned long inode_critical_percent;
    unsigned int  extent_warn_count;
    unsigned int  extent_critical_count;
} diagfs_policy_t;

typedef struct {
    unsigned long used_percent;
    unsigned long inode_used_percent;
    diagfs_health_level_t space_health;
    diagfs_health_level_t inode_health;
    diagfs_health_level_t layout_health;
    unsigned int flags; 
} diagfs_analysis_t;


/* ══════════════════════════════════════════════════════════════════════
 * Layer 介面宣告 (Function Prototypes)
 * ══════════════════════════════════════════════════════════════════════ */

/* ── Layer 2: Analysis (diagfs_analyze.c) ── */
diagfs_layout_analysis_t analyze_layout(const diag_fiemap_info_t *fiemap, 
                                        int fiemap_ret, 
                                        const diagfs_policy_t *policy);

diagfs_analysis_t analyze_filesystem(const diag_fs_info_t *fs, 
                                     const diagfs_policy_t *policy, 
                                     const diagfs_layout_analysis_t *layout);

/* ── Layer 3: Policy (diagfs_policy.c) ── */
diagfs_policy_t get_default_policy(void);

/* ── Layer 4: Presentation (diagfs_output.c) ── */
void diagfs_set_color_mode(diagfs_color_mode_t mode);
void print_out_raw(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout);
void print_out_json(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout);
void print_out_table(const diag_fs_info_t *fs, const diagfs_analysis_t *analysis, const diagfs_layout_analysis_t *layout);

/* ── Utility: Mount Traversal (diagfs_mount.c) ── */
int scan_all_mounts(diagfs_output_format_t out_format, 
                    const diagfs_policy_t *policy, 
                    diagfs_scan_filter_t filter);

#endif /* DIAGFS_H */