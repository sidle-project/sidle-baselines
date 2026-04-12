#if !defined(ART_H)
#define ART_H

#include <cstdlib>
#include <memory>

#include "art.h"

#include "helper.h"

template <typename K, typename V>
class ARTKV {
  static const size_t key_size = sizeof(K);
 
 public:
  ARTKV(const int cxl_percentage = 0);
  ~ARTKV();

  void register_thread() {}

  bool get(const K &k, V &v, threadinfo *ti, query<row_type> &q,
           const uint32_t worker_id);
  bool insert(const K &k, const V &v, threadinfo *ti, query<row_type> &q,
              const uint32_t worker_id);
  bool remove(const K &k, threadinfo *ti, query<row_type> &q,
              const uint32_t worker_id);
  bool lower_bound(K& /* k */, V& /* v */, threadinfo* /* ti */, query<row_type>& /* q */,
                   const uint32_t /* worker_id */) {
    return false;
  }
  size_t scan(const K& /* k_start */, size_t /* n */, std::vector<std::pair<K, V>>& /* result */,
              threadinfo* /* ti */, query<row_type>& /* q */, const uint32_t /* worker_id */) {
    return 0;
  }
  size_t range_scan(const K& /* k_start */, const K& /* k_end */,
                    std::vector<std::pair<K, V>>& /* result */, threadinfo* /* ti */,
                    query<row_type>& /* q */, const uint32_t /* worker_id */) {
    return 0;
  }
  void worker_enter(const uint32_t /* worker_id */) {};
  void worker_exit(const uint32_t /* worker_id */ ) {};

 private:
  adaptive_radix_tree *tree;
};

template <typename K, typename V>
ARTKV<K, V>::ARTKV(const int cxl_percentage):
  tree(new_adaptive_radix_tree(cxl_percentage)) {}

template <typename K, typename V>
ARTKV<K, V>::~ARTKV() {
  delete tree;
}

template <typename K, typename V>
bool ARTKV<K, V>::get(const K &k, V &v, threadinfo* /* ti */, query<row_type>& /* q */,
           const uint32_t /* worker_id */) {
  K str_k = k.to_str_key();
  void *value = adaptive_radix_tree_get(tree, (char *)&str_k, key_size);
  if (value == nullptr) {
    return false;
  }
  v = *((V *)value);
  return true;
}

template <typename K, typename V>
bool ARTKV<K, V>::insert(const K &k, const V& /* v */, threadinfo* /* ti */,
                              query<row_type>& /* q */, const uint32_t /* worker_id */) {
  K str_k = k.to_str_key();
  int ret = adaptive_radix_tree_put(tree, (char *)&str_k, key_size);
  if (ret >= 0) {
    return true;
  }
  return false;
}

template <typename K, typename V>
bool ARTKV<K, V>::remove(const K & /* k */, threadinfo* /* ti */, query<row_type>& /* q */,
                              const uint32_t /* worker_id */) {
  return false;
}

#endif  // ART_H