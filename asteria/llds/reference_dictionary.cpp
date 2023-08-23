// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_dictionary.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference_Dictionary::
do_rehash(uint32_t nbkt)
  {
    if(nbkt != 0) {
      // Extend the storage.
      ROCKET_ASSERT(nbkt >= this->m_size * 2);

      auto bptr = (details_reference_dictionary::Bucket*) ::calloc(nbkt, sizeof(details_reference_dictionary::Bucket));
      if(!bptr)
        throw ::std::bad_alloc();

      if(this->m_size != 0)
        for(uint32_t t = 0;  t != this->m_nbkt;  ++t)
          if(this->m_bptr[t]) {
            // Look for a new bucket for this element. Uniqueness is implied.
            size_t orel = ::rocket::probe_origin(nbkt, this->m_bptr[t].khash);
            auto qrel = ::rocket::linear_probe(bptr, orel, orel, nbkt,
                  [&](const details_reference_dictionary::Bucket&) { return false;  });

            // Relocate the value into the new bucket.
            ::memcpy((void*) qrel, (const void*) (this->m_bptr + t), sizeof(details_reference_dictionary::Bucket));
          }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(details_reference_dictionary::Bucket));
#endif
      ::free(this->m_bptr);

      this->m_bptr = bptr;
      this->m_nbkt = nbkt;
    }
    else {
      // Free the storage.
      this->clear();

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(details_reference_dictionary::Bucket));
#endif
      ::free(this->m_bptr);

      this->m_bptr = nullptr;
      this->m_nbkt = 0;
    }
  }

void
Reference_Dictionary::
clear() noexcept
  {
    if(this->m_size != 0)
      for(uint32_t t = 0;  t != this->m_nbkt;  ++t)
        if(this->m_bptr[t]) {
          this->m_size --;
          this->m_bptr[t].flags = 0;
          ::rocket::destroy(this->m_bptr[t].kstor);
          ::rocket::destroy(this->m_bptr[t].vstor);
        }

    ROCKET_ASSERT(this->m_size == 0);
  }

pair<Reference*, bool>
Reference_Dictionary::
insert(phsh_stringR key)
  {
    if(key.empty())
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "Reference_Dictionary: empty key not valid");

    // Reserve storage for the new element. The load factor is always <= 0.5.
    if(this->m_size >= this->m_nbkt / 2)
      this->do_rehash(this->m_size * 3 | 19);

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uint32_t) key.rdhash());
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
          [&](const details_reference_dictionary::Bucket& r) { return r.key_equals(key);  });

    if(*qbkt)
      return { qbkt->vstor, false };

    // Construct a new element.
    qbkt->flags = 1;
    qbkt->khash = (uint32_t) key.rdhash();
    ::rocket::construct(qbkt->kstor, key);
    ::rocket::construct(qbkt->vstor);
    this->m_size ++;
    return { qbkt->vstor, true };
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
          [&](const details_reference_dictionary::Bucket& r) { return r.key_equals(key);  });

    if(*qbkt)
      return false;

    // Destroy the bucket.
    if(refp_opt)
      *refp_opt = ::std::move(qbkt->vstor[0]);

    this->m_size --;
    qbkt->flags = 0;
    ::rocket::destroy(qbkt->kstor);
    ::rocket::destroy(qbkt->vstor);

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr,
      (size_t) (qbkt - this->m_bptr),
      (size_t) (qbkt - this->m_bptr) + 1,
      this->m_nbkt,
      [&](details_reference_dictionary::Bucket& r) {
        // Clear this bucket temporarily.
        ROCKET_ASSERT(r.flags != 0);
        uint32_t saved_flags = r.flags;
        r.flags = 0;

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(this->m_nbkt, r.khash);
        auto qrel = ::rocket::linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
              [&](const details_reference_dictionary::Bucket&) { return false;  });

        qrel->flags = saved_flags;
        if(qrel != &r) {
          // Relocate the key and value into the new bucket.
          qrel->khash = r.khash;
          ::rocket::construct(qrel->kstor, ::std::move(r.kstor[0]));
          ::rocket::destroy(r.kstor);
          ::rocket::construct(qrel->vstor, ::std::move(r.vstor[0]));
          ::rocket::destroy(r.vstor);
        }
        return false;
      });

    return true;
  }

}  // namespace asteria
