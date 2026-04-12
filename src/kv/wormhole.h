#include <cstdlib>
#include <memory>
#include <set>

#include "helper.h"
#include "wh.h"

#if !defined(WORMHOLE_H)
#define WORMHOLE_H

template <typename K, typename V>
class WormholeKV {
  static const size_t key_size = sizeof(K);
  static const size_t val_size = sizeof(V);

  struct alignas(CACHELINE_SIZE) WorkerInfo {
    wormref *ref = nullptr;
  };

 public:
  WormholeKV(size_t worker_num, int cxl_percentage);
  ~WormholeKV();
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
  bool ref(const uint32_t worker_id);
  bool unref(const uint32_t worker_id);
  void worker_enter(const uint32_t worker_id);
  void worker_exit(const uint32_t worker_id);

 private:
  struct wormhole *const wh_ptr;
  std::vector<WorkerInfo> worker_info;
};

template <typename K, typename V>
WormholeKV<K, V>::WormholeKV(size_t worker_num, int cxl_percentage)
    : 
    wh_ptr(wormhole_create(NULL, cxl_percentage)), worker_info(worker_num) {}
    // wh_ptr(wormhole_create(NULL)), worker_info(worker_num) {}

template <typename K, typename V>
WormholeKV<K, V>::~WormholeKV() {
  for (size_t i = 0; i < worker_info.size(); ++i) {
    if (worker_info[i].ref != nullptr) {
      wormhole_unref(worker_info[i].ref);
    }
  }
  wormhole_destroy(wh_ptr);
}

template <typename K, typename V>
bool WormholeKV<K, V>::get(const K &k, V &v, threadinfo *ti, query<row_type> &q,
                           const uint32_t worker_id) {
  const K &str_k = k.to_wh_key();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size];
  struct kv *__kv = (struct kv *)kv_mem;
  // *(K *)(__kv->kv) = k;
  *(K *)(__kv->kv) = str_k;
  __kv->klen = key_size;
  __kv->vlen = 0;
  kv_update_hash(__kv);

  // query
  struct kv *res;
  const struct kref key_ref = kv_kref(__kv);
  // res is either NULL or a valid ptr
  if ((res = wormhole_get(ref, &key_ref, NULL))) {
    v = *(V *)&(res->kv[key_size]);
    #ifdef CXL
    free_with_cxl(res);
    #else
    free(res);
    #endif
  }

  return res ? true : false;
}

template <typename K, typename V>
bool WormholeKV<K, V>::insert(const K &k, const V &v, threadinfo *ti,
                              query<row_type> &q, const uint32_t worker_id) {
  const K &str_k = k.to_wh_key();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size + val_size];
  struct kv *__kv = (struct kv *)kv_mem;
  // *(K *)(__kv->kv) = k;
  *(K *)(__kv->kv) = str_k;
  *(V *)&(__kv->kv[key_size]) = v;
  __kv->klen = key_size;
  __kv->vlen = val_size;
  kv_update_hash(__kv);

  wormhole_put(ref, __kv);

  return true;
}

template <typename K, typename V>
bool WormholeKV<K, V>::remove(const K &k, threadinfo *ti, query<row_type> &q,
                              const uint32_t worker_id) {
  const K &str_k = k.to_wh_key();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size];
  struct kv *__kv = (struct kv *)kv_mem;
  // *(K *)(__kv->kv) = k;
  *(K *)(__kv->kv) = str_k;
  __kv->klen = key_size;
  __kv->vlen = 0;
  kv_update_hash(__kv);
  const struct kref key_ref = kv_kref(__kv);
  wormhole_del(ref, &key_ref);

  return true;
}

template <typename K, typename V>
bool WormholeKV<K, V>::lower_bound(K &k, V &v, threadinfo *ti,
                                   query<row_type> &q,
                                   const uint32_t worker_id) {
  COUT_N_EXIT("check correctness before use lower_bound. may key convertion");
  K &str_k = k.to_wh_key();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;
  struct wormhole_iter *const iter = wormhole_iter_create(ref);

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size];
  struct kv *__kv = (struct kv *)kv_mem;
  *(K *)__kv->kv = str_k;
  __kv->klen = key_size;
  __kv->vlen = 0;
  kv_update_hash(__kv);
  const struct kref key_ref = kv_kref(__kv);
  wormhole_iter_seek(iter, &key_ref);
  struct kv *res;
  if (res = wormhole_iter_next(iter, NULL)) {
    k = ((K *)res->kv)->to_normal_key();
    v = *(V *)&(res->kv[key_size]);
    #ifdef CXL
    free_with_cxl(res);
    #else 
    free(res);
    #endif
  }

  wormhole_iter_destroy(iter);
  return res ? true : false;
}

