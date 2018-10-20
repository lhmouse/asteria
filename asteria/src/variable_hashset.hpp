// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HASHSET_HPP_
#define ASTERIA_VARIABLE_HASHSET_HPP_

#include "fwd.hpp"
#include "variable.hpp"
#include "rocket/refcounted_ptr.hpp"

// TODO
#include <unordered_set>

namespace Asteria {

class Variable_hashset
  {
  private:
    rocket::refcounted_ptr<Variable> *m_data;
    Size m_nbkt;
    Size m_size;

// TODO
struct phash
  {
    Size operator()(const rocket::refcounted_ptr<Variable> &p) const noexcept
      {
        return std::hash<void *>()(p.get());
      }
  };
std::unordered_set<rocket::refcounted_ptr<Variable>, phash> m_set;

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
    void do_rehash(Size nbkt);

  public:
    bool empty() const noexcept
      {
// TODO
return this->m_set.empty();
        return this->m_size == 0;
      }
    Size size() const noexcept
      {
// TODO
return this->m_set.size();
        return this->m_size;
      }
    void clear() noexcept
      {
// TODO
return this->m_set.clear();
        const auto bptr = this->m_data;
        auto eptr = bptr + this->m_nbkt;
        while(eptr != bptr) {
          --eptr;
          eptr->reset();
        }
        this->m_size = 0;
      }

    template<typename FuncT>
      void for_each(FuncT &&func) const
      {
// TODO
return std::for_each(this->m_set.begin(), this->m_set.end(), std::forward<FuncT>(func)), (void)0;
        auto bptr = this->m_data;
        const auto eptr = bptr + this->m_nbkt;
        while(bptr != eptr) {
          if(*bptr) {
            std::forward<FuncT>(func)(*bptr);
          }
          ++bptr;
        }
      }
    bool has(const rocket::refcounted_ptr<Variable> &var) const
      {
// TODO
return this->m_set.count(var);
        return false;
      }

    bool insert(const rocket::refcounted_ptr<Variable> &var)
      {
// TODO
return this->m_set.insert(var).second;
        return true;
      }
    bool erase(const rocket::refcounted_ptr<Variable> &var) noexcept
      {
// TODO
return this->m_set.erase(var);
        return true;
      }


    void swap(Variable_hashset &other) noexcept
      {
// TODO
return this->m_set.swap(other.m_set);
         std::swap(this->m_data, other.m_data);
         std::swap(this->m_nbkt, other.m_nbkt);
         std::swap(this->m_size, other.m_size);
      }
  };

}

#endif
