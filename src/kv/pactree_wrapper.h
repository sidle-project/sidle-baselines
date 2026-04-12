#if !defined(PACTREE_H)
#define PACTREE_H

#include "helper.h"
#include "pactree.h"

/// @note only support uint64_t here 
template <typename K, typename V>
class PACTreeKV {

public:
  explicit PACTreeKV(const int cxl_percentage = 0);
  ~PACTreeKV();

  void register_thread() {
    pt->registerThread();
  }

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
  pactree *pt;
};

template <typename K, typename V>
PACTreeKV<K, V>::PACTreeKV(const int cxl_percentage):
  pt(new pactree(1, cxl_percentage)) {
  pt->registerThread();
}

template <typename K, typename V>
PACTreeKV<K, V>::~PACTreeKV() {
  pt->unregisterThread();
  delete pt;
}

template <typename K, typename V>
bool PACTreeKV<K, V>::get(const K &k, V &v, threadinfo* /* ti */, 
                  query<row_type>& /* q */, const uint32_t /* worker_id */) {
  uint64_t int_key = k.key;
  auto val = pt->lookup(int_key);
  v.val = val;
  return true;
}

template <typename K, typename V>
bool PACTreeKV<K, V>::insert(const K &k, const V &v, threadinfo* /* ti */, 
                  query<row_type>& /* q */, const uint32_t /* worker_id */) {
  uint64_t int_key = k.key;
  pt->insert(int_key, v.val);
  return true;
}

template <typename K, typename V>
bool PACTreeKV<K, V>::remove(const K &k, threadinfo* /* ti */, 
                  query<row_type>& /* q */, const uint32_t /* worker_id */) {
  uint64_t int_key = k.key;
  pt->remove(int_key);
  return true;
}

#endif