// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_

#include "../fwd.hpp"
#include "variable.hpp"

namespace Asteria {

class Variable_hashset
  {
  private:
    struct Bucket
      {
        rocket::refcounted_ptr<Variable> var;

        explicit operator bool () const noexcept
          {
            return this->var != nullptr;
          }
      };

    Bucket *m_data;
    std::size_t m_nbkt;
    std::size_t m_size;

  public:
    Variable_hashset() noexcept
      : m_data(nullptr), m_nbkt(0), m_size(0)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Variable_hashset);

  private:
    void do_clear() noexcept;
    [[noreturn]] void do_throw_insert_null_pointer();
    void do_rehash(std::size_t res_arg);
    std::ptrdiff_t do_find(const rocket::refcounted_ptr<Variable> &var) const noexcept;
    bool do_insert_unchecked(const rocket::refcounted_ptr<Variable> &var) noexcept;
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

    template<typename FuncT>
      void for_each(FuncT &&func) const
      {
        const auto data = this->m_data;
        const auto nbkt = this->m_nbkt;
        for(std::size_t i = 0; i != nbkt; ++i) {
          if(data[i].var) {
            std::forward<FuncT>(func)(data[i].var);
          }
        }
      }
    bool has(const rocket::refcounted_ptr<Variable> &var) const noexcept
      {
        const auto toff = this->do_find(var);
        if(toff < 0) {
          return false;
        }
        return true;
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
    bool insert(const rocket::refcounted_ptr<Variable> &var)
      {
        if(!var) {
          this->do_throw_insert_null_pointer();
        }
        this->reserve(this->size() + 1);
        return this->do_insert_unchecked(var);
      }
    bool erase(const rocket::refcounted_ptr<Variable> &var) noexcept
      {
        const auto toff = this->do_find(var);
        if(toff < 0) {
          return false;
        }
        this->do_erase_unchecked(static_cast<std::size_t>(toff));
        return true;
      }

    void swap(Variable_hashset &other) noexcept
      {
         std::swap(this->m_data, other.m_data);
         std::swap(this->m_nbkt, other.m_nbkt);
         std::swap(this->m_size, other.m_size);
      }
  };

}

#endif
