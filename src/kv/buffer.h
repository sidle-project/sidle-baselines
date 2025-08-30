#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "helper.h"
#include "masstree_insert.hh"
#include "masstree_remove.hh"
#include "masstree_scan.hh"
#include "masstree_tcursor.hh"
#include "query_masstree.hh"

#if !defined(BUFFER_H)
#define BUFFER_H

#define BS

template <class key_t, class val_t>
class BufferKV {
 public:
  BufferKV();
  BufferKV(const std::vector<key_t> keys, const std::vector<val_t> vals,
           const size_t capacity);
  ~BufferKV();
  inline bool get(const key_t &k, val_t &v, threadinfo *ti, query<row_type> &q,
                  const uint32_t worker_id);
  inline bool insert(const key_t &k, const val_t &v, threadinfo *ti,
                     query<row_type> &q, const uint32_t worker_id);
  inline bool remove(const key_t &k, threadinfo *ti, query<row_type> &q,
                     const uint32_t worker_id);
  inline bool scan(std::vector<std::pair<key_t, val_t>> &result, threadinfo *ti,
                   query<row_type> &q, const uint32_t worker_id);

  inline bool get(const key_t &k, val_t &v);
  inline size_t get_pos(const key_t &k);
  inline bool insert(const key_t &k, const val_t &v);
  inline bool insert_if_not_exst(const key_t &k, const val_t &v);
  inline bool remove(const key_t &k);
  inline bool scan_keys(std::vector<key_t> &result);
  inline size_t size();

 private:
  inline bool binary_search(const key_t &k, size_t &pos);
  inline bool linear_search(const key_t &k, size_t &pos);

  size_t record_n;
  size_t capacity_n;
  std::unique_ptr<key_t[]> keys;
  std::unique_ptr<val_t[]> vals;
  pthread_rwlock_t lock;
  // RWLock rwlock;
};

template <class key_t, class val_t>
BufferKV<key_t, val_t>::BufferKV() {
  capacity_n = 1024;
  record_n = 0;
  keys = std::unique_ptr<key_t[]>(new key_t[capacity_n]);
  vals = std::unique_ptr<val_t[]>(new val_t[capacity_n]);
  int res = pthread_rwlock_init(&lock, NULL);
  if (res != 0) {
    COUT_THIS("arealock initialization failed!!\n");
    exit(-1);
  }
}

template <class key_t, class val_t>
BufferKV<key_t, val_t>::BufferKV(const std::vector<key_t> init_keys,
                                 const std::vector<val_t> init_vals,
                                 const size_t capacity) {
  capacity_n = capacity < init_keys.size() ? init_keys.size() : capacity;
  record_n = init_keys.size();
  keys = std::unique_ptr<key_t[]>(new key_t[capacity_n]);
  vals = std::unique_ptr<val_t[]>(new val_t[capacity_n]);
  for (size_t i = 0; i < init_keys.size(); i++) {
    keys[i] = init_keys[i];
    vals[i] = init_vals[i];
  }
  // int res = pthread_rwlock_init(&lock, NULL);
  // if (res != 0) {
  //   COUT_THIS("arealock initialization failed!!\n");
  //   exit(-1);
  // }
}

template <class key_t, class val_t>
BufferKV<key_t, val_t>::~BufferKV() {
  int res = pthread_rwlock_destroy(&lock);
  if (res != 0) {
    COUT_THIS("arealock destroy failed!!\n");
    exit(-1);
  }
}

