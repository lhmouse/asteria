// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xmemory.hpp"
namespace rocket {
namespace {

struct free_block
  {
    free_block* next;
    void* padding;
  };

struct alignas(64) pool
  {
    free_block* volatile ptr_stor[2];  // head, tail

    bool
    compare_exchange(free_block*& head_cmp, free_block*& tail_cmp, free_block* head_set,
                     free_block* tail_set) noexcept
      {
        bool equal;
#if defined(__x86_64__) && (__SIZEOF_POINTER__ != 4)
        // This is for x86-64 and not x32
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
        __asm__ (
          "lock cmpxchg16b %1"
          : "=@cce"(equal), "+m"(this->ptr_stor), "+a"(head_cmp), "+d"(tail_cmp)
          : "b"(head_set), "c"(tail_set)
          : "memory"
        );
#else
        // This is for x86, x32, ARM, etc.
        free_block* stor_cmp[2] = { head_cmp, tail_cmp };
        free_block* stor_set[2] = { head_set, tail_set };
        equal = __atomic_compare_exchange(&(this->ptr_stor), &stor_cmp, &stor_set,
                                  1 /* weak */, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
        head_cmp = stor_cmp[0];
        tail_cmp = stor_cmp[1];
#endif
        return equal;
      }
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

    if(opt == xmemopt_use_cache) {
      // Detach all blocks from the cache, then leave the first and put all
      // the others back.
      free_block* tail = nullptr;
      while(!p.compare_exchange(b, tail, nullptr, nullptr));

      free_block* t = exchange(tail, nullptr);
      if(ROCKET_EXPECT(b != t))
        while(!p.compare_exchange(t->next, tail, b->next, tail ? tail : t));
    }

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

#ifdef ROCKET_DEBUG
    ::memset(b, 0xCB, rsize);
#endif
    b->next = nullptr;

    if(opt == xmemopt_use_cache) {
      // Put the block into the cache.
      free_block* tail = nullptr;
      while(!p.compare_exchange(b->next, tail, b, tail ? tail : b));
      b = nullptr;
    }
    else if(opt == xmemopt_clear_cache) {
      // Append all blocks from the cache to `b`.
      free_block* tail = nullptr;
      while(!p.compare_exchange(b->next, tail, nullptr, nullptr));
    }

    // Return all blocks to the system.
    while(b != nullptr)
      ::operator delete(exchange(b, b->next));
  }

void
xmemflush() noexcept
  {
    for(auto& p : s_pools) {
      free_block* b = nullptr;

      // Append all blocks from the cache to `b`.
      free_block* tail = nullptr;
      while(!p.compare_exchange(b, tail, nullptr, nullptr));

      // Return all blocks to the system.
      while(b != nullptr)
        ::operator delete(exchange(b, b->next));
    }
  }

}  // namespace rocket
