// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable_hashmap.hpp"
#include "../utils.hpp"

namespace asteria {

void
Variable_HashMap::
do_destroy_buckets(bool xfree) noexcept
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = next;
      next = qbkt->next;

      // Destroy this bucket.
      ROCKET_ASSERT(*qbkt);
      ::rocket::destroy(qbkt->vstor);
#ifdef ROCKET_DEBUG
      ::std::memset((void*)qbkt, 0xD2, sizeof(*qbkt));
#endif
      qbkt->prev = nullptr;
    }

    this->m_head = (Bucket*)0xDEADBEEF;
    if(!xfree)
      return;

    // Deallocate the old table.
    auto bold = ::std::exchange(this->m_bptr, (Bucket*)0xDEADBEEF);
    auto eold = ::std::exchange(this->m_eptr, (Bucket*)0xBEEFDEAD);
    ::rocket::freeN<Bucket>(bold, (size_t)(eold - bold));
  }

void
Variable_HashMap::
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

        // Find a new bucket for the name using linear probing.
        // Uniqueness has already been implied for all elements, so there is
        // no need to check for collisions.
        auto mptr = ::rocket::get_probing_origin(this->m_bptr, this->m_eptr,
                        reinterpret_cast<uintptr_t>(sbkt->key_p));
        auto qbkt = ::rocket::linear_probe(this->m_bptr, mptr, mptr, this->m_eptr,
                        [&](const Bucket&) { return false;  });

        // Mark the new bucket non-empty.
        ROCKET_ASSERT(qbkt);
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);

        // If the two pointers reference the same one, no relocation is needed.
        if(ROCKET_EXPECT(qbkt == sbkt))
          return false;

        // Relocate the bucket.
        qbkt->key_p = sbkt->key_p;
        ::rocket::construct(qbkt->vstor, ::std::move(sbkt->vstor[0]));
        ::rocket::destroy(sbkt->vstor);

        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void
Variable_HashMap::
do_rehash_more(size_t nadd)
  {
    // Allocate a new table.
    size_t nbkt = (this->m_size * 3 + nadd * 2) | 97;
    if(nbkt / 2 <= this->m_size)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "Variable_HashMap: rehash size not valid (`%zd` + `%zd`)",
          this->m_size, nadd);

    auto bptr = ::rocket::allocN<Bucket>(nbkt);
    auto eptr = bptr + nbkt;

    // Initialize an empty table.
    ::std::for_each(bptr, eptr, [&](Bucket& r) { r.prev = nullptr;  });
    auto bold = ::std::exchange(this->m_bptr, bptr);
    auto eold = ::std::exchange(this->m_eptr, eptr);
    if(ROCKET_EXPECT(!bold))
      return;

    // Move buckets into the new table.
    // Warning: no exception shall be thrown from the code below.
    auto sbkt = ::std::exchange(this->m_head, nullptr);
    while(ROCKET_EXPECT(sbkt)) {
      ROCKET_ASSERT(*sbkt);

      // Find a new bucket for the name using linear probing.
      // Uniqueness has already been implied for all elements, so there is
      // no need to check for collisions.
      auto mptr = ::rocket::get_probing_origin(bptr, eptr,
                      reinterpret_cast<uintptr_t>(sbkt->key_p));
      auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                      [&](const Bucket&) { return false;  });

      // Mark the new bucket non-empty.
      ROCKET_ASSERT(qbkt);
      ROCKET_ASSERT(!*qbkt);
      this->do_list_attach(qbkt);

      // Relocate the bucket.
      qbkt->key_p = sbkt->key_p;
      ::rocket::construct(qbkt->vstor, ::std::move(sbkt->vstor[0]));
      ::rocket::destroy(sbkt->vstor);

      // Process the next bucket.
      sbkt = sbkt->next;
    }

    // Deallocate the old table.
    ::rocket::freeN<Bucket>(bold, (size_t)(eold - bold));
  }

size_t
Variable_HashMap::
do_merge_into(Variable_HashMap& other) const
  {
    size_t num = 0;
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = next;
      next = qbkt->next;

      // Copy this bucket.
      ROCKET_ASSERT(*qbkt);
      num += other.insert(qbkt->key_p, qbkt->vstor[0]);
    }
    return num;
  }

}  // namespace asteria
