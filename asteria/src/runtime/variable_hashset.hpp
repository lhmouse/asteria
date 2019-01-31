// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HASHSET_HPP_

#include "../fwd.hpp"
#include "variable.hpp"

namespace Asteria {

class Variable_HashSet
  {
  private:
    struct Bucket
      {
        // A null pointer indicates an empty bucket.
        RefCnt_Ptr<Variable> first;
        // For the first bucket:  `size` is the number of non-empty buckets in this container.
        // For every other bucket: `prev` points to the previous non-empty bucket.
        union { std::size_t size; Bucket *prev; };
        // For the last bucket:   `reserved` is reserved for future use.
        // For every other bucket: `next` points to the next non-empty bucket.
        union { std::size_t reserved; Bucket *next; };

        Bucket() noexcept
          : first()
          {
          }
        ~Bucket()
          {
          }

        Bucket(const Bucket &)
          = delete;
        Bucket & operator=(const Bucket &)
          = delete;

        explicit operator bool () const noexcept
          {
            return this->first != nullptr;
          }
      };

  private:
    static void do_attach_bucket(Bucket *self, Bucket *ipos) noexcept;
    static void do_detach_bucket(Bucket *self) noexcept;

  private:
    // The first and last buckets are permanently reserved.
    CoW_Vector<Bucket> m_stor;

  public:
    Variable_HashSet() noexcept
      : m_stor()
      {
      }

    Variable_HashSet(const Variable_HashSet &)
      = delete;
    Variable_HashSet & operator=(const Variable_HashSet &)
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

    bool has(const RefCnt_Ptr<Variable> &var) const noexcept;
    void for_each(const Abstract_Variable_Callback &callback) const;
    bool insert(const RefCnt_Ptr<Variable> &var);
    bool erase(const RefCnt_Ptr<Variable> &var) noexcept;
    RefCnt_Ptr<Variable> erase_random_opt() noexcept;
  };

}

#endif
