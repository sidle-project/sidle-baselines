#include "cxl_cpp_allocator.hh"

void *
operator new(std::size_t size)
{
#ifdef CXL
    return malloc_with_cxl(size);
#else
    ::operator new(size);
#endif
}

void *
operator new[](std::size_t size)
{
    printf("operator new size: %lu\n", size);
#ifdef CXL
    return malloc_with_cxl(size);
#else
    ::operator new[](size);
#endif
}

void 
operator delete(void* ptr) noexcept
{
#ifdef CXL
    free_with_cxl(ptr);
#else 
    ::operator delete(ptr);
#endif
}

void
operator delete[](void* ptr) noexcept
{
#ifdef CXL
    free_with_cxl(ptr);
#else
    ::operator delete[](ptr);
#endif
}

bool 
is_remote_node(void* ptr)
{
#ifdef CXL
    return false;
    // return ptr >= get_mmap_start_point() && ptr < get_mmap_end_point();
#else
    return false;
#endif
}
