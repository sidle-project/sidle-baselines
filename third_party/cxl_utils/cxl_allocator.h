
#ifndef CXL_ALLOCATOR_H
#define CXL_ALLOCATOR_H

#include <sys/types.h>

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#endif

enum DEVICE_TYPE {
    CXL_DEV, 
    LOCAL_DEV,
    UNKNOWN_DEV
};

// for rw_mix_str with long key, change to 96GB
static const size_t CXL_MAX_SIZE = 1024ULL * 1024ULL * 1024ULL * 30ULL;

extern void 
cxl_init(const size_t wanted_size, const int percentage);

extern void *
malloc_with_cxl(size_t size);

extern void *
calloc_with_cxl(size_t num, size_t size);

extern void *
realloc_with_cxl(void *ptr, size_t new_size);

extern void 
free_with_cxl(void *ptr);

extern void *
mmap_with_cxl(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

extern void 
munmap_with_cxl(void * const ptr, const size_t size);

extern int
posix_memalign_with_cxl(void **memptr, size_t alignment, size_t size);

extern void *
get_mmap_start_point();

extern void *
get_mmap_end_point();

/// @return if allocate on cxl successfully, return true, otherwise return false
extern int
mmap_on_cxl(void *addr, size_t length, int prot, int flags, int fd, off_t offset, void **ptr);

extern void
munmap_on_cxl(void * const ptr, const size_t size);

extern void
cxl_destroy();

#ifdef __cplusplus
}
#endif

#endif  // CXL_ALLOCATOR_H