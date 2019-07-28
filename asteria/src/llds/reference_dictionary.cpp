// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_dictionary.hpp"
#include "../runtime/abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Reference_Dictionary::do_clear_buckets() const noexcept
  {
    auto next = this->m_stor.head;
    for(;;) {
      auto qbkt = next;
      if(ROCKET_UNEXPECT(!qbkt)) {
        break;
      }
      next = qbkt->next;
      // Destroy this bucket.
      ROCKET_ASSERT(*qbkt);
      rocket::destroy_at(qbkt->kstor);
      rocket::destroy_at(qbkt->vstor);
      qbkt->next = nullptr;
    }
  }

Reference_Dictionary::Bucket* Reference_Dictionary::do_xprobe(const PreHashed_String& name) const noexcept
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = this->m_stor.eptr;
    // Find a bucket using linear probing.
    // We keep the load factor below 1.0 so there will always be some empty buckets in the table.
    auto mptr = rocket::get_probing_origin(bptr, eptr, name.rdhash());
    auto qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket& r) { return r.kstor[0] == name;  });
    ROCKET_ASSERT(qbkt);
    return qbkt;
  }

void Reference_Dictionary::do_xrelocate_but(Reference_Dictionary::Bucket* qxcld) noexcept
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
        // Move the old name and reference out, then destroy the bucket.
        ROCKET_ASSERT(*qbkt);
        auto name = rocket::move(qbkt->kstor[0]);
        rocket::destroy_at(qbkt->kstor);
        auto refr = rocket::move(qbkt->vstor[0]);
        rocket::destroy_at(qbkt->vstor);
        this->do_list_detach(qbkt);
        // Find a new bucket for the name using linear probing.
        // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
        auto mptr = rocket::get_probing_origin(bptr, eptr, name.rdhash());
        qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
        ROCKET_ASSERT(qbkt);
        // Insert the reference into the new bucket.
        ROCKET_ASSERT(!*qbkt);
        this->do_list_attach(qbkt);
        rocket::construct_at(qbkt->kstor, rocket::move(name));
        rocket::construct_at(qbkt->vstor, rocket::move(refr));
        // Keep probing until an empty bucket is found.
        return false;
      });
  }

void Reference_Dictionary::do_list_attach(Reference_Dictionary::Bucket* qbkt) noexcept
  {
    // Insert the bucket before `head`.
    auto head = std::exchange(this->m_stor.head, qbkt);
    // Update the forward list, which is non-circular.
    qbkt->next = head;
    // Update the backward list, which is circular.
    qbkt->prev = head ? std::exchange(head->prev, qbkt) : qbkt;
  }

void Reference_Dictionary::do_list_detach(Reference_Dictionary::Bucket* qbkt) noexcept
  {
    auto next = qbkt->next;
    auto prev = qbkt->prev;
    auto head = this->m_stor.head;
    // Update the forward list, which is non-circular.
    ((qbkt == head) ? this->m_stor.head : prev->next) = next;
    // Update the backward list, which is circular.
    (next ? next : head)->prev = prev;
    // Mark the bucket empty.
    qbkt->prev = nullptr;
  }

void Reference_Dictionary::do_rehash(std::size_t nbkt)
  {
    ROCKET_ASSERT(nbkt / 2 > this->m_stor.size);
    // Allocate a new table.
    if(nbkt > PTRDIFF_MAX / sizeof(Bucket)) {
      throw std::bad_array_new_length();
    }
    auto bptr = static_cast<Bucket*>(::operator new(nbkt * sizeof(Bucket)));
    auto eptr = bptr + nbkt;
    // Initialize an empty table.
    for(auto qbkt = bptr; qbkt != eptr; ++qbkt) {
      qbkt->prev = nullptr;
    }
    auto bold = std::exchange(this->m_stor.bptr, bptr);
    this->m_stor.eptr = eptr;
    auto next = std::exchange(this->m_stor.head, nullptr);
    // Move buckets into the new table.
    // Warning: No exception shall be thrown from the code below.
    for(;;) {
      auto qbkt = next;
      if(ROCKET_UNEXPECT(!qbkt)) {
        break;
      }
      next = qbkt->next;
      // Move the old name and reference out, then destroy the bucket.
      ROCKET_ASSERT(*qbkt);
      auto name = rocket::move(qbkt->kstor[0]);
      rocket::destroy_at(qbkt->kstor);
      auto refr = rocket::move(qbkt->vstor[0]);
      rocket::destroy_at(qbkt->vstor);
      qbkt->prev = nullptr;
      // Find a new bucket for the name using linear probing.
      // Uniqueness has already been implied for all elements, so there is no need to check for collisions.
      auto mptr = rocket::get_probing_origin(bptr, eptr, name.rdhash());
      qbkt = rocket::linear_probe(bptr, mptr, mptr, eptr, [&](const Bucket&) { return false;  });
      ROCKET_ASSERT(qbkt);
      // Insert the reference into the new bucket.
      ROCKET_ASSERT(!*qbkt);
      this->do_list_attach(qbkt);
      rocket::construct_at(qbkt->kstor, rocket::move(name));
      rocket::construct_at(qbkt->vstor, rocket::move(refr));
    }
    // Deallocate the old table.
    if(bold) {
      ::operator delete(bold);
    }
  }

void Reference_Dictionary::do_attach(Reference_Dictionary::Bucket* qbkt, const PreHashed_String& name) noexcept
  {
    // Construct the node, then attach it.
    ROCKET_ASSERT(!*qbkt);
    this->do_list_attach(qbkt);
    rocket::construct_at(qbkt->kstor, name);
    rocket::construct_at(qbkt->vstor);
    ROCKET_ASSERT(*qbkt);
    this->m_stor.size++;
  }

void Reference_Dictionary::do_detach(Reference_Dictionary::Bucket* qbkt) noexcept
  {
    // Destroy the old name and reference, then detach the bucket.
    this->m_stor.size--;
    ROCKET_ASSERT(*qbkt);
    rocket::destroy_at(qbkt->kstor);
    rocket::destroy_at(qbkt->vstor);
    this->do_list_detach(qbkt);
    ROCKET_ASSERT(!*qbkt);
    // Relocate nodes that follow `qbkt`, if any.
    this->do_xrelocate_but(qbkt);
  }

void Reference_Dictionary::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    auto next = this->m_stor.head;
    for(;;) {
      auto qbkt = next;
      if(ROCKET_UNEXPECT(!qbkt)) {
        break;
      }
      next = qbkt->next;
      // Enumerate child variables.
      ROCKET_ASSERT(*qbkt);
      qbkt->vstor[0].enumerate_variables(callback);
    }
  }

}  // namespace Asteria
