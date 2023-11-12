// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

void
Variable_HashMap::
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
        size_t orel = ::rocket::probe_origin(new_nbkt, (uintptr_t) eptr->next->key);
        auto qrel = ::rocket::linear_probe(new_bptr, orel, orel, new_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        qrel->key = eptr->next->key;
        bcopy(qrel->var_opt, eptr->next->var_opt);
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
Variable_HashMap::
do_deallocate() noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    while(eptr->next != eptr) {
      // Destroy this bucket.
      this->m_size --;
      ::rocket::destroy(&(eptr->next->var_opt));
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
Variable_HashMap::
do_erase_range(uint32_t tpos, uint32_t tn) noexcept
  {
    auto eptr = this->m_bptr + this->m_nbkt;
    for(auto qbkt = this->m_bptr + tpos;  qbkt != this->m_bptr + tpos + tn;  ++qbkt)
      if(*qbkt) {
        // Destroy this bucket.
        this->m_size --;
        qbkt->detach();
        ::rocket::destroy(&(qbkt->var_opt));
      }

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr, tpos, tpos + tn, this->m_nbkt,
      [&](Bucket& r) {
        // Clear this bucket temporarily.
        ROCKET_ASSERT(r);
        r.detach();

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) r.key);
        auto qrel = ::rocket::linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        qrel->key = r.key;
        bcopy(qrel->var_opt, r.var_opt);
        qrel->attach(*eptr);
        return false;
      });
  }

bool
Variable_HashMap::
insert(const void* key, const refcnt_ptr<Variable>& var_opt)
  {
    if(this->m_size >= this->m_nbkt / 2)
      this->do_reallocate(this->m_size * 3 | 5);

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) key);
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(*qbkt)
      return false;

    // Construct a new element.
    auto eptr = this->m_bptr + this->m_nbkt;
    qbkt->key = key;
    ::rocket::construct(&(qbkt->var_opt), var_opt);
    qbkt->attach(*eptr);
    this->m_size ++;
    return true;
  }

bool
Variable_HashMap::
erase(const void* key, refcnt_ptr<Variable>* varp_opt) noexcept
  {
    if(this->m_size == 0)
      return false;

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) key);
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

    if(!*qbkt)
      return false;

    if(varp_opt)
      *varp_opt = ::std::move(qbkt->var_opt);

    // Destroy this element.
    this->do_erase_range((uint32_t) (qbkt - this->m_bptr), 1);
    return true;
  }

void
Variable_HashMap::
merge_into(Variable_HashMap& other) const
  {
    if((this == &other) || (this->m_size == 0))
      return;

    auto eptr = this->m_bptr + this->m_nbkt;
    for(auto qbkt = eptr->next;  qbkt != eptr;  qbkt = qbkt->next)
      if(qbkt->var_opt)
        other.insert(qbkt->key, qbkt->var_opt);
  }

bool
Variable_HashMap::
extract_variable(refcnt_ptr<Variable>& var) noexcept
  {
    if(this->m_size == 0)
      return false;

    auto eptr = this->m_bptr + this->m_nbkt;
    while((eptr->next != eptr) && !eptr->next->var_opt)
      this->do_erase_range((uint32_t) (eptr->next - this->m_bptr), 1);

    if(eptr->next == eptr)
      return false;

    var.swap(eptr->next->var_opt);
    this->do_erase_range((uint32_t) (eptr->next - this->m_bptr), 1);
    return true;
  }

}  // namespace asteria
