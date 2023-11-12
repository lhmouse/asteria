// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHMAP_
#define ASTERIA_LLDS_VARIABLE_HASHMAP_

#include "../fwd.hpp"
#include "../runtime/variable.hpp"
#include "../details/variable_hashmap.ipp"
namespace asteria {

class Variable_HashMap
  {
  private:
    using Bucket = details_variable_hashmap::Bucket;
    Bucket* m_bptr = nullptr;  // beginning of bucket storage
    uint32_t m_size = 0;  // number of initialized buckets
    uint32_t m_nbkt = 0;  // number of allocated buckets
    uint32_t m_random = 0;  // used by `extract_variable()`

  public:
    explicit constexpr
    Variable_HashMap() noexcept
      { }

    Variable_HashMap(Variable_HashMap&& other) noexcept
      {
        this->swap(other);
      }

    Variable_HashMap&
    operator=(Variable_HashMap&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    Variable_HashMap&
    swap(Variable_HashMap& other) noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_size, other.m_size);
        ::std::swap(this->m_nbkt, other.m_nbkt);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t nbkt);

    void
    do_deallocate() noexcept;

    void
    do_erase_range(uint32_t tpos, uint32_t tn) noexcept;

  public:
    ~Variable_HashMap()
      {
        if(this->m_bptr)
          this->do_deallocate();
      }

    bool
    empty() const noexcept
      { return this->m_size == 0;  }

    uint32_t
    size() const noexcept
      { return this->m_size;  }

    void
    clear() noexcept
      {
        this->do_erase_range(0, this->m_nbkt);
      }

    const refcnt_ptr<Variable>*
    find_opt(const void* key) const noexcept
      {
        if(this->m_nbkt == 0)
          return nullptr;

        // Find a bucket using linear probing.
        size_t orig = ::rocket::probe_origin(this->m_nbkt, (uintptr_t) key);
        auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
              [&](const details_variable_hashmap::Bucket& r) { return r.key_opt == key;  });

        // The load factor is kept <= 0.5 so a bucket is always returned. If
        // probing has stopped on an empty bucket, then there is no match.
        if(!*qbkt)
          return nullptr;

        return qbkt->vstor;
      }

    bool
    insert(const void* key, const refcnt_ptr<Variable>& var);

    bool
    erase(const void* key, refcnt_ptr<Variable>* varp_opt = nullptr) noexcept;

    void
    merge_into(Variable_HashMap& other) const;

    bool
    extract_variable(refcnt_ptr<Variable>& var) noexcept;
  };

inline
void
swap(Variable_HashMap& lhs, Variable_HashMap& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
