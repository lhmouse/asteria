// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHSET_HPP_
#define ASTERIA_LLDS_VARIABLE_HASHSET_HPP_

#include "../fwd.hpp"
#include "../runtime/variable.hpp"

namespace Asteria {

class Variable_HashSet
  {
  private:
    struct Bucket
      {
        Bucket* next;  // the next bucket in the circular list
        Bucket* prev;  // the previous bucket in the circular list
        union { Rcptr<Variable> kstor[1];  };  // initialized iff `next` is non-null

        Bucket() noexcept { }
        ~Bucket() { }
        explicit operator bool () const noexcept { return this->next != nullptr;  }
      };

    struct Storage
      {
        Bucket* bptr;  // beginning of bucket storage
        Bucket* eptr;  // end of bucket storage
        Bucket* aptr;  // any initialized bucket
        std::size_t size;  // number of initialized buckets
      };

    Storage m_stor;

  public:
    constexpr Variable_HashSet() noexcept
      : m_stor()
      {
      }
    Variable_HashSet(Variable_HashSet&& other) noexcept
      : m_stor()
      {
        std::swap(this->m_stor, other.m_stor);
      }
    Variable_HashSet& operator=(Variable_HashSet&& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
        return *this;
      }
    ~Variable_HashSet()
      {
        if(this->m_stor.aptr) {
          this->do_clear_buckets();
        }
        if(this->m_stor.bptr) {
          this->do_free_table(this->m_stor.bptr);
        }
#ifdef ROCKET_DEBUG
        std::memset(std::addressof(this->m_stor), 0xF9, sizeof(this->m_stor));
#endif
      }

  private:
    Bucket* do_allocate_table(std::size_t nbkt);
    void do_free_table(Bucket* bptr) noexcept;
    void do_clear_buckets() const noexcept;

    Bucket* do_xprobe(const Rcptr<Variable>& var) const noexcept;
    void do_xrelocate_but(Bucket* qxcld) noexcept;

    inline void do_list_attach(Bucket* qbkt) noexcept;
    inline void do_list_detach(Bucket* qbkt) noexcept;

    void do_rehash(std::size_t nbkt);
    void do_attach(Bucket* qbkt, const Rcptr<Variable>& var) noexcept;
    Rcptr<Variable> do_detach(Bucket* qbkt) noexcept;

  public:
    bool empty() const noexcept
      {
        return this->m_stor.aptr == nullptr;
      }
    std::size_t size() const noexcept
      {
        return this->m_stor.size;
      }
    void clear() noexcept
      {
        if(this->m_stor.aptr) {
          this->do_clear_buckets();
        }
        // Clean invalid data up.
        this->m_stor.aptr = nullptr;
        this->m_stor.size = 0;
      }

    void swap(Variable_HashSet& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
      }

    bool has(const Rcptr<Variable>& var) const noexcept
      {
        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt) {
          // Not found.
          return false;
        }
        ROCKET_ASSERT(qbkt->kstor[0] == var);
        return true;
      }
    bool insert(const Rcptr<Variable>& var) noexcept
      {
        // Reserve more room by rehashing if the load factor would exceed 0.5.
        auto nbkt = static_cast<std::size_t>(this->m_stor.eptr - this->m_stor.bptr);
        if(ROCKET_UNEXPECT(this->m_stor.size >= nbkt / 2)) {
          // Ensure the number of buckets is an odd number.
          this->do_rehash(this->m_stor.size * 3 | 97);
        }
        // Find a bucket for the new variable.
        auto qbkt = this->do_xprobe(var);
        if(*qbkt) {
          // Existent.
          return false;
        }
        // Insert the new variable into this empty bucket.
        this->do_attach(qbkt, var);
        return true;
      }
    bool erase(const Rcptr<Variable>& var) noexcept
      {
        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt) {
          // Not found.
          return false;
        }
        // Detach this variable. It cannot be unique because `var` outlives this function.
        this->do_detach(qbkt).release()->drop_reference();
        return true;
      }
    Rcptr<Variable> erase_random_opt() noexcept
      {
        // Get a random bucket that contains a variable.
        auto qbkt = this->m_stor.aptr;
        if(!qbkt) {
          // Empty.
          return nullptr;
        }
        // Detach this variable and return it.
        auto var = this->do_detach(qbkt);
        return var;
      }

    void enumerate(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Variable_HashSet& lhs, Variable_HashSet& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
