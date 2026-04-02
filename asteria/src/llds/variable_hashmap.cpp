// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../llds/variable_hashmap.hpp"
#include "../../rocket/xmemory.hpp"
#include "../../utils.hpp"
namespace asteria {

ASTERIA_FLATTEN
void
Variable_HashMap::
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
        size_t orel = probe_origin(minfo.count, (uintptr_t) eptr->next->key);
        auto qrel = linear_probe((Bucket*) minfo.data, orel, orel, minfo.count,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ASTERIA_ASSERT(qrel);
        qrel->key = eptr->next->key;
        bcopy(qrel->var_opt, eptr->next->var_opt);
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
Variable_HashMap::
do_clear(bool free_storage)
  noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    while(eptr->next != eptr) {
      this->m_size --;
      destroy(&(eptr->next->var_opt));
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

bool
Variable_HashMap::
insert(const void* key, const refcnt_ptr<Variable>& var_opt)
  {
#ifdef ASTERIA_DEBUG
    this->do_reallocate(this->m_size * 2 + 2);
#else
    if(ASTERIA_UNEXPECT(this->m_size >= this->m_nbkt / 2))
      this->do_reallocate(this->m_size * 3 | 5);
#endif

    // Find a bucket using linear probing.
    size_t orig = probe_origin(this->m_nbkt, (uintptr_t) key);
    auto qbkt = linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(*qbkt)
      return false;

    // Construct a new element.
    qbkt->key = key;
    construct(&(qbkt->var_opt), var_opt);
    qbkt->attach(*(this->m_bptr + this->m_nbkt));
    this->m_size ++;
    return true;
  }

bool
Variable_HashMap::
erase(const void* key, refcnt_ptr<Variable>* varp_opt)
  noexcept
  {
    if(this->m_size == 0)
      return false;

    // Find a bucket using linear probing.
    size_t orig = probe_origin(this->m_nbkt, (uintptr_t) key);
    auto qbkt = linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(!*qbkt)
      return false;

    if(varp_opt)
      *varp_opt = move(qbkt->var_opt);

    // Destroy this bucket.
    this->m_size --;
    qbkt->detach();
    destroy(&(qbkt->var_opt));

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
        size_t orel = probe_origin(this->m_nbkt, (uintptr_t) r.key);
        auto qrel = linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ASTERIA_ASSERT(qrel);
        qrel->key = r.key;
        bcopy(qrel->var_opt, r.var_opt);
        qrel->attach(*(this->m_bptr + this->m_nbkt));
        return false;
      });

    return true;
  }

void
Variable_HashMap::
merge_into(Variable_HashMap& other)
  const
  {
    if(this == &other)
      return;

    if(this->m_size == 0)
      return;

    auto eptr = this->m_bptr + this->m_nbkt;
    for(auto qbkt = eptr->next;  qbkt != eptr;  qbkt = qbkt->next)
      if(qbkt->var_opt)
        other.insert(qbkt->key, qbkt->var_opt);
  }

bool
Variable_HashMap::
extract_variable(refcnt_ptr<Variable>& var)
  noexcept
  {
    for(;;) {
      if(this->m_size == 0)
        return false;

      auto eptr = this->m_bptr + this->m_nbkt;
      ASTERIA_ASSERT(eptr->next != eptr);
      auto qbkt = eptr->next;

      var.swap(qbkt->var_opt);

      // Destroy this bucket.
      this->m_size --;
      qbkt->detach();
      destroy(&(qbkt->var_opt));

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
          size_t orel = probe_origin(this->m_nbkt, (uintptr_t) r.key);
          auto qrel = linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                          [&](const Bucket&) { return false;  });

          // Relocate the value into the new bucket.
          ASTERIA_ASSERT(qrel);
          qrel->key = r.key;
          bcopy(qrel->var_opt, r.var_opt);
          qrel->attach(*(this->m_bptr + this->m_nbkt));
          return false;
        });

      if(var)
        return true;
    }
  }

}  // namespace asteria
