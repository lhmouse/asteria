// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xmemory.hpp"
#include "mutex.hpp"
#include "atomic.hpp"
namespace rocket {
namespace {

struct block
  {
    block* next;
    size_t count;
  };

struct pool
  {
    alignas(64) mutex m;
    atomic_relaxed<block*> head;
  };

pool s_pools[64];

inline
pool&
do_get_pool_for_size(size_t& rsize)
  {
    uint64_t si64 = ::std::max(sizeof(block), rsize);
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

    auto& p = do_get_pool_for_size(rsize);
    block* b = nullptr;
    mutex::unique_lock lock;

    if(opt == xmemopt_use_cache) {
      // Try getting a block from the cache.
      if(ROCKET_EXPECT(p.head.load() != nullptr)) {
        lock.lock(p.m);
        b = p.head.load();
        if(ROCKET_EXPECT(b != nullptr))
          p.head.store(b->next);
      }
    }
    lock.unlock();

    // If the cache was empty, allocate a block from the system.
    if(b == nullptr)
      b = (block*) ::operator new(rsize);

    info.data = b;
    info.count = (info.element_size > 1) ? (rsize / info.element_size) : rsize;
#ifdef ROCKET_DEBUG
    ::memset(info.data, 0xB5, rsize);
#endif
  }

void
xmemfree(xmeminfo& info, xmemopt opt) noexcept
  {
    if(!info.data)
      return;

    size_t rsize = info.element_size * info.count;
    auto& p = do_get_pool_for_size(rsize);
#ifdef ROCKET_DEBUG
    ::memset(info.data, 0xCB, rsize);
#endif
    block* b = (block*) info.data;
    b->next = nullptr;
    b->count = 1;
    mutex::unique_lock lock;

    if(opt == xmemopt_use_cache) {
      // Put the block into the cache.
      lock.lock(p.m);
      b->next = p.head.load();
      if(b->next)
        b->count = b->next->count + 1;
      p.head.store(b);
      b = nullptr;
    }
    else if(opt == xmemopt_clear_cache) {
      // Append all blocks from the cache to `b`.
      lock.lock(p.m);
      b->next = p.head.load();
      p.head.store(nullptr);
    }
    lock.unlock();

    // Return all blocks to the system.
    while(b)
      ::operator delete(exchange(b, b->next));
  }

void
xmemflush() noexcept
  {
    mutex::unique_lock lock;
    block* b = nullptr;

    for(auto& p : s_pools) {
      lock.lock(p.m);
      b = p.head.load();
      p.head.store(nullptr);
      lock.unlock();

      // Return all blocks to the system.
      while(b)
        ::operator delete(exchange(b, b->next));
    }
  }

}  // namespace rocket
