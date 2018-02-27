#ifndef OPENSHMEM_COLLECTIVE_ROUTINES_TREES_H
#define OPENSHMEM_COLLECTIVE_ROUTINES_TREES_H

#include <limits.h>

struct _node_info_binomial {
    int parent;
    int children_num;
    int children[sizeof(int) * CHAR_BIT];
};

typedef struct _node_info_binomial node_info_binomial_t;

void get_node_info_binomial(int tree_size, int node, node_info_binomial_t *node_info);


struct _node_info_complete {
    int parent;
    int children_num;
    int children_begin;
    int children_end;
};

typedef struct _node_info_complete node_info_complete_t;

void get_node_info_complete(int tree_size, int tree_degree, int node, node_info_complete_t *node_info);

#endif /* OPENSHMEM_COLLECTIVE_ROUTINES_TREES_H */
