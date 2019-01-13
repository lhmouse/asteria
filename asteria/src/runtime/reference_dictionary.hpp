// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Reference_dictionary
  {
  public:
    struct Template
      {
        rocket::cow_string name;
        Reference ref;

        template<typename XnameT, typename XrefT,
                 ROCKET_ENABLE_IF(std::is_constructible<rocket::cow_string, XnameT &&>::value && std::is_constructible<Reference, XrefT &&>::value)>
          Template(XnameT &&xname, XrefT &&xref)
          : name(std::forward<XnameT>(xname)), ref(std::forward<XrefT>(xref))
          {
          }
      };

  private:
    struct Bucket
      {
        // An empty name indicates an empty bucket.
        // `refv[0]` is initialized if and only if `name` is non-empty.
        rocket::prehashed_string name;
        union { Reference refv[1]; };
        // For the first bucket:  `size` is the number of non-empty buckets in this container.
        // For each other bucket: `prev` points to the previous non-empty bucket.
        union { std::size_t size; Bucket *prev; };
        // For the last bucket:   `reserved` is reserved for future use.
        // For each other bucket: `next` points to the next non-empty bucket.
        union { std::size_t reserved; Bucket *next; };

        Bucket() noexcept
          : name()
          {
#ifdef ROCKET_DEBUG
            std::memset(static_cast<void *>(this->refv), 0xEC, sizeof(Reference));
#endif
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Bucket)
          {
            // Be careful, VERY careful.
            if(ROCKET_UNEXPECT(*this)) {
              rocket::destroy_at(this->refv);
            }
          }

        explicit operator bool () const noexcept
          {
            return !this->name.empty();
          }
      };

  private:
    const Template *m_templ_data;
    std::size_t m_templ_size;
    // The first and last buckets are permanently reserved.
    rocket::cow_vector<Bucket> m_stor;

  public:
    Reference_dictionary() noexcept
      : m_templ_data(nullptr), m_templ_size(0),
        m_stor()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_dictionary);

  private:
    const Reference * do_get_template_opt(const rocket::prehashed_string &name) const noexcept;
    const Reference * do_get_dynamic_opt(const rocket::prehashed_string &name) const noexcept;

    void do_clear() noexcept;
    void do_rehash(std::size_t res_arg);
    void do_check_relocation(Bucket *to, Bucket *from);

  public:
    void set_templates(const Template *tdata_opt, std::size_t tsize) noexcept
      {
        // Elements in [begin, end) must have been sorted.
#ifdef ROCKET_DEBUG
        if(tsize != 0) {
          ROCKET_ASSERT(tdata_opt);
          for(auto ptr = tdata_opt + 1; ptr != tdata_opt + tsize; ++ptr) {
            ROCKET_ASSERT(ptr[-1].name < ptr[0].name);
          }
        }
#endif
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

    const Reference * get_opt(const rocket::prehashed_string &name) const noexcept
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

    Reference & open(const rocket::prehashed_string &name);
    bool unset(const rocket::prehashed_string &name) noexcept;
  };

}

#endif
