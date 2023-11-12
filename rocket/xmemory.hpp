// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XMEMORY_
#define ROCKET_XMEMORY_

#include "fwd.hpp"
#include "xassert.hpp"
namespace rocket {

struct xmeminfo
  {
    size_t element_size;
    void* data;
    size_t count;
  };

enum xmemopt : uint32_t
  {
    xmemopt_use_cache     = 0,
    xmemopt_bypass_cache  = 1,
    xmemopt_clear_cache   = 2,
  };

// Allocates a block of memory. The caller shall initialize `element_size`
// and `count` before calling this function. After a successful return,
// `data` will point to a block of memory of at least `element_size * count`
// bytes. `count` may be updated to reflect the actual number of allocated
// elements, which will be no less than the request.
void
xmemalloc(xmeminfo& info, xmemopt opt = xmemopt_use_cache);

// Frees a block of memory. `info` must be the result of a previous call
// to `xmemalloc()`. After this function returns, `data` will be set to a
// null pointer and `count` will be set to zero.
void
xmemfree(xmeminfo& info, xmemopt opt = xmemopt_use_cache) noexcept;

// Clears the global cache.
void
xmemflush() noexcept;

// Copies a block into another, with some checking.
inline
void
xmemcopy(xmeminfo& info, const xmeminfo& src) noexcept
  {
    ROCKET_ASSERT(info.count >= src.count);
    if(src.count != 0)
      ::std::memcpy(info.data, src.data, src.element_size * src.count);
  }

// Zero-initializes a block.
inline
void
xmemzero(xmeminfo& info) noexcept
  {
    if(info.count != 0)
      ::std::memset(info.data, 0, info.element_size * info.count);
  }

}  // namespace rocket
#endif