template <class key_t, class val_t>
inline size_t BufferKV<key_t, val_t>::size() {
  return record_n;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::get(const key_t &k, val_t &v,
                                        threadinfo *ti, query<row_type> &q,
                                        const uint32_t worker_id) {
  return get(k, v);
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::insert(const key_t &k, const val_t &v,
                                           threadinfo *ti, query<row_type> &q,
                                           const uint32_t worker_id) {
  return insert(k, v);
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::remove(const key_t &k, threadinfo *ti,
                                           query<row_type> &q,
                                           const uint32_t worker_id) {
  return remove(k);
}

template <class key_t, class val_t>
inline size_t BufferKV<key_t, val_t>::get_pos(const key_t &k) {
  size_t pos;
#ifdef BS
  bool match = binary_search(k, pos);
#else
  bool match = linear_search(k, pos);
#endif
  assert(match);
  return pos;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::get(const key_t &k, val_t &v) {
  pthread_rwlock_rdlock(&lock);
  size_t pos;
#ifdef BS
  bool match = binary_search(k, pos);
#else
  bool match = linear_search(k, pos);
#endif
  if (match) {
    v = vals[pos];
  }

  pthread_rwlock_unlock(&lock);
  return match;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::insert_if_not_exst(const key_t &k,
                                                       const val_t &v) {
  pthread_rwlock_wrlock(&lock);
  assert(record_n <= capacity_n);
  size_t pos;
#ifdef BS
  bool match = binary_search(k, pos);
#else
  bool match = linear_search(k, pos);
#endif
  assert(pos >= 0 && pos <= capacity_n);

  if (!match) {
    if (record_n == capacity_n) {
      size_t new_capacity = capacity_n >> 2;

      std::unique_ptr<key_t[]> new_keys =
          std::unique_ptr<key_t[]>(new key_t[new_capacity]);
      if (unlikely(new_keys.get() == nullptr)) {
        COUT_THIS("failed to resize keys from " << capacity_n << " to "
                                                << new_capacity);
        return false;
      }
      std::unique_ptr<val_t[]> new_vals =
          std::unique_ptr<val_t[]>(new val_t[new_capacity]);
      if (unlikely(new_vals.get() == nullptr)) {
        COUT_THIS("failed to resize vals from " << capacity_n << " to "
                                                << new_capacity);
        return false;
      }

      memcpy(new_keys.get(), keys.get(), pos * sizeof(key_t));
      memcpy(new_vals.get(), vals.get(), pos * sizeof(val_t));
      memcpy(new_keys.get() + pos + 1, keys.get() + pos,
             (record_n - pos) * sizeof(key_t));
      memcpy(new_vals.get() + pos + 1, vals.get() + pos,
             (record_n - pos) * sizeof(val_t));

      new_keys[pos] = k;
      new_vals[pos] = v;

      keys = std::move(new_keys);
      vals = std::move(new_vals);

      capacity_n = new_capacity;
      record_n++;

    } else {
      memmove(keys.get() + pos + 1, keys.get() + pos,
              (record_n - pos) * sizeof(key_t));
      memmove(vals.get() + pos + 1, vals.get() + pos,
              (record_n - pos) * sizeof(val_t));
      keys[pos] = k;
      vals[pos] = v;
      record_n++;
    }
  }

  pthread_rwlock_unlock(&lock);
  return !match;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::insert(const key_t &k, const val_t &v) {
  pthread_rwlock_wrlock(&lock);
  assert(record_n <= capacity_n);
  size_t pos;
#ifdef BS
  bool match = binary_search(k, pos);
#else
  bool match = linear_search(k, pos);
#endif
  assert(pos >= 0 && pos <= capacity_n);

  if (match) {
    vals[pos] = v;
  } else {
    if (record_n == capacity_n) {
      size_t new_capacity = capacity_n >> 2;

      std::unique_ptr<key_t[]> new_keys =
          std::unique_ptr<key_t[]>(new key_t[new_capacity]);
      if (unlikely(new_keys.get() == nullptr)) {
        COUT_THIS("failed to resize keys from " << capacity_n << " to "
                                                << new_capacity);
        return false;
      }
      std::unique_ptr<val_t[]> new_vals =
          std::unique_ptr<val_t[]>(new val_t[new_capacity]);
      if (unlikely(new_vals.get() == nullptr)) {
        COUT_THIS("failed to resize vals from " << capacity_n << " to "
                                                << new_capacity);
        return false;
      }

      memcpy(new_keys.get(), keys.get(), pos * sizeof(key_t));
      memcpy(new_vals.get(), vals.get(), pos * sizeof(val_t));
      memcpy(new_keys.get() + pos + 1, keys.get() + pos,
             (record_n - pos) * sizeof(key_t));
      memcpy(new_vals.get() + pos + 1, vals.get() + pos,
             (record_n - pos) * sizeof(val_t));

      new_keys[pos] = k;
      new_vals[pos] = v;

      keys = std::move(new_keys);
      vals = std::move(new_vals);

      capacity_n = new_capacity;
      record_n++;

    } else {
      memmove(keys.get() + pos + 1, keys.get() + pos,
              (record_n - pos) * sizeof(key_t));
      memmove(vals.get() + pos + 1, vals.get() + pos,
              (record_n - pos) * sizeof(val_t));
      keys[pos] = k;
      vals[pos] = v;
      record_n++;
    }
  }

  pthread_rwlock_unlock(&lock);
  return true;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::remove(const key_t &k) {
  pthread_rwlock_wrlock(&lock);

  assert(record_n <= capacity_n);
  size_t pos;
#ifdef BS
  bool match = binary_search(k, pos);
#else
  bool match = linear_search(k, pos);
#endif
  if (match) {
    memmove(keys.get() + pos, keys.get() + pos + 1,
            (record_n - pos - 1) * sizeof(key_t));
    memmove(vals.get() + pos, vals.get() + pos + 1,
            (record_n - pos - 1) * sizeof(val_t));
    record_n--;
  }

  pthread_rwlock_unlock(&lock);
  return match;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::scan_keys(std::vector<key_t> &result) {
  for (size_t i = 0; i < record_n; i++) {
    result.push_back(keys[i]);
  }
  return true;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::binary_search(const key_t &k, size_t &pos) {
  size_t search_begin = 0, search_end = record_n, mid = record_n / 2;
  while (search_end > search_begin) {
    if (keys[mid] < k) {
      search_begin = mid + 1;
    } else {
      search_end = mid;
    }

    mid = (search_begin + search_end) / 2;
  }
  assert(search_begin == search_end);
  assert(search_begin == mid);
  pos = mid;
  bool match = pos < record_n && keys[pos] == k;
  assert(!match || (match && pos >= 0 && pos < record_n));
  return match;
}

template <class key_t, class val_t>
inline bool BufferKV<key_t, val_t>::linear_search(const key_t &k, size_t &pos) {
  for (size_t i = 0; i < record_n; i++) {
    if (k == keys[i]) {
      pos = i;
      return true;
    }
  }

  pos = record_n;
  return false;
}

#endif  // BUFFER_H
