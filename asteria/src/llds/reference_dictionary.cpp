// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_dictionary.hpp"
#include "../runtime/variable_callback.hpp"
#include "../utils.hpp"

namespace asteria {

void
Reference_Dictionary::
do_destroy_buckets()
  noexcept
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = next;
      next = qbkt->next;

      // Destroy this bucket.
      ROCKET_ASSERT(*qbkt);
      ::rocket::destroy_at(qbkt->kstor);
      ::rocket::destroy_at(qbkt->vstor);
      qbkt->next = nullptr;
    }
#ifdef ROCKET_DEBUG
    ::std::for_each(this->m_bptr, this->m_eptr, [&](Bucket& r) {
              ::std::memset((void*)&r, 0xD3, sizeof(r));  r.prev = nullptr;  });
#endif
    this->m_head = reinterpret_cast<Bucket*>(0xDEADBEEF);
  }

details_reference_dictionary::Bucket*
Reference_Dictionary::
do_xprobe(const phsh_string& name)
  const noexcept
  {
    auto bptr = this->m_bptr;
    auto eptr = this->m_eptr;

    // Find a bucket using linear probing.
    // We keep the load factor below 1.0 so there will always be some empty buckets
    // in the table.
    auto mptr = ::rocket::get_probing_origin(bptr, eptr, name.rdhash());
    auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                            [&](const Bucket& r) { return r.kstor[0] == name;  });
    ROCKET_ASSERT(qbkt);
    return qbkt;
  }

void
Reference_Dictionary::
do_xrelocate_but(Bucket* qxcld)
  noexcept
  {
    ::rocket::linear_probe(
      this->m_bptr,
      qxcld,
      qxcld + 1,
      this->m_eptr,
      [&](Bucket& rb) {
        auto sbkt = &rb;

        // Mark this bucket empty, without destroying its contents.
        ROCKET_ASSERT(*sbkt);
        this->do_list_detach(sbkt);

        // Find a new bucket for the name using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need
        // to check for collisions.
        auto mptr = ::rocket::get_probing_origin(this->m_bptr, this->m_eptr,
                                                 sbkt->kstor->rdhash());
        auto qbkt = ::rocket::linear_probe(this->m_bptr, mptr, mptr, this->m_eptr,
                                           [&](const Bucket&) { return false;  });
        ROCKET_ASSERT(qbkt);

        // Mark the new bucket non-empty.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);

        // If the two pointers reference the same one, no relocation is needed.
        if(ROCKET_EXPECT(qbkt == sbkt))
          return false;

        // Relocate the bucket.
        ::rocket::construct_at(qbkt->kstor, ::std::move(sbkt->kstor[0]));
        ::rocket::destroy_at(sbkt->kstor);
        ::rocket::construct_at(qbkt->vstor, ::std::move(sbkt->vstor[0]));
        ::rocket::destroy_at(sbkt->vstor);

        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void
Reference_Dictionary::
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
Reference_Dictionary::
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
Reference_Dictionary::
do_rehash(size_t nbkt)
  {
    ROCKET_ASSERT(nbkt / 2 > this->m_size);

    // Allocate a new table.
    if(nbkt > PTRDIFF_MAX / sizeof(Bucket))
      throw ::std::bad_array_new_length();

    auto bptr = static_cast<Bucket*>(::operator new(nbkt * sizeof(Bucket)));
    auto eptr = bptr + nbkt;

    // Initialize an empty table.
    ::std::for_each(bptr, eptr, [&](Bucket& r) { r.prev = nullptr;  });
    auto bold = ::std::exchange(this->m_bptr, bptr);
    this->m_eptr = eptr;

    // Move buckets into the new table.
    // Warning: No exception shall be thrown from the code below.
    auto sbkt = ::std::exchange(this->m_head, nullptr);
    while(ROCKET_EXPECT(sbkt)) {
      ROCKET_ASSERT(*sbkt);

      // Find a new bucket for the name using linear probing.
      // Uniqueness has already been implied for all elements, so there is no need
      // to check for collisions.
      auto mptr = ::rocket::get_probing_origin(bptr, eptr, sbkt->kstor->rdhash());
      auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                                         [&](const Bucket&) { return false;  });
      ROCKET_ASSERT(qbkt);

      // Mark the new bucket non-empty.
      ROCKET_ASSERT(!*qbkt);
      this->do_list_attach(qbkt);

      // Relocate the bucket.
      ::rocket::construct_at(qbkt->kstor, ::std::move(sbkt->kstor[0]));
      ::rocket::destroy_at(sbkt->kstor);
      ::rocket::construct_at(qbkt->vstor, ::std::move(sbkt->vstor[0]));
      ::rocket::destroy_at(sbkt->vstor);

      // Process the next bucket.
      sbkt = sbkt->next;
    }
    if(bold)
      ::operator delete(bold);
  }

void
Reference_Dictionary::
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
Reference_Dictionary::
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

Variable_Callback&
Reference_Dictionary::
enumerate_variables(Variable_Callback& callback)
  const
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = next;
      next = qbkt->next;

      // Enumerate child variables.
      ROCKET_ASSERT(*qbkt);
      qbkt->vstor[0].enumerate_variables(callback);
    }
    return callback;
  }

}  // namespace asteria
