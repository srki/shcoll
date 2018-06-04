#include "trees.h"


void get_node_info_binomial(int tree_size, int node, node_info_binomial_t *node_info) {
    int mask;
    int child;

    node_info->parent = node == 0 ? -1 : node & (node - 1);

    /* Lowest bit */
    if (node == 0) {
        mask = 1 << (sizeof(int) * 8 - 2);
    } else {
        mask = node & ~(node - 1);
        mask >>= 1;
    }

    node_info->children_num = 0;

    while (mask != 0) {
        child = node | mask;
        if (child < tree_size) {
            node_info->children[node_info->children_num++] = child;
        }

        mask >>= 1;
    }
}


void get_node_info_knomial(int tree_size, int k, int node, node_info_knomial_t *node_info) {
    int left = 0;
    int right = tree_size;
    int parent = -1;
    int dist = -1, group;
    int child, children_num = 0;

    while (node != left) {
        parent = left;

        dist = (right - left + k - 1) / k;
        group = (node - left) / dist;

        int new_right = left + (group + 1) * dist;
        right = new_right > right ? right : new_right;
        left = left + group * dist;
    }

    node_info->parent = parent;

    while (dist != 0) {
        dist = (right - left + k - 1) / k;
        child = left + dist * ((right - left - 1) / dist);
        if (child == left) {
            break;
        }

        do {
            node_info->children[children_num++] = child;
            child -= dist;
        } while (child > left);

        right = left + dist;
    }

    node_info->children_num = children_num;
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