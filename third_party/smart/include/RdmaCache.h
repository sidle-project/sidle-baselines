#if !defined(_CACHE_H_)
#define _CACHE_H_

#include "Config.h"
#include "HugePageAlloc.h"

namespace SMART {

class Cache {

public:
  Cache(const CacheConfig &cache_config);
  ~Cache();

  uint64_t data;
  uint64_t size;

private:
};

} // namespace SMART

#endif // _CACHE_H_
