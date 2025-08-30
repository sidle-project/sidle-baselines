/**
 *    author:     UncP
 *    date:    2019-02-05
 *    license:    BSD-3
**/

#ifndef _art_node_h_
#define _art_node_h_

#include <stddef.h>

#ifdef Debug
#include <assert.h>
#define debug_assert_art(v) assert(v)
#else
#define debug_assert_art(v)
#endif // Debug

#define fuck printf("fuck\n");

// #define CXL

#ifdef CXL
#include "../cxl_utils/cxl_allocator.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

struct art_node;
typedef struct art_node art_node;

#define art_node_header \
  uint64_t version;     \
  char prefix[8];       \
  art_node *new_;        \
  art_node *parent;

// 32 bytes
struct art_node
{
  art_node_header;
};

typedef struct art_node4
{
  art_node_header;
  unsigned char key[4];
  unsigned char unused[4];
  art_node *child[4];
  char meta[0];
}art_node4;

typedef struct art_node16
{
  art_node_header;
  unsigned char key[16];
  art_node *child[16];
  char meta[0];
}art_node16;

typedef struct art_node48
{
  art_node_header;
  unsigned char index[256];
  art_node *child[48];
  char meta[0];
}art_node48;

typedef struct art_node256
{
  uint64_t version;
  char prefix[8];
  art_node *new_;
  art_node *parent;
  art_node *child[256];
  char meta[0];
}art_node256;

#define is_leaf(ptr) ((uintptr_t)(ptr) & 1)
#define make_leaf(ptr) ((uintptr_t)((const char *)(ptr) - 1) | 1)
#define get_leaf_key(ptr) (((const char *)((uintptr_t)(ptr) & (~(uintptr_t)1))) + 1)
#define get_leaf_len(ptr) ((size_t)*(char *)((uintptr_t)(ptr) & (~(uintptr_t)1)))

art_node* new_art_node();
uintptr_t alloc_leaf(const char* key, size_t len);
void free_art_node(art_node *an);
art_node** art_node_add_child(art_node *an, unsigned char byte, art_node *child, art_node **new_);
art_node** art_node_find_child(art_node *an, uint64_t version, unsigned char byte);
int art_node_is_full(art_node *an);
void art_node_set_prefix(art_node *an, const void *key, size_t off, int prefix_len);
const char* art_node_get_prefix(art_node *an);
int art_node_prefix_compare(art_node *an, uint64_t version, const void *key, size_t len, size_t off);
unsigned char art_node_truncate_prefix(art_node *an, int off);
uint64_t art_node_get_version(art_node *an);
uint64_t art_node_get_version_unsafe(art_node *an);
uint64_t art_node_get_stable_expand_version(art_node *an);
// uint64_t art_node_get_stable_insert_version(art_node *an);
int art_node_version_get_prefix_len(uint64_t version);
int art_node_version_compare_expand(uint64_t version1, uint64_t version2);
// int art_node_version_compare_insert(uint64_t version1, uint64_t version2);
int art_node_lock(art_node *an);
art_node* art_node_get_locked_parent(art_node *an);
void art_node_set_parent_unsafe(art_node *an, art_node *parent);
void art_node_unlock(art_node *an);
int art_node_version_is_old(uint64_t version);
art_node* art_node_replace_leaf_child(art_node *an, const void *key, size_t len, size_t off);
void art_node_replace_child(art_node *parent, unsigned char byte, art_node *old, art_node *new_);
art_node* art_node_expand_and_insert(art_node *an, const void *key, size_t len, size_t off, int common);
size_t art_node_version_get_offset(uint64_t version);

#ifdef Debug
void art_node_print(art_node *an);
void print_key(const void *key, size_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _art_node_h_ */
