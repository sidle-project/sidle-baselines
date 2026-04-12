#if !defined(_GLOBAL_ALLOCATOR_H_)
#define _GLOBAL_ALLOCATOR_H_

#include "Common.h"
#include "cxl_allocator.h"
#include "Debug.h"
#include "GlobalAddress.h"

#include <cstring>

namespace SMART {

// global allocator for coarse-grained (chunck level) alloc 
// used by home agent
// bitmap based
class GlobalAllocator {

public:
#ifndef CXL
  GlobalAllocator(const GlobalAddress &start, size_t size)
      : start(start), size(size) {
    bitmap_len = size / define::kChunkSize;
    bitmap = new bool[bitmap_len];
    memset(bitmap, 0, bitmap_len);

    // null ptr
    bitmap[0] = true;
    bitmap_tail = 1;
  }
#endif

  GlobalAllocator(size_t size): size(size) {
    cxl_init(0, 100);
    void* start_ptr = nullptr;
    printf("[DEBUG] mmap size: %ld\n", size);
    int result = mmap_on_cxl(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0, 0, &start_ptr);
    if (result <= 0) {
      Debug::notifyError("mmap_on_cxl failed");
    }
    start.val = (uint64_t)start_ptr;
    bitmap_len = size / define::kChunkSize;
    bitmap = new bool[bitmap_len];
    memset(bitmap, 0, bitmap_len);

    // null ptr
    bitmap[0] = true;
    bitmap_tail = 1;
  }

  ~GlobalAllocator() { 
    delete[] bitmap; 
    munmap_on_cxl((void*)start.val, size);
  }

  GlobalAddress alloc_chunck() {
  #ifdef CXL
    GlobalAddress res;
    res.val = 0;
  #else
    GlobalAddress res = start;
  #endif
    if (bitmap_tail >= bitmap_len) {
      Debug::notifyError("shared memory space run out");
      throw std::runtime_error("[GlobalAllocator::alloc_chunck] shared memory space run out");
      // bitmap_tail = 1;
      // memset(bitmap, 0, bitmap_len);
      // bitmap[0] = true;
    }

    if (bitmap[bitmap_tail] == false) {
      bitmap[bitmap_tail] = true;
      res.offset += bitmap_tail * define::kChunkSize;

      bitmap_tail++;
    } else {
      // TODO
    }

    return res;
  }

  void free_chunk(const GlobalAddress &addr) {
    bitmap[(addr.offset - start.offset) / define::kChunkSize] = false;
  }

  uint64_t start_addr() {
    return start.val;
  }

private:
  GlobalAddress start;
  size_t size;

  bool *bitmap;
  size_t bitmap_len;
  size_t bitmap_tail;
};

} // namespace SMART

#endif
