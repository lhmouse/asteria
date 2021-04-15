// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_POINTER_HASHSET_HPP_
#define ASTERIA_LLDS_POINTER_HASHSET_HPP_

#include "../fwd.hpp"
#include "../details/pointer_hashset.ipp"

namespace asteria {

class Pointer_HashSet
  {
  private:
    using Bucket = details_pointer_hashset::Bucket;

    Bucket* m_bptr = nullptr;  // beginning of bucket storage
    Bucket* m_eptr = nullptr;  // end of bucket storage
    Bucket* m_head = nullptr;  // the first initialized bucket
    size_t m_size = 0;         // number of initialized buckets

  public:
    explicit constexpr
    Pointer_HashSet() noexcept
      { }

    Pointer_HashSet(Pointer_HashSet&& other) noexcept
      { this->swap(other);  }

    Pointer_HashSet&
    operator=(Pointer_HashSet&& other) noexcept
      { return this->swap(other);  }

  private:
    void
    do_destroy_buckets() noexcept;

    // This function returns a pointer to either an empty bucket or a
    // bucket containing a key which is equal to `ptr`, but in no case
    // can a null pointer be returned.
    ROCKET_PURE_FUNCTION Bucket*
    do_xprobe(const void* ptr) const noexcept
      {
        auto bptr = this->m_bptr;
        auto eptr = this->m_eptr;

        // Find a bucket using linear probing.
        // We keep the load factor below 1.0 so there will always be some empty buckets
        // in the table.
        auto mptr = ::rocket::get_probing_origin(bptr, eptr,
                        reinterpret_cast<uintptr_t>(ptr));
        auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                        [&](const Bucket& r) { return r.key_ptr == ptr;  });

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
    do_attach(Bucket* qbkt, const void* ptr) noexcept
      {
        // Construct the node, then attach it.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        qbkt->key_ptr = ptr;
        ROCKET_ASSERT(*qbkt);
        this->m_size++;
      }

    void
    do_detach(Bucket* qbkt) noexcept
      {
        // Transfer ownership of the old pointer, then detach the bucket.
        this->m_size--;
        ROCKET_ASSERT(*qbkt);
        this->do_list_detach(qbkt);
        ROCKET_ASSERT(!*qbkt);

        // Relocate nodes that follow `qbkt`, if any.
        this->do_xrelocate_but(qbkt);
      }

    void
    do_rehash_more();

  public:
    ~Pointer_HashSet()
      {
        if(this->m_head)
          this->do_destroy_buckets();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0x93, sizeof(*this));
#endif
      }

    bool
    empty() const noexcept
      { return this->m_head == nullptr;  }

    size_t
    size() const noexcept
      { return this->m_size;  }

    Pointer_HashSet&
    clear() noexcept
      {
        if(this->m_head)
          this->do_destroy_buckets();

        // Clean invalid data up.
        this->m_head = nullptr;
        this->m_size = 0;
        return *this;
      }

    Pointer_HashSet&
    swap(Pointer_HashSet& other) noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_eptr, other.m_eptr);
        ::std::swap(this->m_head, other.m_head);
        ::std::swap(this->m_size, other.m_size);
        return *this;
      }

    bool
    has(const void* ptr) const noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the pointer.
        auto qbkt = this->do_xprobe(ptr);
        if(!*qbkt)
          return false;

        ROCKET_ASSERT(qbkt->key_ptr == ptr);
        return true;
      }

    bool
    insert(const void* ptr) noexcept
      {
        // Reserve more room by rehashing if the load factor would
        // exceed 0.5.
        auto nbkt = static_cast<size_t>(this->m_eptr - this->m_bptr);
        if(ROCKET_UNEXPECT(this->m_size >= nbkt / 2))
          this->do_rehash_more();

        // Find a bucket for the new pointer. Fail if it already exists.
        auto qbkt = this->do_xprobe(ptr);
        if(*qbkt)
          return false;

        // Insert the new pointer into this empty bucket.
        this->do_attach(qbkt, ptr);
        return true;
      }

    bool
    erase(const void* ptr) noexcept
      {
        // Be advised that `do_xprobe()` shall not be called when the
        // table has not been allocated.
        if(!this->m_bptr)
          return false;

        // Find the bucket for the pointer.
        auto qbkt = this->do_xprobe(ptr);
        if(!*qbkt)
          return false;

        // Detach this pointer.
        this->do_detach(qbkt);
        return true;
      }
  };

inline void
swap(Pointer_HashSet& lhs, Pointer_HashSet& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
