#include "ptop.h"
#include <stdlib.h>
#include <string.h>

/*
 * Build parent-child tree (Toybox-like pstree model)
 */

static ptop_node_t *find_node(ptop_node_t *nodes, size_t n, pid_t pid)
{
    for (size_t i = 0; i < n; i++) {
        if (nodes[i].raw.pid == pid)
            return &nodes[i];
    }
    return NULL;
}

ptop_tree_t ptop_tree_build(ptop_snapshot_t *snap)
{
    ptop_tree_t tree;
    tree.count = snap->proc_count;
    tree.nodes = calloc(tree.count, sizeof(ptop_node_t));

    for (size_t i = 0; i < tree.count; i++) {
        tree.nodes[i].raw = snap->procs[i];
        tree.nodes[i].parent = NULL;
        tree.nodes[i].child_count = 0;
        tree.nodes[i].children = NULL;
    }

    /* build links */
    for (size_t i = 0; i < tree.count; i++) {
        pid_t ppid = tree.nodes[i].raw.ppid;

        ptop_node_t *parent = find_node(tree.nodes, tree.count, ppid);
        if (!parent) continue;

        parent->children = realloc(
            parent->children,
            sizeof(ptop_node_t*) * (parent->child_count + 1)
        );

        parent->children[parent->child_count++] = &tree.nodes[i];
        tree.nodes[i].parent = parent;
    }

    return tree;
}

void ptop_tree_free(ptop_tree_t *tree)
{
    if (!tree || !tree->nodes) return;

    for (size_t i = 0; i < tree->count; i++) {
        free(tree->nodes[i].children);
    }

    free(tree->nodes);
    tree->nodes = NULL;
    tree->count = 0;
}
