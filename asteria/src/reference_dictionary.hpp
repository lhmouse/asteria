// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_REFERENCE_DICTIONARY_HPP_

#include "fwd.hpp"
#include "reference.hpp"
#include "rocket/static_vector.hpp"

namespace Asteria {

class Reference_dictionary
  {
  private:
    struct Bucket
      {
        rocket::cow_string name;
        rocket::static_vector<Reference, 1> refv;
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
    void do_rehash(std::size_t res_arg);
    std::ptrdiff_t do_find(const rocket::cow_string &name) const noexcept;
    bool do_insert_or_assign_unchecked(const rocket::cow_string &name, Reference &&ref) noexcept;
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
        const auto data = this->m_data;
        const auto nbkt = this->m_nbkt;
        for(std::size_t i = 0; i != nbkt; ++i) {
          data[i].refv.clear();
        }
        this->m_size = 0;
      }

    template<typename FuncT>
      void for_each(FuncT &&func) const
      {
        const auto data = this->m_data;
        const auto nbkt = this->m_nbkt;
        for(std::size_t i = 0; i != nbkt; ++i) {
          if(!data[i].refv.empty()) {
            std::forward<FuncT>(func)(data[i].name, data[i].refv.front());
          }
        }
      }
    const Reference * get_opt(const rocket::cow_string &name) const noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return nullptr;
        }
        return this->m_data[toff].refv.data();
      }
    Reference * get_opt(const rocket::cow_string &name) noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return nullptr;
        }
        return this->m_data[toff].refv.data();
      }
    std::size_t max_size() const noexcept
      {
        const auto max_nbkt = std::size_t(-1) / 2 / sizeof(*(this->m_data));
        return max_nbkt / 2;
      }
    std::size_t capacity() const noexcept
      {
        const auto nbkt = this->m_nbkt;
        return nbkt / 2;
      }

    void reserve(std::size_t res_arg)
      {
        if(res_arg <= this->capacity()) {
          return;
        }
        this->do_rehash(res_arg);
      }
    bool set(const rocket::cow_string &name, const Reference &ref)
      {
        ROCKET_ASSERT(!name.empty());
        this->reserve(this->size() + 1);
        return this->do_insert_or_assign_unchecked(name, Reference(ref));
      }
    bool set(const rocket::cow_string &name, Reference &&ref)
      {
        ROCKET_ASSERT(!name.empty());
        this->reserve(this->size() + 1);
        return this->do_insert_or_assign_unchecked(name, std::move(ref));
      }
    bool unset(const rocket::cow_string &name) noexcept
      {
        const auto toff = this->do_find(name);
        if(toff < 0) {
          return false;
        }
        this->do_erase_unchecked(static_cast<std::size_t>(toff));
        return true;
      }

    void swap(Reference_dictionary &other) noexcept
      {
         std::swap(this->m_data, other.m_data);
         std::swap(this->m_nbkt, other.m_nbkt);
         std::swap(this->m_size, other.m_size);
      }
  };

}

#endif
