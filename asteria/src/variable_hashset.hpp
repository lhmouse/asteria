// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HASHSET_HPP_
#define ASTERIA_VARIABLE_HASHSET_HPP_

#include "fwd.hpp"
#include "variable.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Variable_hashset
  {
  private:
    rocket::refcounted_ptr<Variable> *m_data;
    Size m_nbkt;
    Size m_size;

  public:
    Variable_hashset() noexcept
      : m_data(nullptr), m_nbkt(0), m_size(0)
      {
      }
    ~Variable_hashset();

    Variable_hashset(const Variable_hashset &)
      = delete;
    Variable_hashset & operator=(const Variable_hashset &)
      = delete;

  private:
    void do_rehash(Size res_arg);
    Diff do_find(const rocket::refcounted_ptr<Variable> &var) const noexcept;
    bool do_insert_unchecked(const rocket::refcounted_ptr<Variable> &var) noexcept;
    void do_erase_unchecked(Size tpos) noexcept;

  public:
    bool empty() const noexcept
      {
        return this->m_size == 0;
      }
    Size size() const noexcept
      {
        return this->m_size;
      }
    void clear() noexcept
      {
        const auto data = this->m_data;
        const auto nbkt = this->m_nbkt;
        for(Size i = 0; i != nbkt; ++i) {
          data[i] = nullptr;
        }
        this->m_size = 0;
      }

    template<typename FuncT>
      void for_each(FuncT &&func) const
      {
        const auto data = this->m_data;
        const auto nbkt = this->m_nbkt;
        for(Size i = 0; i != nbkt; ++i) {
          if(data[i]) {
            std::forward<FuncT>(func)(data[i]);
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
    Size max_size() const noexcept
      {
        const auto max_nbkt = Size(-1) / 2 / sizeof(*(this->m_data));
        return max_nbkt / 2;
      }
    Size capacity() const noexcept
      {
        const auto nbkt = this->m_nbkt;
        return nbkt / 2;
      }
    void reserve(Size res_arg)
      {
        if(res_arg <= this->capacity()) {
          return;
        }
        this->do_rehash(res_arg);
      }
    bool insert(const rocket::refcounted_ptr<Variable> &var)
      {
        this->reserve(this->size() + 1);
        return this->do_insert_unchecked(var);
      }
    bool erase(const rocket::refcounted_ptr<Variable> &var) noexcept
      {
        const auto toff = this->do_find(var);
        if(toff < 0) {
          return false;
        }
        this->do_erase_unchecked(static_cast<Size>(toff));
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
