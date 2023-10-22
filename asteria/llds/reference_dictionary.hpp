// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_
#define ASTERIA_LLDS_REFERENCE_DICTIONARY_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
#include "../details/reference_dictionary.ipp"
namespace asteria {

class Reference_Dictionary
  {
  private:
    details_reference_dictionary::Bucket* m_bptr = nullptr;  // beginning of bucket storage
    uint32_t m_nbkt = 0;  // number of allocated buckets
    uint32_t m_size = 0;  // number of initialized buckets

  public:
    explicit constexpr
    Reference_Dictionary() noexcept
      { }

    Reference_Dictionary(Reference_Dictionary&& other) noexcept
      {
        this->swap(other);
      }

    Reference_Dictionary&
    operator=(Reference_Dictionary&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    Reference_Dictionary&
    swap(Reference_Dictionary& other) noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_nbkt, other.m_nbkt);
        ::std::swap(this->m_size, other.m_size);
        return *this;
      }

  private:
    // This is the only memory management function. `nbkt` shall specify the
    // new number of buckets of the hash table. If `nbkt` is zero, any dynamic
    // storage will be deallocated.
    void
    do_rehash(uint32_t nbkt);

  public:
    ~Reference_Dictionary()
      {
        if(this->m_bptr)
          this->do_rehash(0);
      }

    bool
    empty() const noexcept
      { return this->m_size == 0;  }

    size_t
    size() const noexcept
      { return this->m_size;  }

    void
    clear() noexcept;

    const Reference*
    find_opt(phsh_stringR key) const noexcept
      {
        if(this->m_nbkt == 0)
          return nullptr;

        // Find a bucket using linear probing.
        size_t orig = ::rocket::probe_origin(this->m_nbkt, key.rdhash());
        auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
              [&](const details_reference_dictionary::Bucket& r) { return r.key_equals(key);  });

        // The load factor is kept <= 0.5 so a bucket is always returned. If
        // probing has stopped on an empty bucket, then there is no match.
        if(!*qbkt)
          return nullptr;

        return qbkt->vstor;
      }

    Reference*
    mut_find_opt(phsh_stringR key) noexcept
      {
        if(this->m_nbkt == 0)
          return nullptr;

        // Find a bucket using linear probing.
        size_t orig = ::rocket::probe_origin(this->m_nbkt, key.rdhash());
        auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
              [&](const details_reference_dictionary::Bucket& r) { return r.key_equals(key);  });

        // The load factor is kept <= 0.5 so a bucket is always returned. If
        // probing has stopped on an empty bucket, then there is no match.
        if(!*qbkt)
          return nullptr;

        return qbkt->vstor;
      }

    Reference&
    insert(phsh_stringR key, bool* newly = nullptr);

    bool
    erase(phsh_stringR key, Reference* refp_opt = nullptr) noexcept;
  };

inline
void
swap(Reference_Dictionary& lhs, Reference_Dictionary& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
