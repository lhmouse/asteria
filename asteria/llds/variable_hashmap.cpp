// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

void
Variable_HashMap::
do_rehash(uint32_t nbkt)
  {
    if(nbkt != 0) {
      // Extend the storage.
      ROCKET_ASSERT(nbkt >= this->m_size * 2);

      auto bptr = (details_variable_hashmap::Bucket*) ::calloc(nbkt, sizeof(details_variable_hashmap::Bucket));
      if(!bptr)
        throw ::std::bad_alloc();

      for(uint32_t t = 0;  t != this->m_nbkt;  ++t)
        if(this->m_bptr[t]) {
          // Look for a new bucket for this element. Uniqueness is implied.
          auto qrel = ::rocket::get_probing_origin(
              bptr, bptr + nbkt, (uintptr_t) this->m_bptr[t].key_opt);

          qrel = ::rocket::linear_probe(
              bptr, qrel, qrel, bptr + nbkt,
              [&](const details_variable_hashmap::Bucket&) { return false;  });

          // Relocate the value into the new bucket.
          qrel->key_opt = this->m_bptr[t].key_opt;
          ::rocket::construct(qrel->vstor, ::std::move(this->m_bptr[t].vstor[0]));
          ::rocket::destroy(this->m_bptr[t].vstor);
        }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(details_variable_hashmap::Bucket));
#endif

      ::free(this->m_bptr);

      this->m_bptr = bptr;
      this->m_nbkt = nbkt;
    }
    else {
      // Free the storage.
      for(uint32_t t = 0;  t != this->m_nbkt;  ++t)
        if(this->m_bptr[t]) {
          this->m_size --;
          this->m_bptr[t].key_opt = nullptr;
          ::rocket::destroy(this->m_bptr[t].vstor);
        }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_nbkt * sizeof(details_variable_hashmap::Bucket));
#endif

      ::free(this->m_bptr);

      this->m_bptr = nullptr;
      this->m_nbkt = 0;
      ROCKET_ASSERT(this->m_size == 0);
    }
  }

void
Variable_HashMap::
clear() noexcept
  {
    for(auto qbkt = this->m_bptr;  qbkt != this->m_bptr + this->m_nbkt;  ++ qbkt)
      if(*qbkt) {
        this->m_size --;
        qbkt->key_opt = nullptr;
        ::rocket::destroy(qbkt->vstor);
      }

    ROCKET_ASSERT(this->m_size == 0);
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
      this->do_rehash(this->m_size * 3 | 57);

    // Find a bucket using linear probing.
    auto qbkt = ::rocket::get_probing_origin(
        this->m_bptr, this->m_bptr + this->m_nbkt, (uintptr_t) key);

    qbkt = ::rocket::linear_probe(
        this->m_bptr, qbkt, qbkt, this->m_bptr + this->m_nbkt,
        [&](const details_variable_hashmap::Bucket& r) { return r.key_opt == key;  });

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
    auto qbkt = ::rocket::get_probing_origin(
        this->m_bptr, this->m_bptr + this->m_nbkt, (uintptr_t) key);

    qbkt = ::rocket::linear_probe(
        this->m_bptr, qbkt, qbkt, this->m_bptr + this->m_nbkt,
        [&](const details_variable_hashmap::Bucket& r) { return r.key_opt == key;  });

    if(!*qbkt)
      return false;

    // Destroy the bucket.
    if(varp_opt)
      *varp_opt = ::std::move(qbkt->vstor[0]);

    this->m_size --;
    qbkt->key_opt = nullptr;
    ::rocket::destroy(qbkt->vstor);

    // Relocate elements that are not placed in their immediate locations.
    ::rocket::linear_probe(
      this->m_bptr, qbkt, qbkt + 1, this->m_bptr + this->m_nbkt,
      [&](details_variable_hashmap::Bucket& t) {
        // Clear this bucket temporarily.
        const void* saved_key = t.key_opt;
        ROCKET_ASSERT(t.key_opt);
        t.key_opt = nullptr;

        // Look for a new bucket for this element. Uniqueness is implied.
        auto qrel = ::rocket::get_probing_origin(
            this->m_bptr, this->m_bptr + this->m_nbkt, (uintptr_t) saved_key);

        qrel = ::rocket::linear_probe(
            this->m_bptr, qrel, qrel, this->m_bptr + this->m_nbkt,
            [&](const details_variable_hashmap::Bucket&) { return false;  });

        if(qrel != &t) {
          // Relocate the value into the new bucket.
          qrel->key_opt = saved_key;
          ::rocket::construct(qrel->vstor, ::std::move(t.vstor[0]));
          ::rocket::destroy(t.vstor);
        }
        else
          t.key_opt = saved_key;

        return false;
      });

    return true;
  }

void
Variable_HashMap::
merge(const Variable_HashMap& other)
  {
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
