// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xmemory.hpp"
#include "mutex.hpp"
namespace rocket {
namespace {

struct free_block
  {
    size_t size;
    free_block* next;
  };

struct alignas(64) pool
  {
    mutex m;
    free_block* head;
  };

pool s_pools[64];

inline
pool&
do_get_pool_for_size(size_t& rsize)
  {
    uint64_t si64 = ::std::max(sizeof(free_block), rsize);
    uint32_t i = 64U - (uint32_t) ROCKET_LZCNT64(si64 - 1ULL);
    si64 = 1ULL << i;
    ROCKET_ASSERT(si64 >= rsize);
    rsize = (size_t) si64;
    return s_pools[i];
  }

}  // namespace

void
xmemalloc(xmeminfo& info, xmemopt opt)
  {
    size_t rsize;
    if(ROCKET_MUL_OVERFLOW(info.element_size, info.count, &rsize))
      throw ::std::bad_alloc();

    free_block* b = nullptr;
    auto& p = do_get_pool_for_size(rsize);
    mutex::unique_lock lock;

    if(opt == xmemopt_use_cache) {
      // Get a block from the cache.
      lock.lock(p.m);
      if(p.head != nullptr)
        b = exchange(p.head, p.head->next);
    }
    lock.unlock();

    // If the cache was empty, allocate a block from the system.
    if(b == nullptr)
      b = (free_block*) ::operator new(rsize);

#ifdef ROCKET_DEBUG
    ::memset(b, 0xB5, rsize);
#endif
    info.data = b;
    info.count = (info.element_size > 1) ? (rsize / info.element_size) : rsize;
  }

void
xmemfree(xmeminfo& info, xmemopt opt) noexcept
  {
    free_block* b = (free_block*) info.data;
    if(b == nullptr)
      return;

    size_t rsize = info.element_size * info.count;
    auto& p = do_get_pool_for_size(rsize);
    mutex::unique_lock lock;

#ifdef ROCKET_DEBUG
    ::memset(b, 0xCB, rsize);
#endif
    b->size = rsize;
    b->next = nullptr;

    if(opt == xmemopt_use_cache) {
      // Put the block into the cache.
      lock.lock(p.m);
      b->next = exchange(p.head, b);
      b = nullptr;
    }
    else if(opt == xmemopt_clear_cache) {
      // Append all blocks from the cache to `b`.
      lock.lock(p.m);
      b->next = exchange(p.head, nullptr);
    }
    lock.unlock();

    // Return all blocks to the system.
    while(b != nullptr)
      ::operator delete(exchange(b, b->next));
  }

void
xmemflush() noexcept
  {
    for(auto& p : s_pools) {
      // Extract all blocks.
      mutex::unique_lock lock(p.m);
      free_block* b = exchange(p.head, nullptr);
      lock.unlock();

      // Return all blocks to the system.
      while(b != nullptr)
        ::operator delete(exchange(b, b->next));
    }
  }

}  // namespace rocket
