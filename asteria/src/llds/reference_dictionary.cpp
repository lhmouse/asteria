// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../llds/reference_dictionary.hpp"
#include "../../rocket/xmemory.hpp"
#include "../../utils.hpp"
namespace asteria {

ASTERIA_FLATTEN
void
Reference_Dictionary::
do_reallocate(uint32_t nbkt)
  {
    if(nbkt >= 0x7FFE0000U / sizeof(Bucket))
      throw ::std::bad_alloc();

    ASTERIA_ASSERT(nbkt >= this->m_size * 2);
    xmeminfo minfo;
    minfo.element_size = sizeof(Bucket);
    minfo.count = nbkt + 1;
    xmemalloc(minfo);

    // Initialize the circular list head. This node may be read, but it is never
    // written to, and needs no cleanup.
    minfo.count --;
    auto new_eptr = (Bucket*) minfo.data + minfo.count;
    new_eptr->prev = new_eptr;
    new_eptr->next = new_eptr;

    // Initialize the other nodes.
    for(auto q = (Bucket*) minfo.data;  q != new_eptr;  ++q)
      q->next = nullptr;

    if(this->m_bptr) {
      // Move-construct old elements into the new table.
      auto eptr = this->m_bptr + this->m_nbkt;
      while(eptr->next != eptr) {
        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = probe_origin(minfo.count, eptr->next->key.rdhash());
        auto qrel = linear_probe((Bucket*) minfo.data, orel, orel, minfo.count,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ASTERIA_ASSERT(qrel);
        bcopy(qrel->key, eptr->next->key);
        bcopy(qrel->ref, eptr->next->ref);
        qrel->attach(*new_eptr);
        eptr->next->detach();
      }

      xmeminfo rinfo;
      rinfo.element_size = sizeof(Bucket);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_nbkt + 1;
      xmemfree(rinfo);
    }

    this->m_bptr = (Bucket*) minfo.data;
    this->m_nbkt = (uint32_t) minfo.count;
  }

ASTERIA_FLATTEN
void
Reference_Dictionary::
do_clear(bool free_storage)
  noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    while(eptr->next != eptr) {
      this->m_size --;
      destroy(&(eptr->next->key));
      destroy(&(eptr->next->ref));
      eptr->next->detach();
    }

    if(free_storage) {
      xmeminfo rinfo;
      rinfo.element_size = sizeof(Bucket);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_nbkt;
      xmemfree(rinfo);

      this->m_bptr = nullptr;
      this->m_nbkt = 0;
    }

    ASTERIA_ASSERT(this->m_size == 0);
  }

Reference*
Reference_Dictionary::
do_xfind_opt(const phcow_string& key)
  const noexcept
  {
    if(this->m_nbkt == 0)
      return nullptr;

    // Find a bucket using linear probing. The load factor is kept <= 0.5
    // so a bucket is always returned. If probing has stopped on an empty
    // bucket, then there is no match.
    size_t orig = probe_origin(this->m_nbkt, key.rdhash());
    auto qbkt = linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
           [&](const Bucket& r) __attribute__((__flatten__)) {
             return (r.key.size() == key.size())
                    && (ASTERIA_EXPECT(r.key.data() == key.data())
                        || (ASTERIA_EXPECT(r.key.rdhash() == key.rdhash())
                            && ::std::equal(r.key.rbegin(), r.key.rend(), key.rbegin())));
           });

    if(!*qbkt)
      return nullptr;

    return &(qbkt->ref);
  }

Reference&
Reference_Dictionary::
insert(const phcow_string& key, bool* newly_opt)
  {
#ifdef ASTERIA_DEBUG
    this->do_reallocate(this->m_size * 2 + 2);
#else
    if(ASTERIA_UNEXPECT(this->m_size >= this->m_nbkt / 2))
      this->do_reallocate(this->m_size * 3 | 11);
#endif

    // Find a bucket using linear probing.
    size_t orig = probe_origin(this->m_nbkt, key.rdhash());
    auto qbkt = linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(newly_opt)
      *newly_opt = !*qbkt;

    if(*qbkt)
      return qbkt->ref;

    // Construct a new element.
    construct(&(qbkt->key), key);
    construct(&(qbkt->ref));
    qbkt->attach(*(this->m_bptr + this->m_nbkt));
    this->m_size ++;
    return qbkt->ref;
  }

bool
Reference_Dictionary::
erase(const phcow_string& key, Reference* refp_opt)
  noexcept
  {
    if(this->m_size == 0)
      return false;

    // Find a bucket using linear probing.
    size_t orig = probe_origin(this->m_nbkt, key.rdhash());
    auto qbkt = linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(!*qbkt)
      return false;

    if(refp_opt)
      refp_opt->swap(qbkt->ref);

    // Destroy this bucket.
    this->m_size --;
    qbkt->detach();
    destroy(&(qbkt->key));
    destroy(&(qbkt->ref));

    // Relocate elements that are not placed in their immediate locations.
    linear_probe(
      this->m_bptr,
      (size_t) (qbkt - this->m_bptr),
      (size_t) (qbkt - this->m_bptr + 1),
      this->m_nbkt,
      [&](Bucket& r) {
        // Clear this bucket temporarily.
        ASTERIA_ASSERT(r);
        r.detach();

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = probe_origin(this->m_nbkt, r.key.rdhash());
        auto qrel = linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ASTERIA_ASSERT(qrel);
        bcopy(qrel->key, r.key);
        bcopy(qrel->ref, r.ref);
        qrel->attach(*(this->m_bptr + this->m_nbkt));
        return false;
      });

    return true;
  }

}  // namespace asteria
