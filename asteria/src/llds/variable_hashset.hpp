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
        Bucket* next;  // the next bucket in the [non-circular] list
        Bucket* prev;  // the previous bucket in the [circular] list
        union { rcptr<Variable> kstor[1];  };  // initialized iff `prev` is non-null

        Bucket() noexcept { }
        ~Bucket() { }
        explicit operator bool () const noexcept { return this->prev != nullptr;  }
      };

    struct Storage
      {
        Bucket* bptr;  // beginning of bucket storage
        Bucket* eptr;  // end of bucket storage
        Bucket* head;  // the first initialized bucket
        size_t size;  // number of initialized buckets
      };
    Storage m_stor;

  public:
    constexpr Variable_HashSet() noexcept
      :
        m_stor()
      { }
    Variable_HashSet(Variable_HashSet&& other) noexcept
      :
        m_stor()
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
        if(this->m_stor.head) {
          this->do_destroy_buckets();
        }
        if(this->m_stor.bptr) {
          ::operator delete(this->m_stor.bptr);
        }
#ifdef ROCKET_DEBUG
        std::memset(std::addressof(this->m_stor), 0xD2, sizeof(m_stor));
#endif
      }

  private:
    void do_destroy_buckets() const noexcept;
    void do_enumerate_variables(Variable_Callback& callback) const;

    Bucket* do_xprobe(const rcptr<Variable>& var) const noexcept;
    void do_xrelocate_but(Bucket* qxcld) noexcept;

    inline void do_list_attach(Bucket* qbkt) noexcept;
    inline void do_list_detach(Bucket* qbkt) noexcept;

    void do_rehash(size_t nbkt);
    void do_attach(Bucket* qbkt, const rcptr<Variable>& var) noexcept;
    void do_detach(Bucket* qbkt) noexcept;

  public:
    bool empty() const noexcept
      {
        return this->m_stor.head == nullptr;
      }
    Variable_HashSet& clear() noexcept
      {
        if(this->m_stor.head) {
          this->do_destroy_buckets();
        }
        // Clean invalid data up.
        this->m_stor.head = nullptr;
        this->m_stor.size = 0;
        return *this;
      }

    void swap(Variable_HashSet& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
      }

    size_t size() const noexcept
      {
        return this->m_stor.size;
      }
    bool has(const rcptr<Variable>& var) const noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the table has not been allocated.
        if(!this->m_stor.bptr) {
          return false;
        }
        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt) {
          // Not found.
          return false;
        }
        ROCKET_ASSERT(qbkt->kstor[0] == var);
        return true;
      }
    bool insert(const rcptr<Variable>& var) noexcept
      {
        // Reserve more room by rehashing if the load factor would exceed 0.5.
        auto nbkt = static_cast<size_t>(this->m_stor.eptr - this->m_stor.bptr);
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
    bool erase(const rcptr<Variable>& var) noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the table has not been allocated.
        if(!this->m_stor.bptr) {
          return false;
        }
        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt) {
          // Not found.
          return false;
        }
        // Detach this variable. It cannot be unique because `var` outlives this function.
        qbkt->kstor[0].release()->drop_reference();
        this->do_detach(qbkt);
        return true;
      }
    rcptr<Variable> erase_random_opt() noexcept
      {
        // Get a random bucket that contains a variable.
        auto qbkt = this->m_stor.head;
        if(!qbkt) {
          // Empty.
          return nullptr;
        }
        // Detach this variable and return it.
        auto var = rocket::move(qbkt->kstor[0]);
        this->do_detach(qbkt);
        return var;
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        this->do_enumerate_variables(callback);
        return callback;
      }
  };

inline void swap(Variable_HashSet& lhs, Variable_HashSet& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
