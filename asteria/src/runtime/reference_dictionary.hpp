// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_

#include "../fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Reference_Dictionary
  {
  private:
    struct Bucket
      {
        // An empty name indicates an empty bucket.
        // `second[0]` is initialized if and only if `name` is non-empty.
        PreHashed_String first;
        union { Reference second[1];  };
        // For the first bucket:  `size` is the number of non-empty buckets in this container.
        // For every other bucket: `prev` points to the previous non-empty bucket.
        union { std::size_t size;  Bucket *prev;  };
        // For the last bucket:   `reserved` is reserved for future use.
        // For every other bucket: `next` points to the next non-empty bucket.
        union { std::size_t reserved;  Bucket *next;  };

        Bucket() noexcept
          {
#ifdef ROCKET_DEBUG
            std::memset(static_cast<void *>(this->second), 0xEC, sizeof(Reference));
#endif
          }
        inline ~Bucket();

        Bucket(const Bucket &)
          = delete;
        Bucket & operator=(const Bucket &)
          = delete;

        explicit inline operator bool () const noexcept;
        inline void do_attach(Bucket *ipos) noexcept;
        inline void do_detach() noexcept;
      };

  private:
    // The first and last buckets are permanently reserved.
    Cow_Vector<Bucket> m_stor;

  public:
    Reference_Dictionary() noexcept
      : m_stor()
      {
      }

    Reference_Dictionary(const Reference_Dictionary &)
      = delete;
    Reference_Dictionary & operator=(const Reference_Dictionary &)
      = delete;

  private:
    void do_clear() noexcept;
    void do_rehash(std::size_t res_arg);
    void do_check_relocation(Bucket *to, Bucket *from);

  public:
    bool empty() const noexcept
      {
        if(this->m_stor.empty()) {
          return true;
        }
        return this->m_stor.front().size == 0;
      }
    std::size_t size() const noexcept
      {
        if(this->m_stor.empty()) {
          return 0;
        }
        return this->m_stor.front().size;
      }
    void clear() noexcept
      {
        if(this->m_stor.empty()) {
          return;
        }
        this->do_clear();
      }

    const Reference * get_opt(const PreHashed_String &name) const noexcept;
    Reference & open(const PreHashed_String &name);
    bool remove(const PreHashed_String &name) noexcept;
  };

}  // namespace Asteria

#endif
