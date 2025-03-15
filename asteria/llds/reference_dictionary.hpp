// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_
#define ASTERIA_LLDS_REFERENCE_DICTIONARY_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
#include "../details/reference_dictionary.ipp"
namespace asteria {

class Reference_Dictionary
  {
  private:
    using Bucket = details_reference_dictionary::Bucket;
    Bucket* m_bptr = nullptr;
    uint32_t m_nbkt = 0;
    uint32_t m_size = 0;

  public:
    constexpr Reference_Dictionary() noexcept = default;

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
    void
    do_reallocate(uint32_t nbkt);

    void
    do_deallocate() noexcept;

    void
    do_erase_range(uint32_t tpos, uint32_t tn) noexcept;

    Reference*
    do_xfind_opt(phsh_stringR key) const noexcept
      {
        if(this->m_nbkt == 0)
          return nullptr;

        // Find a bucket using linear probing.
        size_t orig = ::rocket::probe_origin(this->m_nbkt, key.rdhash());
        auto qbkt = ::rocket::linear_probe(this->m_bptr, orig, orig, this->m_nbkt,
                    [&](const Bucket& r) { return r.key == key;  });

        // The load factor is kept <= 0.5 so a bucket is always returned. If
        // probing has stopped on an empty bucket, then there is no match.
        return *qbkt ? &(qbkt->ref) : nullptr;
      }

  public:
    ~Reference_Dictionary()
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

    const Reference*
    find_opt(phsh_stringR key) const noexcept
      {
        return this->do_xfind_opt(key);
      }

    Reference*
    mut_find_opt(phsh_stringR key) noexcept
      {
        return this->do_xfind_opt(key);
      }

    Reference&
    insert(phsh_stringR key, bool* newly_opt);

    bool
    erase(phsh_stringR key, Reference* refp_opt) noexcept;
  };

inline
void
swap(Reference_Dictionary& lhs, Reference_Dictionary& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
