// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "xmemory.hpp"
#include "atomic.hpp"
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
namespace rocket {
namespace {

struct free_block
  {
    free_block* next;
    size_t size;
  };

struct alignas(64) pool
  {
    atomic_acq_rel<free_block*> head;
  };

pool s_pools[64];

inline
pool&
do_get_pool_for_size(size_t& size)
  {
    uint32_t si = 5;
    uint64_t rsize64 = 1ULL << si;

    if(size > rsize64) {
      // Round `size` up to the nearest power of two.
      si = (uint32_t) (64 - ROCKET_LZCNT64(size - 1ULL));
      rsize64 = 1ULL << si;
    }

    ROCKET_ASSERT(size <= rsize64);
    size = (size_t) rsize64;
    return s_pools[si];
  }

void
do_deallocate_list(free_block* head) noexcept
  {
    free_block* next;
    for(free_block* p = head;  p != nullptr;  p = next) {
      next = p->next;
      ::operator delete(p);
    }
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

    if(opt == xmemopt_use_cache) {
      // Get a block from the cache.
      b = p.head.xchg(nullptr);
      if(ROCKET_EXPECT(b != nullptr) && (b->next != nullptr))
        b->next = p.head.xchg(b->next);
    }
    else if(opt == xmemopt_clear_cache) {
      // Extract all blocks.
      b = p.head.xchg(nullptr);
    }

    // If the cache was empty, allocate a block from the system.
    if(b == nullptr)
      b = (free_block*) ::operator new(rsize);
    else
      do_deallocate_list(b->next);

#ifdef __SANITIZE_ADDRESS__
    ::__asan_unpoison_memory_region(&(b->next) + 1, rsize - sizeof(b->next));
#endif
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

#ifdef ROCKET_DEBUG
    ::memset(b, 0xCB, rsize);
#endif
#ifdef __SANITIZE_ADDRESS__
    ::__asan_poison_memory_region(&(b->next) + 1, rsize - sizeof(b->next));
#endif

    b->next = nullptr;
    b->size = rsize;

    if(opt == xmemopt_use_cache) {
      // Put the block into the cache.
      b->next = p.head.load();
      while(!p.head.cmpxchg_weak(b->next, b));
      b = nullptr;
    }
    else if(opt == xmemopt_clear_cache) {
      // Append all blocks from the cache to `b`.
      b->next = p.head.xchg(nullptr);
    }

    // Return all blocks to the system.
    do_deallocate_list(b);
  }

void
xmemclean() noexcept
  {
    for(auto& p : s_pools) {
      // Extract all blocks and return them to the system.
      free_block* b = p.head.xchg(nullptr);
      do_deallocate_list(b);
    }
  }

}  // namespace rocket
