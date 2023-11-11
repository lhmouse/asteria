// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_dictionary.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference_Dictionary::
do_reallocate(uint32_t nbkt)
  {
    if(nbkt >= 0x7FFE0000U / sizeof(Bucket))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(nbkt >= this->m_size * 2);
    auto bptr = (Bucket*) ::calloc(nbkt, sizeof(Bucket));
    if(!bptr)
      throw ::std::bad_alloc();

    if(this->m_bptr) {
      for(uint32_t t = 0;  t != this->m_nbkt;  ++t)
        if(this->m_bptr[t]) {
          // Look for a new bucket for this element. Uniqueness is implied.
          size_t orel = ::rocket::probe_origin(nbkt, this->m_bptr[t].khash);
          auto qrel = ::rocket::linear_probe(bptr, orel, orel, nbkt,
                          [&](const Bucket&) { return false;  });

          // Relocate the value into the new bucket.
          ::memcpy((void*) qrel, (const void*) (this->m_bptr + t), sizeof(Bucket));
          ::std::atomic_signal_fence(::std::memory_order_release);
          this->m_bptr[t].flags = 0;
        }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(Bucket));
#endif
      ::free(this->m_bptr);
    }

    this->m_bptr = bptr;
    this->m_nbkt = nbkt;
  }

void
Reference_Dictionary::
do_deallocate() noexcept
  {
    if(this->m_bptr) {
      this->clear();

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(Bucket));
#endif
      ::free(this->m_bptr);
    }

    this->m_bptr = nullptr;
    this->m_nbkt = 0;
  }

void
Reference_Dictionary::
do_erase_range(uint32_t tpos, uint32_t tn) noexcept
  {
    for(uint32_t t = tpos;  t != tpos + tn;  ++t)
      if(this->m_bptr[t]) {
        this->m_size --;
        this->m_bptr[t].flags = 0;
        ::rocket::destroy(this->m_bptr[t].kstor);
        ::rocket::destroy(this->m_bptr[t].vstor);
      }

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr, tpos, tpos + tn, this->m_nbkt,
      [&](Bucket& r) {
        // Clear this bucket temporarily.
        ROCKET_ASSERT(r.flags != 0);
        uint32_t saved_flags = r.flags;
        r.flags = 0;

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(this->m_nbkt, r.khash);
        auto qrel = ::rocket::linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ::memcpy((void*) qrel, (const void*) &r, sizeof(Bucket));
        ::std::atomic_signal_fence(::std::memory_order_release);
        qrel->flags = saved_flags;
        return false;
      });
  }

Reference&
Reference_Dictionary::
insert(phsh_stringR key, bool* newly_opt)
  {
    if(key.empty())
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "Reference_Dictionary: empty key not valid");

    // Reserve storage for the new element. The load factor is always <= 0.5.
    if(this->m_size >= this->m_nbkt / 2)
      this->do_reallocate(this->m_size * 3 | 5);

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uint32_t) key.rdhash());
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
            [&](const Bucket& r) { return r.key_equals(key);  });

    if(newly_opt)
      *newly_opt = !*qbkt;

    if(*qbkt)
      return qbkt->vstor[0];

    // Construct a new element.
    qbkt->flags = 1;
    qbkt->khash = (uint32_t) key.rdhash();
    ::rocket::construct(qbkt->kstor, key);
    ::rocket::construct(qbkt->vstor);
    this->m_size ++;
    return qbkt->vstor[0];
  }

bool
Reference_Dictionary::
erase(phsh_stringR key, Reference* refp_opt) noexcept
  {
    if(this->m_size == 0)
      return false;

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uint32_t) key.rdhash());
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
          [&](const Bucket& r) { return r.key_equals(key);  });

    if(!*qbkt)
      return false;

    if(refp_opt)
      *refp_opt = ::std::move(qbkt->vstor[0]);

    // Destroy the bucket.
    this->do_erase_range((uint32_t) (qbkt - this->m_bptr), 1);
    return true;
  }

}  // namespace asteria
