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
        union { std::size_t size; Bucket *prev;  };
        // For the last bucket:   `reserved` is reserved for future use.
        // For every other bucket: `next` points to the next non-empty bucket.
        union { std::size_t reserved; Bucket *next;  };

        Bucket() noexcept
          : first()
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
    const std::pair<CoW_String, Reference> *m_templ_data;
    std::size_t m_templ_size;
    // The first and last buckets are permanently reserved.
    CoW_Vector<Bucket> m_stor;

  public:
    Reference_Dictionary() noexcept
      : m_templ_data(nullptr), m_templ_size(0),
        m_stor()
      {
      }

    Reference_Dictionary(const Reference_Dictionary &)
      = delete;
    Reference_Dictionary & operator=(const Reference_Dictionary &)
      = delete;

  private:
    const Reference * do_get_template_noninline_opt(const PreHashed_String &name) const noexcept;
    const Reference * do_get_dynamic_noninline_opt(const PreHashed_String &name) const noexcept;

    const Reference * do_get_template_opt(const PreHashed_String &name) const noexcept
      {
        if(ROCKET_EXPECT(this->m_templ_size == 0)) {
          return nullptr;
        }
        return this->do_get_template_noninline_opt(name);
      }
    const Reference * do_get_dynamic_opt(const PreHashed_String &name) const noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.empty())) {
          return nullptr;
        }
        return this->do_get_dynamic_noninline_opt(name);
      }

    void do_clear() noexcept;
    void do_rehash(std::size_t res_arg);
    void do_check_relocation(Bucket *to, Bucket *from);

  public:
    void set_templates(const std::pair<CoW_String, Reference> *tdata_opt, std::size_t tsize) noexcept
      {
        // Elements in [begin, end) must have been sorted.
        this->m_templ_data = tdata_opt;
        this->m_templ_size = tsize;
      }

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

    const Reference * get_opt(const PreHashed_String &name) const noexcept
      {
        auto qref = this->do_get_dynamic_opt(name);
        if(ROCKET_EXPECT(qref)) {
          return qref;
        }
        qref = this->do_get_template_opt(name);
        if(ROCKET_EXPECT(qref)) {
          return qref;
        }
        return nullptr;
      }
    Reference & open(const PreHashed_String &name);
    bool remove(const PreHashed_String &name) noexcept;
  };

}

#endif
