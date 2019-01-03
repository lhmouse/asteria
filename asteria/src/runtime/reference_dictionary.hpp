// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_RUNTIME_REFERENCE_DICTIONARY_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../rocket/static_vector.hpp"

namespace Asteria {

class Reference_dictionary
  {
  private:
    struct Bucket
      {
        rocket::prehashed_string name;
        rocket::static_vector<Reference, 1> refv;

        explicit operator bool () const noexcept
          {
            return this->refv.size() != 0;
          }
      };

    Bucket *m_data;
    std::size_t m_nbkt;
    std::size_t m_size;

  public:
    Reference_dictionary() noexcept
      : m_data(nullptr), m_nbkt(0), m_size(0)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_dictionary);

  private:
    void do_clear() noexcept;
    [[noreturn]] void do_throw_open_empty_name();
    void do_rehash(std::size_t res_arg);
    std::ptrdiff_t do_find(const rocket::prehashed_string &name) const noexcept;
    Reference & do_open_unchecked(const rocket::prehashed_string &name) noexcept;
    void do_erase_unchecked(std::size_t tpos) noexcept;

  public:
    bool empty() const noexcept
      {
        return this->m_size == 0;
      }
    std::size_t size() const noexcept
      {
        return this->m_size;
      }
    void clear() noexcept
      {
        if(this->empty()) {
          return;
        }
        this->do_clear();
      }

    const Reference * get_opt(const rocket::prehashed_string &name) const noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return nullptr;
        }
        const auto ptr = this->m_data[toff].refv.data();
        ROCKET_ASSERT(ptr);
        return ptr;
      }
    Reference * get_opt(const rocket::prehashed_string &name) noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return nullptr;
        }
        const auto ptr = this->m_data[toff].refv.mut_data();
        ROCKET_ASSERT(ptr);
        return ptr;
      }

    Reference & open(const rocket::prehashed_string &name)
      {
        if(name.empty()) {
          this->do_throw_open_empty_name();
        }
        if(this->m_size >= this->m_nbkt / 2) {
          this->do_rehash(this->m_size + 1);
        }
        return this->do_open_unchecked(name);
      }
    bool unset(const rocket::prehashed_string &name) noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return false;
        }
        this->do_erase_unchecked(static_cast<std::size_t>(toff));
        return true;
      }
  };

}

#endif
