// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHMAP_HPP_
#define ASTERIA_LLDS_VARIABLE_HASHMAP_HPP_

#include "../fwd.hpp"
#include "../runtime/variable.hpp"
#include "../details/variable_hashmap.ipp"

namespace asteria {

class Variable_HashMap
  {
  private:
    using Bucket = details_variable_hashmap::Bucket;

    Bucket* m_bptr = nullptr;  // beginning of bucket storage
    Bucket* m_eptr = nullptr;  // end of bucket storage
    Bucket* m_head = nullptr;  // the first initialized bucket
    size_t m_size = 0;         // number of initialized buckets

  public:
    explicit constexpr
    Variable_HashMap() noexcept
      { }

    Variable_HashMap(Variable_HashMap&& other) noexcept
      { this->swap(other);  }

    Variable_HashMap&
    operator=(Variable_HashMap&& other) noexcept
      { return this->swap(other);  }

    Variable_HashMap&
    swap(Variable_HashMap& other) noexcept
      { ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_eptr, other.m_eptr);
        ::std::swap(this->m_head, other.m_head);
        ::std::swap(this->m_size, other.m_size);
        return *this;  }

  private:
    void
    do_destroy_buckets(bool xfree) noexcept;

    // This function returns a pointer to either an empty bucket or a
    // bucket containing a key which is equal to `name`, but in no case
    // can a null pointer be returned.
    ROCKET_PURE Bucket*
    do_xprobe(const void* key_p) const noexcept
      {
        // Find a bucket using linear probing.
        auto mptr = ::rocket::get_probing_origin(this->m_bptr, this->m_eptr,
                          reinterpret_cast<uintptr_t>(key_p));
        auto qbkt = ::rocket::linear_probe(this->m_bptr, mptr, mptr, this->m_eptr,
                          [&](const Bucket& r) { return r.key_p == key_p;  });

        // The load factor is kept <= 0.5 so there must always be a bucket available.
        ROCKET_ASSERT(qbkt);
        return qbkt;
      }

    // This function is used for relocation after an element is erased.
    void
    do_xrelocate_but(Bucket* qxcld) noexcept;

    // Valid buckets are linked altogether for efficient iteration.
    void
    do_list_attach(Bucket* qbkt) noexcept
      {
        // Insert the bucket before `head`.
        auto next = ::std::exchange(this->m_head, qbkt);
        // Update the forward list, which is non-circular.
        qbkt->next = next;
        // Update the backward list, which is circular.
        qbkt->prev = next ? ::std::exchange(next->prev, qbkt) : qbkt;
      }

    void
    do_list_detach(Bucket* qbkt) noexcept
      {
        auto next = qbkt->next;
        auto prev = qbkt->prev;
        auto head = this->m_head;

        // Update the forward list, which is non-circular.
        ((qbkt == head) ? this->m_head : prev->next) = next;
        // Update the backward list, which is circular.
        (next ? next : head)->prev = prev;
        // Mark the bucket empty.
        qbkt->prev = nullptr;
      }

    void
    do_attach(Bucket* qbkt, const void* key_p, const rcptr<Variable>& var) noexcept
      {
        // Construct the node, then attach it.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        qbkt->key_p = key_p;
        ::rocket::construct(qbkt->vstor, var);
        ROCKET_ASSERT(*qbkt);
        this->m_size++;
      }

    void
    do_detach(Bucket* qbkt) noexcept
      {
        // Destroy the old variable, then detach the bucket.
        this->m_size--;
        ROCKET_ASSERT(*qbkt);
        ::rocket::destroy(qbkt->vstor);
        this->do_list_detach(qbkt);
        ROCKET_ASSERT(!*qbkt);

        // Relocate nodes that follow `qbkt`, if any.
        this->do_xrelocate_but(qbkt);
      }

    void
    do_rehash_more(size_t nadd);

    size_t
    do_merge_into(Variable_HashMap& other) const;

  public:
    ~Variable_HashMap()
      {
        if(this->m_bptr)
          this->do_destroy_buckets(true);
      }

    bool
    empty() const noexcept
      { return this->m_head == nullptr;  }

    size_t
    size() const noexcept
      { return this->m_size;  }

    size_t
    capacity() const noexcept
      { return static_cast<size_t>(this->m_eptr - this->m_bptr) / 2;  }

    Variable_HashMap&
    clear() noexcept
      {
        if(this->m_head)
          this->do_destroy_buckets(false);

        // Clean invalid data up.
        this->m_head = nullptr;
        this->m_size = 0;
        return *this;
      }

    const rcptr<Variable>*
    find_opt(const void* key_p) const noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return nullptr;

        // Find the bucket for the name.
        auto qbkt = this->do_xprobe(key_p);
        if(!*qbkt)
          return nullptr;

        ROCKET_ASSERT(qbkt->key_p == key_p);
        return qbkt->vstor;
      }

    Variable_HashMap&
    reserve_more(size_t nadd)
      {
        this->do_rehash_more(nadd);
        return *this;
      }

    bool
    insert(const void* key_p, const rcptr<Variable>& var_opt)
      {
        // Reserve more room by rehashing if the load factor would
        // exceed 0.5.
        if(ROCKET_UNEXPECT(this->m_size >= this->capacity()))
          this->do_rehash_more(1);

        // Find a bucket for the new name.
        auto qbkt = this->do_xprobe(key_p);
        if(*qbkt)
          return false;

        // Construct the new element.
        this->do_attach(qbkt, key_p, var_opt);
        return true;
      }

    bool
    erase(const void* key_p) noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the name.
        auto qbkt = this->do_xprobe(key_p);
        if(!*qbkt)
          return false;

        // Detach this element.
        this->do_detach(qbkt);
        return true;
      }

    bool
    erase_random(const void** key_p_out_opt, rcptr<Variable>* var_out_opt)
      {
        // Get a random bucket that contains a variable.
        auto qbkt = this->m_head;
        if(!qbkt)
          return false;

        // Move this element out.
        if(key_p_out_opt)
          *key_p_out_opt = qbkt->key_p;

        if(var_out_opt)
          *var_out_opt = ::std::move(qbkt->vstor[0]);

        // Detach this element.
        this->do_detach(qbkt);
        return true;
      }

    size_t
    merge(const Variable_HashMap& other)
      {
        ROCKET_ASSERT(this != &other);
        return other.do_merge_into(*this);
      }
  };

inline void
swap(Variable_HashMap& lhs, Variable_HashMap& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
