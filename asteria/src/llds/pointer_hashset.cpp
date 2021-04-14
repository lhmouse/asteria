// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "pointer_hashset.hpp"
#include "../utils.hpp"

namespace asteria {

void
Pointer_HashSet::
do_destroy_buckets() noexcept
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = next;
      next = qbkt->next;

      // Destroy this bucket.
      ROCKET_ASSERT(*qbkt);
      qbkt->prev = nullptr;
    }
#ifdef ROCKET_DEBUG
    ::std::for_each(this->m_bptr, this->m_eptr, ::std::mem_fn(&Bucket::debug_clear));
#endif
    this->m_head = reinterpret_cast<Bucket*>(0xDEADBEEF);
  }

details_pointer_hashset::Bucket*
Pointer_HashSet::
do_xprobe(const void* ptr) const noexcept
  {
    auto bptr = this->m_bptr;
    auto eptr = this->m_eptr;

    // Find a bucket using linear probing.
    // We keep the load factor below 1.0 so there will always be some empty buckets
    // in the table.
    auto mptr = ::rocket::get_probing_origin(bptr, eptr,
                    reinterpret_cast<uintptr_t>(ptr));
    auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                    [&](const Bucket& r) { return r.key_ptr == ptr;  });

    // The load factor is kept <= 0.5 so there must always be a bucket available.
    ROCKET_ASSERT(qbkt);
    return qbkt;
  }

void
Pointer_HashSet::
do_xrelocate_but(Bucket* qxcld) noexcept
  {
    ::rocket::linear_probe(
      this->m_bptr,
      qxcld,
      qxcld + 1,
      this->m_eptr,
      [&](Bucket& rb) {
        auto sbkt = &rb;

        // Mark this bucket empty, without destroying its contents.
        ROCKET_ASSERT(*sbkt);
        this->do_list_detach(sbkt);

        // Find a new bucket for the pointer using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need
        // to check for collisions.
        auto mptr = ::rocket::get_probing_origin(this->m_bptr, this->m_eptr,
                        reinterpret_cast<uintptr_t>(sbkt->key_ptr));
        auto qbkt = ::rocket::linear_probe(this->m_bptr, mptr, mptr,this->m_eptr,
                        [&](const Bucket&) { return false;  });

        // Mark the new bucket non-empty.
        ROCKET_ASSERT(qbkt);
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);

        // If the two pointers reference the same one, no relocation is needed.
        if(ROCKET_EXPECT(qbkt == sbkt))
          return false;

        // Relocate the bucket.
        qbkt->key_ptr = sbkt->key_ptr;
        sbkt->key_ptr = reinterpret_cast<void*>(0xBEEFDEAD);

        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void
Pointer_HashSet::
do_rehash_more()
  {
    // Allocate a new table.
    size_t nbkt = (this->m_size * 3 + 5) | 67;
    if(nbkt > PTRDIFF_MAX / sizeof(Bucket))
      throw ::std::bad_array_new_length();

    if(nbkt / 2 <= this->m_size)
      throw ::std::bad_alloc();

    auto bptr = static_cast<Bucket*>(::operator new(nbkt * sizeof(Bucket)));
    auto eptr = bptr + nbkt;

    // Initialize an empty table.
    ::std::for_each(bptr, eptr, [&](Bucket& r) { r.prev = nullptr;  });
    auto bold = ::std::exchange(this->m_bptr, bptr);
    this->m_eptr = eptr;

    // Move buckets into the new table.
    // Warning: No exception shall be thrown from the code below.
    auto sbkt = ::std::exchange(this->m_head, nullptr);
    while(ROCKET_EXPECT(sbkt)) {
      ROCKET_ASSERT(*sbkt);

      // Find a new bucket for the pointer using linear probing.
      // Uniqueness has already been implied for all elements, so there is no need
      // to check for collisions.
      auto mptr = ::rocket::get_probing_origin(bptr, eptr,
                      reinterpret_cast<uintptr_t>(sbkt->key_ptr));
      auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                      [&](const Bucket&) { return false;  });

      // Mark the new bucket non-empty.
      ROCKET_ASSERT(qbkt);
      ROCKET_ASSERT(!*qbkt);
      this->do_list_attach(qbkt);

      // Relocate the bucket.
      qbkt->key_ptr = sbkt->key_ptr;
      sbkt->key_ptr = reinterpret_cast<void*>(0xBEEFDEAD);

      // Process the next bucket.
      sbkt = sbkt->next;
    }
    if(bold)
      ::operator delete(bold);
  }

}  // namespace asteria
