#include <cstdlib>
#include <memory>
#include <set>

#include "masstree_insert.hh"
#include "masstree_remove.hh"
#include "masstree_scan.hh"
#include "masstree_tcursor.hh"
#include "query_masstree.hh"

#include "helper.h"

#if !defined(MASSTREE_H)
#define MASSTREE_H

typedef Masstree::default_table mass_tree_t;

template <typename K, typename V>
class MasstreeKV {
  static const size_t key_size = sizeof(K);
  static const size_t val_size = sizeof(V);

 public:
  MasstreeKV(threadinfo *main_ti, const int cxl_percentage);
  void register_thread() {}
  bool get(const K &k, V &v, threadinfo *ti, query<row_type> &q,
           const uint32_t worker_id);
  bool insert(const K &k, const V &v, threadinfo *ti, query<row_type> &q,
              const uint32_t worker_id);
  bool remove(const K &k, threadinfo *ti, query<row_type> &q,
              const uint32_t worker_id);
  bool lower_bound(K &k, V &v, threadinfo *ti, query<row_type> &q,
                   const uint32_t worker_id);
  size_t scan(const K &k_start, size_t n, std::vector<std::pair<K, V>> &result,
              threadinfo *ti, query<row_type> &q, const uint32_t worker_id);
  size_t range_scan(const K &k_start, const K &k_end,
                    std::vector<std::pair<K, V>> &result, threadinfo *ti,
                    query<row_type> &q, const uint32_t worker_id);
  void worker_enter(const uint32_t worker_id);
  void worker_exit(const uint32_t worker_id);

  /// @brief function for tpc-c
  void start_bg();
  void terminate_bg();

 private:
  mass_tree_t mass_tree;
};

template <typename K, typename V>
MasstreeKV<K, V>::MasstreeKV(threadinfo *main_ti, const int cxl_percentage) {
  mass_tree.initialize(*main_ti, cxl_percentage);
}

template <typename K, typename V>
bool MasstreeKV<K, V>::get(const K &k, V &v, threadinfo *ti, query<row_type> &q,
                           const uint32_t worker_id) {
  K str_k = k.to_str_key();
  Str key((char *)&str_k, key_size);
  Str val_str;
  bool got = q.run_get1(mass_tree.table(), key, 0, val_str, *ti);
  if (got) {
    try {
      v = *((V *)val_str.s);
    } catch (const std::exception &e) {
      std::cerr << "Exception in MasstreeKV::get: " << e.what() << std::endl;
    }
    return true;
  } 
  return false;
}

template <typename K, typename V>
bool MasstreeKV<K, V>::insert(const K &k, const V &v, threadinfo *ti,
                              query<row_type> &q, const uint32_t worker_id) {
  K str_k = k.to_str_key();
  Str key((char *)&str_k, key_size);
  // Str key((char *)&k, key_size);
  Str val((char *)&v, val_size);
  result_t res = q.run_replace(mass_tree.table(), key, val, *ti);
  // result_t res = q.run_put(mass_tree.table(), key, val, *ti);
  assert(res == Inserted || res == Updated);
  // uint64_t result = 0;
  // for (int i = 0; i < 8; i++) {
  //     result = result << 8;
  //     result |= (uint8_t)((char *)&str_k)[i];
  // }
  // printf("masstree.insert: key %lx\n", result);
  return res == Inserted || res == Updated;
}

template <typename K, typename V>
bool MasstreeKV<K, V>::remove(const K &k, threadinfo *ti, query<row_type> &q,
                              const uint32_t worker_id) {
  K str_k = k.to_str_key();
  Str key((char *)&str_k, key_size);
  // Str key((char *)&k, key_size);
  q.run_remove(mass_tree.table(), key, *ti);
  return true;
}

template <typename K, typename V>
bool MasstreeKV<K, V>::lower_bound(K &k, V &v, threadinfo *ti,
                                   query<row_type> &q,
                                   const uint32_t worker_id) {
  COUT_N_EXIT("masstree.lower_bound: change key to masstree key before use");
  // std::vector<std::pair<K, V>> scan_result;
  // if (scan(k, 1, scan_result, ti, q, worker_id) > 0) {
  //   k = scan_result[0].first;
  //   v = scan_result[0].second;
  //   return true;
  // }
  return false;
}

// TODO masstree scan can be optimized
template <typename K, typename V>
size_t MasstreeKV<K, V>::scan(const K &k_start, size_t n,
                              std::vector<std::pair<K, V>> &result,
                              threadinfo *ti, query<row_type> &q,
                              const uint32_t worker_id) {
  K str_k = k_start.to_str_key();
  Str first_key((char *)&str_k, key_size);
  lcdf::Json req = lcdf::Json::array(0, 0, first_key, n);
  q.run_scan(mass_tree.table(), req, *ti);
  assert(req.size() >= 2);

  for (int i = 2; i < req.size(); i += 2) {
    result.emplace_back(((K *)req[i].as_s().data())->to_normal_key(),
                        *(V *)req[i + 1].as_s().data());
  }
  return result.size();
}

template <typename K, typename V>
size_t MasstreeKV<K, V>::range_scan(const K &k_start, const K &k_end,
                                    std::vector<std::pair<K, V>> &result,
                                    threadinfo *ti, query<row_type> &q,
                                    const uint32_t worker_id) {
  COUT_N_EXIT("masstree.range_scan: change key to masstree key before use");
  //   result.clear();
  //   // K mt_key_begin = k_start.to_str_key();
  //   // K mt_key_end = k_end.to_str_key();
  //   // Str key_start_((char *)&mt_key_begin, key_size);
  //   Str key_start_((char *)&k_start, key_size);

  //   lcdf::Json req = lcdf::Json::array(0, 0, key_start_);
  //   // RangeScanCallback<K, V, row_type> cb(mt_key_begin, mt_key_end,
  //   result); RangeScanCallback<K, V, row_type> cb(k_start, k_end, result);
  //   q.run_proxy_scan(mass_tree.table(), req, cb, *ti);

  //   return result.size();
  return 0;
}

template <typename K, typename V>
void MasstreeKV<K, V>::worker_enter(const uint32_t worker_id) {}

template <typename K, typename V>
void MasstreeKV<K, V>::worker_exit(const uint32_t worker_id) {}

template <typename K, typename V>
void MasstreeKV<K, V>::start_bg() {}

template <typename K, typename V>
void MasstreeKV<K, V>::terminate_bg() {}

#endif  // MASSTREE_H
