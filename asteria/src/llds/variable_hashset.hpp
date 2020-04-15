// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

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

        Bucket()
          { }

        ~Bucket()
          { }

        explicit operator
        bool()
        const noexcept
          { return this->prev != nullptr;  }
      };

    Bucket* m_bptr = nullptr;  // beginning of bucket storage
    Bucket* m_eptr = nullptr;  // end of bucket storage
    Bucket* m_head = nullptr;  // the first initialized bucket
    size_t m_size = 0;         // number of initialized buckets

  public:
    constexpr
    Variable_HashSet()
    noexcept
      { }

    Variable_HashSet(Variable_HashSet&& other)
    noexcept
      { this->swap(other);  }

    Variable_HashSet&
    operator=(Variable_HashSet&& other)
    noexcept
      { return this->swap(other);  }

    ~Variable_HashSet()
      {
        if(this->m_head)
          this->do_destroy_buckets();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0x93, sizeof(*this));
#endif
      }

  private:
    void
    do_destroy_buckets()
    noexcept;

    // This function returns a pointer to either an empty bucket or a bucket containing
    // a key which is equal to `var`, but in no case can a null pointer be returned.
    Bucket*
    do_xprobe(const rcptr<Variable>& var)
    const noexcept;

    // This function is used for relocation after an element is erased.
    inline
    void
    do_xrelocate_but(Bucket* qxcld)
    noexcept;

    // Valid buckets are linked altogether for efficient iteration.
    inline
    void
    do_list_attach(Bucket* qbkt)
    noexcept;

    inline
    void
    do_list_detach(Bucket* qbkt)
    noexcept;

    // This function is primarily used to reallocate a larger table.
    void
    do_rehash(size_t nbkt);

    // This functions stores `var` in the bucket `*qbkt`.
    // `*qbkt` must be empty.
    void
    do_attach(Bucket* qbkt, const rcptr<Variable>& var)
    noexcept;

    // This functions clears the bucket `*qbkt`
    // `*qbkt` must not be empty.
    void
    do_detach(Bucket* qbkt)
    noexcept;

  public:
    bool
    empty()
    const noexcept
      { return this->m_head == nullptr;  }

    size_t
    size()
    const noexcept
      { return this->m_size;  }

    Variable_HashSet&
    clear()
    noexcept
      {
        if(this->m_head)
          this->do_destroy_buckets();

        // Clean invalid data up.
        this->m_head = nullptr;
        this->m_size = 0;
        return *this;
      }

    Variable_HashSet&
    swap(Variable_HashSet& other)
    noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_eptr, other.m_eptr);
        ::std::swap(this->m_head, other.m_head);
        ::std::swap(this->m_size, other.m_size);
        return *this;
      }

    bool
    has(const rcptr<Variable>& var)
    const noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt)
          return false;

        ROCKET_ASSERT(qbkt->kstor[0] == var);
        return true;
      }

    bool
    insert(const rcptr<Variable>& var)
    noexcept
      {
        // Reserve more room by rehashing if the load factor would exceed 0.5.
        auto nbkt = static_cast<size_t>(this->m_eptr - this->m_bptr);
        if(ROCKET_UNEXPECT(this->m_size >= nbkt / 2))
          // Ensure the number of buckets is an odd number.
          this->do_rehash(this->m_size * 3 | 97);

        // Find a bucket for the new variable. Fail if it already exists.
        auto qbkt = this->do_xprobe(var);
        if(*qbkt)
          return false;

        // Insert the new variable into this empty bucket.
        this->do_attach(qbkt, var);
        return true;
      }

    bool
    erase(const rcptr<Variable>& var)
    noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the variable.
        auto qbkt = this->do_xprobe(var);
        if(!*qbkt)
          return false;

        // Detach this variable. It cannot be unique because `var` outlives this function.
        qbkt->kstor[0].release()->drop_reference();
        this->do_detach(qbkt);
        return true;
      }

    rcptr<Variable>
    erase_random_opt()
    noexcept
      {
        // Get a random bucket that contains a variable.
        auto qbkt = this->m_head;
        if(!qbkt)
          return nullptr;

        // Detach this variable and return it.
        auto var = ::std::move(qbkt->kstor[0]);
        this->do_detach(qbkt);
        return var;
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(Variable_HashSet& lhs, Variable_HashSet& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace Asteria

#endif
