//
// Created by Srđan Milaković on 2/8/18.
//

#include "trees.h"

void get_node_info_binomial(int tree_size, int node, node_info_binomial_t *node_info) {
    node_info->parent = node == 0 ? -1 : node & (node - 1);

    // Lowest bit
    int mask;
    if (node == 0) {
        mask = 1 << (sizeof(int) * 8 - 2);
    } else {
        mask = node & ~(node - 1);
        mask >>= 1;
    }

    node_info->children_num = 0;

    while (mask != 0) {
        int child = node | mask;
        if (child < tree_size) {
            node_info->children[node_info->children_num++] = child;
        }

        mask >>= 1;
    }
}

void get_node_info_complete(int tree_size, int tree_degree, int node, node_info_complete_t *node_info) {
    node_info->parent = node != 0 ? (node - 1) / tree_degree : -1;
    node_info->children_begin = node * tree_degree + 1;

    if (node_info->children_begin >= tree_size) {
        node_info->children_begin = node_info->children_end = -1;
        node_info->children_num = 0;
        return;
    }

    node_info->children_end = node_info->children_begin + tree_degree;

    if (node_info->children_end > tree_size) {
        node_info->children_end = tree_size;
    }

    node_info->children_num = node_info->children_end - node_info->children_begin;
}