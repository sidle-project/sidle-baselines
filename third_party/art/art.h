/**
 *    author:     UncP
 *    date:    2019-02-16
 *    license:    BSD-3
**/

#ifndef _adapitve_radix_tree_h_
#define _adaptive_radix_tree_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "art_node.h"

#include <stddef.h>

struct adaptive_radix_tree
{
  art_node *root;
};

typedef struct adaptive_radix_tree adaptive_radix_tree;

adaptive_radix_tree* new_adaptive_radix_tree(int cxl_percentage);
void free_adaptive_radix_tree(adaptive_radix_tree *art);
int adaptive_radix_tree_put(adaptive_radix_tree *art, const void *key, size_t len);
void* adaptive_radix_tree_get(adaptive_radix_tree *art, const void *key, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _adaptive_radix_tree_h_ */
