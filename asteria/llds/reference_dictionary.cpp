// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_dictionary.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference_Dictionary::
do_reallocate(uint32_t new_nbkt)
  {
    if(new_nbkt >= 0x7FFE0000U / sizeof(Bucket))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(new_nbkt >= this->m_size * 2);
    auto new_bptr = (Bucket*) ::calloc(new_nbkt + 1, sizeof(Bucket));
    if(!new_bptr)
      throw ::std::bad_alloc();

    auto new_eptr = new_bptr + new_nbkt;
    new_eptr->prev = new_eptr;
    new_eptr->next = new_eptr;

    if(this->m_bptr) {
      auto eptr = this->m_bptr + this->m_nbkt;
      while(eptr->next != eptr) {
        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(new_nbkt, eptr->next->key.rdhash());
        auto qrel = ::rocket::linear_probe(new_bptr, orel, orel, new_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        bcopy(qrel->key, eptr->next->key);
        bcopy(qrel->ref, eptr->next->ref);
        qrel->attach(*new_eptr);
        eptr->next->detach();
      }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(Bucket));
#endif
      ::free(this->m_bptr);
    }

    this->m_bptr = new_bptr;
    this->m_nbkt = new_nbkt;
  }

void
Reference_Dictionary::
do_deallocate() noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    while(eptr->next != eptr) {
      // Destroy this bucket.
      this->m_size --;
      ::rocket::destroy(&(eptr->next->key));
      ::rocket::destroy(&(eptr->next->ref));
      eptr->next->detach();
    }

#ifdef ROCKET_DEBUG
    ::memset((void*) this->m_bptr, 0xE7, this->m_nbkt * sizeof(Bucket));
#endif
    ::free(this->m_bptr);

    this->m_bptr = nullptr;
    this->m_nbkt = 0;
    ROCKET_ASSERT(this->m_size == 0);
  }

void
Reference_Dictionary::
do_erase_range(uint32_t tpos, uint32_t tn) noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    for(auto qbkt = this->m_bptr + tpos;  qbkt != this->m_bptr + tpos + tn;  ++qbkt)
      if(*qbkt) {
        // Destroy this bucket.
        this->m_size --;
        qbkt->detach();
        ::rocket::destroy(&(qbkt->key));
        ::rocket::destroy(&(qbkt->ref));
      }

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr, tpos, tpos + tn, this->m_nbkt,
      [&](Bucket& r) {
        // Clear this bucket temporarily.
        ROCKET_ASSERT(r);
        r.detach();

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(this->m_nbkt, r.key.rdhash());
        auto qrel = ::rocket::linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        bcopy(qrel->key, r.key);
        bcopy(qrel->ref, r.ref);
        qrel->attach(*eptr);
        return false;
      });
  }

Reference&
Reference_Dictionary::
insert(phsh_stringR key, bool* newly_opt)
  {
    if(this->m_size >= this->m_nbkt / 2)
      this->do_reallocate(this->m_size * 3 | 5);

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, key.rdhash());
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(newly_opt)
      *newly_opt = !*qbkt;

    if(*qbkt)
      return qbkt->ref;

    // Construct a new element.
    auto eptr = this->m_bptr + this->m_nbkt;
    ::rocket::construct(&(qbkt->key), key);
    ::rocket::construct(&(qbkt->ref));
    qbkt->attach(*eptr);
    this->m_size ++;
    return qbkt->ref;
  }

bool
Reference_Dictionary::
erase(phsh_stringR key, Reference* refp_opt) noexcept
  {
    if(this->m_size == 0)
      return false;

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, key.rdhash());
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(!*qbkt)
      return false;

    if(refp_opt)
      *refp_opt = ::std::move(qbkt->ref);

    // Destroy this element.
    this->do_erase_range((uint32_t) (qbkt - this->m_bptr), 1);
    return true;
  }

}  // namespace asteria
