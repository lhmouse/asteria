// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable_hashset.hpp"
#include "../runtime/variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

void
Variable_HashSet::
do_destroy_buckets()
noexcept
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = ::std::exchange(next, next->next);

      // Destroy this bucket.
      ROCKET_ASSERT(*qbkt);
      ::rocket::destroy_at(qbkt->kstor);
      qbkt->prev = nullptr;
    }
#ifdef ROCKET_DEBUG
    this->m_head = reinterpret_cast<Bucket*>(0xDEADBEEF);
#endif
  }

Variable_HashSet::Bucket*
Variable_HashSet::
do_xprobe(const rcptr<Variable>& var)
const
noexcept
  {
    auto bptr = this->m_bptr;
    auto eptr = this->m_eptr;

    // Find a bucket using linear probing.
    // We keep the load factor below 1.0 so there will always be some empty buckets in the table.
    auto mptr = ::rocket::get_probing_origin(bptr, eptr, reinterpret_cast<uintptr_t>(var.get()));
    auto qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr,
                                       [&](const Bucket& r) { return r.kstor[0] == var;  });
    ROCKET_ASSERT(qbkt);
    return qbkt;
  }

void
Variable_HashSet::
do_xrelocate_but(Bucket* qxcld)
noexcept
  {
    auto bptr = this->m_bptr;
    auto eptr = this->m_eptr;

    // Reallocate buckets that follow `*qbkt`.
    ::rocket::linear_probe(
      // Only probe non-erased buckets.
      bptr, qxcld, qxcld + 1, eptr,
      // Relocate every bucket found.
      [&](Bucket& rb) {
        auto qbkt = &rb;

        // Transfer ownership of the old variable, then detach the bucket.
        ROCKET_ASSERT(*qbkt);
        auto var = ::std::move(qbkt->kstor[0]);
        ::rocket::destroy_at(qbkt->kstor);
        this->do_list_detach(qbkt);

        // Find a new bucket for the variable using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
        auto mptr = ::rocket::get_probing_origin(bptr, eptr, reinterpret_cast<uintptr_t>(var.get()));
        qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
        ROCKET_ASSERT(qbkt);

        // Insert the variable into the new bucket.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        ::rocket::construct_at(qbkt->kstor, ::std::move(var));
        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void
Variable_HashSet::
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
Variable_HashSet::
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
Variable_HashSet::
do_rehash(size_t nbkt)
  {
    ROCKET_ASSERT(nbkt / 2 > this->m_size);
    // Allocate a new table.
    if(nbkt > PTRDIFF_MAX / sizeof(Bucket))
      throw ::std::bad_array_new_length();
    auto bptr = static_cast<Bucket*>(::operator new(nbkt * sizeof(Bucket)));
    auto eptr = bptr + nbkt;

    // Initialize an empty table.
    for(auto qbkt = bptr;  qbkt != eptr;  ++qbkt)
      qbkt->prev = nullptr;
    auto bold = ::std::exchange(this->m_bptr, bptr);
    this->m_eptr = eptr;

    // Move buckets into the new table.
    // Warning: No exception shall be thrown from the code below.
    auto next = ::std::exchange(this->m_head, nullptr);
    while(ROCKET_EXPECT(next)) {
      auto qbkt = ::std::exchange(next, next->next);

      // Transfer ownership of the old variable, then destroy the bucket.
      ROCKET_ASSERT(*qbkt);
      auto var = ::std::move(qbkt->kstor[0]);
      ::rocket::destroy_at(qbkt->kstor);
      qbkt->prev = nullptr;

      // Find a new bucket for the variable using linear probing.
      // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
      auto mptr = ::rocket::get_probing_origin(bptr, eptr, reinterpret_cast<uintptr_t>(var.get()));
      qbkt = ::rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
      ROCKET_ASSERT(qbkt);

      // Insert the variable into the new bucket.
      ROCKET_ASSERT(!*qbkt);
      this->do_list_attach(qbkt);
      ::rocket::construct_at(qbkt->kstor, ::std::move(var));
    }
    // Deallocate the old table.
    if(bold)
      ::operator delete(bold);
  }

void
Variable_HashSet::
do_attach(Bucket* qbkt, const rcptr<Variable>& var)
noexcept
  {
    // Construct the node, then attach it.
    ROCKET_ASSERT(!*qbkt);
    this->do_list_attach(qbkt);
    ::rocket::construct_at(qbkt->kstor, var);
    ROCKET_ASSERT(*qbkt);
    this->m_size++;
  }

void
Variable_HashSet::
do_detach(Bucket* qbkt)
noexcept
  {
    // Transfer ownership of the old variable, then detach the bucket.
    this->m_size--;
    ROCKET_ASSERT(*qbkt);
    ::rocket::destroy_at(qbkt->kstor);
    this->do_list_detach(qbkt);
    ROCKET_ASSERT(!*qbkt);

    // Relocate nodes that follow `qbkt`, if any.
    this->do_xrelocate_but(qbkt);
  }

Variable_Callback&
Variable_HashSet::
enumerate_variables(Variable_Callback& callback)
const
  {
    auto next = this->m_head;
    while(ROCKET_EXPECT(next)) {
      auto qbkt = ::std::exchange(next, next->next);

      // Enumerate a child variable.
      ROCKET_ASSERT(*qbkt);
      if(!callback.process(qbkt->kstor[0]))
        continue;

      // Enumerate grandchildren recursively.
      qbkt->kstor[0]->enumerate_variables(callback);
    }
    return callback;
  }

}  // namespace Asteria