template <typename K, typename V>
size_t WormholeKV<K, V>::scan(const K &k_start, size_t n,
                              std::vector<std::pair<K, V>> &result,
                              threadinfo *ti, query<row_type> &q,
                              const uint32_t worker_id) {
  const K &str_k_start = k_start.to_wh_key();
  result.clear();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;
  struct wormhole_iter *const iter = wormhole_iter_create(ref);

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size];
  struct kv *__kv = (struct kv *)kv_mem;
  *(K *)__kv->kv = str_k_start;
  __kv->klen = key_size;
  __kv->vlen = 0;
  kv_update_hash(__kv);
  const struct kref key_ref = kv_kref(__kv);
  wormhole_iter_seek(iter, &key_ref);
  struct kv *res;
  size_t cnt = 0;
  while (cnt < n && (res = wormhole_iter_next(iter, NULL))) {
    result.emplace_back(((K *)(res->kv))->to_normal_key(),
                        *(V *)&(res->kv[key_size]));

    cnt++;
    #ifdef CXL
    free_with_cxl(res);
    #else
    free(res);
    #endif
  }

  wormhole_iter_destroy(iter);
  return result.size();
}

template <typename K, typename V>
size_t WormholeKV<K, V>::range_scan(const K &k_start, const K &k_end,
                                    std::vector<std::pair<K, V>> &result,
                                    threadinfo *ti, query<row_type> &q,
                                    const uint32_t worker_id) {
  COUT_N_EXIT("check correctness before use range_scan.");
  const K &str_k_start = k_start.to_wh_key();
  result.clear();
  assert(worker_id >= 0 && worker_id < worker_info.size());
  struct wormref *ref = worker_info[worker_id].ref;
  struct wormhole_iter *const iter = wormhole_iter_create(ref);

  // prepare stack memory storage for this kv struct
  uint8_t kv_mem[sizeof(struct kv) + key_size];
  struct kv *__kv = (struct kv *)kv_mem;
  *(K *)__kv->kv = str_k_start;
  __kv->klen = key_size;
  __kv->vlen = 0;
  kv_update_hash(__kv);
  const struct kref key_ref = kv_kref(__kv);
  wormhole_iter_seek(iter, &key_ref);
  struct kv *res;
  while (res = wormhole_iter_next(iter, NULL)) {
    K &k = *(K *)res->kv;
    if (k >= k_end) {
      break;
    }

    result.emplace_back(k.to_normal_key(), *(V *)&(res->kv[key_size]));

    #ifdef CXL
    free_with_cxl(res);
    #else
    free(res);
    #endif
  }

  wormhole_iter_destroy(iter);
  return result.size();
}

template <typename K, typename V>
bool WormholeKV<K, V>::ref(const uint32_t worker_id) {
  assert(worker_id >= 0 && worker_id < worker_info.size());
  assert(worker_info[worker_id].ref == nullptr);

  worker_info[worker_id].ref = wormhole_ref(wh_ptr);
  return true;
}

template <typename K, typename V>
bool WormholeKV<K, V>::unref(const uint32_t worker_id) {
  assert(worker_id >= 0 && worker_id < worker_info.size());
  assert(worker_info[worker_id].ref != nullptr);

  wormhole_unref(worker_info[worker_id].ref);
  worker_info[worker_id].ref = nullptr;
  return true;
}

template <typename K, typename V>
void WormholeKV<K, V>::worker_enter(const uint32_t worker_id) {
  assert(worker_id >= 0 && worker_id < worker_info.size());
  assert(worker_info[worker_id].ref == nullptr);

  worker_info[worker_id].ref = wormhole_ref(wh_ptr);
}

template <typename K, typename V>
void WormholeKV<K, V>::worker_exit(const uint32_t worker_id) {
  assert(worker_id >= 0 && worker_id < worker_info.size());
  assert(worker_info[worker_id].ref != nullptr);

  wormhole_unref(worker_info[worker_id].ref);
  worker_info[worker_id].ref = nullptr;
}

#endif  // WORMHOLE_H
