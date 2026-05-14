#include "diagfs.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *path = "/";
    diag_fs_info_t fs;
    diag_fiemap_info_t fiemap;
    int ret;
    diagfs_output_format_t out_format  = DIAGFS_OUT_TABLE;
    diagfs_scan_filter_t   scan_filter = DIAGFS_SCAN_REAL;
    int scan_all       = 0;
    int require_fiemap = 0;
    int filter_set_manually = 0; 

    /* ── CLI 參數解析 ── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            scan_all = 1;
        } else if (strcmp(argv[i], "--real") == 0) {
            scan_filter = DIAGFS_SCAN_REAL;
            filter_set_manually = 1;
        } else if (strcmp(argv[i], "--pseudo") == 0) {
            scan_filter = DIAGFS_SCAN_PSEUDO;
            filter_set_manually = 1;
        } else if (strcmp(argv[i], "--fiemap") == 0) {
            require_fiemap = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                i++;
                if      (strcmp(argv[i], "raw")   == 0) out_format = DIAGFS_OUT_RAW;
                else if (strcmp(argv[i], "json")  == 0) out_format = DIAGFS_OUT_JSON;
                else if (strcmp(argv[i], "table") == 0) out_format = DIAGFS_OUT_TABLE;
                else {
                    fprintf(stderr, "Error: 不支援的輸出格式 '%s'\n", argv[i]);
                    return EXIT_ERR_USAGE;
                }
            } else {
                fprintf(stderr, "Error: --output 選項需要指定格式 (table|raw|json)\n");
                return EXIT_ERR_USAGE;
            }
        } else if (strcmp(argv[i], "--color") == 0) {
            if (i + 1 < argc) {
                i++;
                /* 微調：改為呼叫封裝好的 function，而不是直接改全域變數 */
                if      (strcmp(argv[i], "auto")   == 0) diagfs_set_color_mode(DIAGFS_COLOR_AUTO);
                else if (strcmp(argv[i], "always") == 0) diagfs_set_color_mode(DIAGFS_COLOR_ALWAYS);
                else if (strcmp(argv[i], "never")  == 0) diagfs_set_color_mode(DIAGFS_COLOR_NEVER);
                else {
                    fprintf(stderr, "Error: 不支援的顏色模式 '%s'\n", argv[i]);
                    return EXIT_ERR_USAGE;
                }
            } else {
                fprintf(stderr, "Error: --color 選項需要指定模式 (auto|always|never)\n");
                return EXIT_ERR_USAGE;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: diagfs [PATH] [--all] [--real] [--pseudo]\n");
            printf("              [--output table|raw|json]\n");
            printf("              [--color auto|always|never]\n");
            printf("              [--fiemap]\n");
            return EXIT_OK;
        } else {
            path = argv[i];
        }
    }

    /* 修復：防呆機制，避免單一路徑模式下誤用過濾參數造成誤解 */
    if (!scan_all && filter_set_manually) {
        fprintf(stderr, "Warning: --real / --pseudo 過濾器只有在搭配 --all 掃描時才會生效。\n");
    }

    /* ── 初始化 Policy ── */
    diagfs_policy_t policy = get_default_policy();

    /* ── 執行路徑 1：掃描所有掛載點 ── */
    if (scan_all)
        return scan_all_mounts(out_format, &policy, scan_filter);

    /* ── 執行路徑 2：單一掛載點分析 ── */
    ret = diag_fs_read(path, &fs);
    if (ret != DIAG_OK) {
        fprintf(stderr, "Error: 無法讀取 %s：%s\n", path, diag_strerror(ret));
        return EXIT_ERR_RUNTIME;
    }

    int ret_fiemap = diag_fs_read_fiemap(path, &fiemap);
    if (require_fiemap && ret_fiemap == DIAG_ERR_UNSUPPORTED) {
        fprintf(stderr, "Error: %s 所在的檔案系統不支援 FIEMAP\n", path);
        return EXIT_ERR_UNSUPPORTED;
    }

    /* 取得分析結果 */
    diagfs_layout_analysis_t layout   = analyze_layout(&fiemap, ret_fiemap, &policy);
    diagfs_analysis_t        analysis = analyze_filesystem(&fs, &policy, &layout);

    /* ── 輸出 ── */
    if (out_format == DIAGFS_OUT_RAW) {
        /* 微調：補上 layout 參數 */
        print_out_raw(&fs, &analysis, &layout);
    } else if (out_format == DIAGFS_OUT_JSON) {
        printf("{\n  \"filesystems\": [\n");
        /* 微調：補上 layout 參數 */
        print_out_json(&fs, &analysis, &layout);
        printf("\n  ]\n}\n");
    } else {
        printf("diagfs - Filesystem Health Checker\n");
        printf("------------------------------------------\n");
        print_out_table(&fs, &analysis, &layout);
        
        if (analysis.inode_health == DIAGFS_HEALTH_CRITICAL) {
            /* 把 stderr 警告留在主流程中，因為這算是工具的特殊行為提示 */
            fprintf(stderr, "  [警告] inode 使用率已達危險等級（>= %lu%%），可能導致無法建立新檔案！\n",
                    policy.inode_critical_percent);
        }
    }

    return EXIT_OK;
}