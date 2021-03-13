// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_LLDS_REFERENCE_DICTIONARY_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
#include "../details/reference_dictionary.ipp"

namespace asteria {

class Reference_Dictionary
  {
  private:
    using Bucket = details_reference_dictionary::Bucket;

    Bucket* m_bptr = nullptr;  // beginning of bucket storage
    Bucket* m_eptr = nullptr;  // end of bucket storage
    Bucket* m_head = nullptr;  // the first initialized bucket
    size_t m_size = 0;         // number of initialized buckets

  public:
    explicit constexpr
    Reference_Dictionary()
      noexcept
      { }

    Reference_Dictionary(Reference_Dictionary&& other)
      noexcept
      { this->swap(other);  }

    Reference_Dictionary&
    operator=(Reference_Dictionary&& other)
      noexcept
      { return this->swap(other);  }

  private:
    void
    do_destroy_buckets()
      noexcept;

    // This function returns a pointer to either an empty bucket or a
    // bucket containing a key which is equal to `name`, but in no case
    // can a null pointer be returned.
    Bucket*
    do_xprobe(const phsh_string& name)
      const noexcept;

    // This function is used for relocation after an element is erased.
    void
    do_xrelocate_but(Bucket* qxcld)
      noexcept;

    // Valid buckets are linked altogether for efficient iteration.
    void
    do_list_attach(Bucket* qbkt)
      noexcept
      {
        // Insert the bucket before `head`.
        auto next = ::std::exchange(this->m_head, qbkt);
        // Update the forward list, which is non-circular.
        qbkt->next = next;
        // Update the backward list, which is circular.
        qbkt->prev = next ? ::std::exchange(next->prev, qbkt) : qbkt;
      }

    void
    do_list_detach(Bucket* qbkt)
      noexcept
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
    do_attach(Bucket* qbkt, const phsh_string& name)
      noexcept
      {
        // Construct the node, then attach it.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        ::rocket::construct_at(qbkt->kstor, name);
        ::rocket::construct_at(qbkt->vstor, Reference::S_uninit());
        ROCKET_ASSERT(*qbkt);
        this->m_size++;
      }

    void
    do_detach(Bucket* qbkt)
      noexcept
      {
        // Destroy the old name and reference, then detach the bucket.
        this->m_size--;
        ROCKET_ASSERT(*qbkt);
        ::rocket::destroy_at(qbkt->kstor);
        ::rocket::destroy_at(qbkt->vstor);
        this->do_list_detach(qbkt);
        ROCKET_ASSERT(!*qbkt);

        // Relocate nodes that follow `qbkt`, if any.
        this->do_xrelocate_but(qbkt);
      }

    // This function is primarily used to reallocate a larger table.
    void
    do_rehash(size_t nbkt);

  public:
    ~Reference_Dictionary()
      {
        if(this->m_head)
          this->do_destroy_buckets();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0xA6, sizeof(*this));
#endif
      }

    bool
    empty()
      const noexcept
      { return this->m_head == nullptr;  }

    size_t
    size()
      const noexcept
      { return this->m_size;  }

    Reference_Dictionary&
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

    Reference_Dictionary&
    swap(Reference_Dictionary& other)
      noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_eptr, other.m_eptr);
        ::std::swap(this->m_head, other.m_head);
        ::std::swap(this->m_size, other.m_size);
        return *this;
      }

    const Reference*
    find_opt(const phsh_string& name)
      const noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return nullptr;

        // Find the bucket for the name.
        auto qbkt = this->do_xprobe(name);
        if(!*qbkt)
          return nullptr;

        ROCKET_ASSERT(qbkt->kstor[0].rdhash() == name.rdhash());
        return qbkt->vstor;
      }

    Reference*
    mut_find_opt(const phsh_string& name)
      noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return nullptr;

        // Find the bucket for the name.
        auto qbkt = this->do_xprobe(name);
        if(!*qbkt)
          return nullptr;

        ROCKET_ASSERT(qbkt->kstor[0].rdhash() == name.rdhash());
        return qbkt->vstor;
      }

    pair<Reference*, bool>
    insert(const phsh_string& name)
      {
        // Reserve more room by rehashing if the load factor would
        // exceed 0.5.
        auto nbkt = static_cast<size_t>(this->m_eptr - this->m_bptr);
        if(ROCKET_UNEXPECT(this->m_size >= nbkt / 2))
          this->do_rehash(this->m_size * 3 | 9);

        // Find a bucket for the new name.
        auto qbkt = this->do_xprobe(name);
        if(*qbkt)
          return ::std::make_pair(qbkt->vstor, false);

        // Construct a null reference and return it.
        this->do_attach(qbkt, name);
        return ::std::make_pair(qbkt->vstor, true);
      }

    bool
    erase(const phsh_string& name)
      noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the name.
        auto qbkt = this->do_xprobe(name);
        if(!*qbkt)
          return false;

        // Detach this reference.
        this->do_detach(qbkt);
        return true;
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const;
  };

inline
void
swap(Reference_Dictionary& lhs, Reference_Dictionary& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
