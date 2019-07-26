// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable_hashset.hpp"
#include "../runtime/abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variable_HashSet::Bucket* Variable_HashSet::do_allocate_table(std::size_t nbkt)
  {
    auto bptr = static_cast<Bucket*>(std::calloc(nbkt, sizeof(Bucket)));
    if(!bptr) {
      throw std::bad_alloc();
    }
    return bptr;
  }

void Variable_HashSet::do_free_table(Variable_HashSet::Bucket* bptr) noexcept
  {
    std::free(bptr);
  }

void Variable_HashSet::do_clear_buckets() const noexcept
  {
    auto next = this->m_stor.aptr;
    if(ROCKET_EXPECT(next)) {
      auto origin = next;
      do {
        auto qbkt = std::exchange(next, next->next);
        // Destroy this bucket.
        ROCKET_ASSERT(*qbkt);
        rocket::destroy_at(qbkt->kstor);
        qbkt->next = nullptr;
        // Stop if the head is encountered a second time, as the linked list is circular.
      } while(ROCKET_EXPECT(next != origin));
    }
  }

Variable_HashSet::Bucket* Variable_HashSet::do_xprobe(const Rcptr<Variable>& var) const noexcept
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = this->m_stor.eptr;
    // Find a bucket using linear probing.
    // We keep the load factor below 1.0 so there will always be some empty buckets in the table.
    auto mptr = rocket::get_probing_origin(bptr, eptr, reinterpret_cast<std::uintptr_t>(var.get()));
    auto qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket& r) { return r.kstor[0] == var;  });
    ROCKET_ASSERT(qbkt);
    return qbkt;
  }

void Variable_HashSet::do_xrelocate_but(Variable_HashSet::Bucket* qxcld) noexcept
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = this->m_stor.eptr;
    // Reallocate buckets that follow `*qbkt`.
    rocket::linear_probe(
      // Only probe non-erased buckets.
      bptr, qxcld, qxcld + 1, eptr,
      // Relocate every bucket found.
      [&](Bucket& r) {
        auto qbkt = std::addressof(r);
        // Transfer ownership of the old variable, then detach the bucket.
        ROCKET_ASSERT(*qbkt);
        auto var = rocket::move(qbkt->kstor[0]);
        rocket::destroy_at(qbkt->kstor);
        this->do_list_detach(qbkt);
        // Find a new bucket for the variable using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
        auto mptr = rocket::get_probing_origin(bptr, eptr, reinterpret_cast<std::uintptr_t>(var.get()));
        qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
        ROCKET_ASSERT(qbkt);
        // Insert the variable into the new bucket.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        rocket::construct_at(qbkt->kstor, rocket::move(var));
        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void Variable_HashSet::do_list_attach(Variable_HashSet::Bucket* qbkt) noexcept
  {
    auto next = std::exchange(this->m_stor.aptr, qbkt);
    // Note the circular list.
    if(ROCKET_EXPECT(next)) {
      auto prev = next->prev;
      // Insert the node between `prev` and `next`.
      prev->next = qbkt;
      next->prev = qbkt;
      // Set up pointers in `qbkt`.
      qbkt->next = next;
      qbkt->prev = prev;
    }
    else {
      // Set up the first node.
      qbkt->next = qbkt;
      qbkt->prev = qbkt;
    }
  }

void Variable_HashSet::do_list_detach(Variable_HashSet::Bucket* qbkt) noexcept
  {
    auto next = std::exchange(qbkt->next, nullptr);
    // Note the circular list.
    if(ROCKET_EXPECT(next != qbkt)) {
      auto prev = qbkt->prev;
      // Remove the node from `prev` and `next`.
      prev->next = next;
      next->prev = prev;
      // Make `aptr` point to some valid bucket, should it equal `qbkt`.
      this->m_stor.aptr = next;
    }
    else {
      // Remove the last node.
      this->m_stor.aptr = nullptr;
    }
  }

void Variable_HashSet::do_rehash(std::size_t nbkt)
  {
    ROCKET_ASSERT(nbkt / 2 > this->m_stor.size);
    // Allocate a new table.
    auto bptr = this->do_allocate_table(nbkt);
    auto eptr = bptr + nbkt;
    // Set up an empty table.
    auto bold = std::exchange(this->m_stor.bptr, bptr);
    this->m_stor.eptr = eptr;
    auto next = std::exchange(this->m_stor.aptr, nullptr);
    // Move buckets into the new table.
    // Warning: No exception shall be thrown from the code below.
    if(ROCKET_EXPECT(next)) {
      auto origin = next;
      do {
        auto qbkt = std::exchange(next, next->next);
        // Transfer ownership of the old variable, then destroy the bucket.
        ROCKET_ASSERT(*qbkt);
        auto var = rocket::move(qbkt->kstor[0]);
        rocket::destroy_at(qbkt->kstor);
        qbkt->next = nullptr;
        // Find a new bucket for the variable using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
        auto mptr = rocket::get_probing_origin(bptr, eptr, reinterpret_cast<std::uintptr_t>(var.get()));
        qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
        ROCKET_ASSERT(qbkt);
        // Insert the variable into the new bucket.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        rocket::construct_at(qbkt->kstor, rocket::move(var));
        // Stop if the head is encountered a second time, as the linked list is circular.
      } while(ROCKET_EXPECT(next != origin));
    }
    // Deallocate the old table.
    if(bold) {
      this->do_free_table(bold);
    }
  }

void Variable_HashSet::do_attach(Variable_HashSet::Bucket* qbkt, const Rcptr<Variable>& var) noexcept
  {
    // Construct the node, then attch it.
    ROCKET_ASSERT(!*qbkt);
    this->do_list_attach(qbkt);
    rocket::construct_at(qbkt->kstor, var);
    this->m_stor.size++;
    ROCKET_ASSERT(*qbkt);
  }

Rcptr<Variable> Variable_HashSet::do_detach(Variable_HashSet::Bucket* qbkt) noexcept
  {
    // Transfer ownership of the old variable, then detach the bucket.
    ROCKET_ASSERT(*qbkt);
    auto var = rocket::move(qbkt->kstor[0]);
    this->m_stor.size--;
    rocket::destroy_at(qbkt->kstor);
    this->do_list_detach(qbkt);
    ROCKET_ASSERT(!*qbkt);
    // Relocate nodes that follow `qbkt`, if any.
    this->do_xrelocate_but(qbkt);
    return var;
  }

void Variable_HashSet::enumerate(const Abstract_Variable_Callback& callback) const
  {
    auto next = this->m_stor.aptr;
    if(ROCKET_EXPECT(next)) {
      auto origin = next;
      do {
        auto qbkt = std::exchange(next, next->next);
        // Enumerate child variables.
        ROCKET_ASSERT(*qbkt);
        bool recur = callback(qbkt->kstor[0]);
        if(recur) {
          // Enumerate grandchildren recursively.
          qbkt->kstor[0]->enumerate_variables(callback);
        }
        // Stop if the head is encountered a second time, as the linked list is circular.
      } while(ROCKET_EXPECT(next != origin));
    }
  }

}  // namespace Asteria
