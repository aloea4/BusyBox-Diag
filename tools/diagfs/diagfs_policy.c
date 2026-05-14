#include "diagfs.h"

/* ── 取得預設政策 ── */
diagfs_policy_t get_default_policy(void)
{
    diagfs_policy_t policy = {
        .space_warn_percent     = 70,
        .space_critical_percent = 90,
        .inode_warn_percent     = 70,
        .inode_critical_percent = 90,
        .extent_warn_count      = 10,
        .extent_critical_count  = 50
    };
    return policy;
}
