// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

void
Variable_HashMap::
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
          size_t orel = ::rocket::probe_origin(nbkt, (uintptr_t) this->m_bptr[t].key_opt);
          auto qrel = ::rocket::linear_probe(bptr, orel, orel, nbkt,
                          [&](const Bucket&) { return false;  });

          // Relocate the value into the new bucket.
          ::memcpy((void*) qrel, (const void*) (this->m_bptr + t), sizeof(Bucket));
          ::std::atomic_signal_fence(::std::memory_order_release);
          this->m_bptr[t].key_opt = nullptr;
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
Variable_HashMap::
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
Variable_HashMap::
do_erase_range(uint32_t tpos, uint32_t tn) noexcept
  {
    for(uint32_t t = tpos;  t != tpos + tn;  ++t)
      if(this->m_bptr[t]) {
        this->m_size --;
        this->m_bptr[t].key_opt = nullptr;
        ::rocket::destroy(this->m_bptr[t].vstor);
      }

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr, tpos, tpos + tn, this->m_nbkt,
      [&](Bucket& r) {
        // Clear this bucket temporarily.
        ROCKET_ASSERT(r.key_opt);
        const void* saved_key = r.key_opt;
        r.key_opt = nullptr;

        // Look for a new bucket for this element. Uniqueness is implied.
        size_t orel = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) saved_key);
        auto qrel = ::rocket::linear_probe(this->m_bptr, orel, orel, this->m_nbkt,
                        [&](const Bucket&) { return false;  });

        // Relocate the value into the new bucket.
        ::memcpy((void*) qrel, (const void*) &r, sizeof(Bucket));
        ::std::atomic_signal_fence(::std::memory_order_release);
        qrel->key_opt = saved_key;
        return false;
      });
  }

bool
Variable_HashMap::
insert(const void* key, const refcnt_ptr<Variable>& var)
  {
    if(!key)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "Variable_HashMap: null key not valid");

    // Reserve storage for the new element. The load factor is always <= 0.5.
    if(this->m_size >= this->m_nbkt / 2)
      this->do_reallocate(this->m_size * 3 | 57);

    // Find a bucket using linear probing.
    size_t orig = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) key);
    auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
            [&](const Bucket& r) { return r.key_opt == key;  });

    if(*qbkt)
      return false;

    // Construct a new element.
    qbkt->key_opt = key;
    ::rocket::construct(qbkt->vstor, var);
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
          [&](const Bucket& r) { return r.key_opt == key;  });

    if(!*qbkt)
      return false;

    // Destroy the bucket.
    if(varp_opt)
      *varp_opt = ::std::move(qbkt->vstor[0]);

    this->do_erase_range((uint32_t) (qbkt - this->m_bptr), 1);
    return true;
  }

void
Variable_HashMap::
merge(const Variable_HashMap& other)
  {
    if(this == &other)
      return;

    if(other.m_size != 0)
      for(uint32_t t = 0;  t != other.m_nbkt;  ++t)
        if(other.m_bptr[t])
          this->insert(other.m_bptr[t].key_opt, other.m_bptr[t].vstor[0]);
  }

refcnt_ptr<Variable>
Variable_HashMap::
extract_variable_opt() noexcept
  {
    if(ROCKET_UNEXPECT(this->m_random > this->m_nbkt))
      this->m_random = this->m_nbkt;

    // Like linear probing, use `m_random` as a hint. It will be updated after
    // a variable has been popped.
    for(uint32_t t = this->m_random;  t != this->m_nbkt;  ++t)
      if(ROCKET_UNEXPECT(this->m_bptr[t] && this->m_bptr[t].vstor[0])) {
        this->m_random = t;
        return ::std::move(this->m_bptr[t].vstor[0]);
      }

    for(uint32_t t = 0;  t != this->m_random;  ++t)
      if(ROCKET_UNEXPECT(this->m_bptr[t] && this->m_bptr[t].vstor[0])) {
        this->m_random = t;
        return ::std::move(this->m_bptr[t].vstor[0]);
      }

    return nullptr;
  }

}  // namespace asteria
