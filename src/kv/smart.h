#if !defined(SMART_H)
#define SMART_H

#include <cstdlib>
#include <memory>

#include "Tree.h"
#include <city.h>

#include "helper.h"

template <typename K, typename V>
class Smart {
  using DSM = SMART::DSM;
  using DSMConfig = SMART::DSMConfig;
  using Tree = SMART::Tree;
public:
  Smart(const int cxl_percentage, const uint32_t num_threads) {
    DSMConfig config;
    config.machineNR = 1;
    config.threadNR = num_threads;
    dsm = DSM::getInstance(config);
    dsm->registerThread();
    tree = new Tree(dsm);
  }
  ~Smart() {
    delete tree;
  }

  void register_thread() {
    dsm->registerThread();
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
  Tree *tree;
  DSM* dsm;
};

template <typename K, typename V>
bool Smart<K, V>::get(const K &k, V &v, threadinfo* /* ti */, query<row_type>& /* q */,
           const uint32_t /* worker_id */) {
  uint64_t int_key = k.key;
  SMART::Key smart_key = SMART::int2key(int_key);
  SMART::Value value;
  bool ret = tree->search(smart_key, value, nullptr, 0);
  v.val = value;
  return ret;
}

template <typename K, typename V>
bool Smart<K, V>::insert(const K &k, const V& v, threadinfo* /* ti */,
                              query<row_type>& /* q */, const uint32_t /* worker_id */) {
  uint64_t int_key = k.key;
  SMART::Key smart_key = SMART::int2key(int_key);
  SMART::Value smart_value = v.val; 
  tree->insert(smart_key, smart_value, nullptr, 0);
  return true;
}

template <typename K, typename V>
bool Smart<K, V>::remove(const K & /* k */, threadinfo* /* ti */, query<row_type>& /* q */,
                          const uint32_t /* worker_id */) {
  return false;
}
#endif