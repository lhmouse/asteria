// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_

#include "../fwd.hpp"
#include "variable.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Variable_hashset
  {
  private:
    struct Bucket
      {
        union {
          std::size_t size;  // of the first bucket
          Bucket *prev;      // of the rest
        };
        union {
          std::size_t resv;  // of the last bucket
          Bucket *next;      // of the rest
        };
        rocket::refcounted_ptr<Variable> var;

        Bucket() noexcept
          : prev(nullptr), next(nullptr), var()
          {
          }
        ROCKET_MOVABLE_DESTRUCTOR(Bucket)
          {
          }

        explicit operator bool () const noexcept
          {
            return bool(this->var);
          }
      };

    // The first and last buckets are permanently reserved.
    rocket::cow_vector<Bucket> m_stor;

  public:
    Variable_hashset() noexcept
      : m_stor()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Variable_hashset);

  private:
    void do_clear() noexcept;
    void do_rehash(std::size_t res_arg);
    void do_check_relocation(Bucket *from, Bucket *to);

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

    bool has(const rocket::refcounted_ptr<Variable> &var) const noexcept;
    void for_each(const Abstract_variable_callback &callback) const;
    bool insert(const rocket::refcounted_ptr<Variable> &var);
    bool erase(const rocket::refcounted_ptr<Variable> &var) noexcept;
  };

}

#endif
