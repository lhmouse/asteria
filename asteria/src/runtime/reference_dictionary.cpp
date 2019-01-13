// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_dictionary.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_dictionary::~Reference_dictionary()
  {
  }

const Reference * Reference_dictionary::do_get_template_opt(const rocket::prehashed_string &name) const noexcept
  {
    // Get template table range.
    auto bptr = this->m_templ_data;
    auto eptr = bptr + this->m_templ_size;
    while(bptr != eptr) {
      // This is a handwritten binary search, utilizing 3-way comparison result of strings.
      const auto mptr = bptr + (eptr - bptr) / 2;
      const auto cmp = name.rdstr().compare(mptr->name);
      if(cmp < 0) {
        eptr = mptr;
        continue;
      }
      if(cmp > 0) {
        bptr = mptr + 1;
        continue;
      }
      // Found.
      return &(mptr->ref);
    }
    // Not found.
    return nullptr;
  }

const Reference * Reference_dictionary::do_get_dynamic_opt(const rocket::prehashed_string &name) const noexcept
  {
    if(this->m_stor.empty()) {
      return nullptr;
    }
    // Get table bounds.
    const auto pre = this->m_stor.data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    const auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.name == name; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return nullptr;
    }
    return bkt->refv;
  }

void Reference_dictionary::do_clear() noexcept
  {
    ROCKET_ASSERT(this->m_stor.size() >= 2);
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Clear all buckets.
    for(auto bkt = pre->next; bkt != end; bkt = bkt->next) {
      bkt->name.clear();
      rocket::destroy_at(bkt->refv);
    }
    // Clear the table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
  }

void Reference_dictionary::do_rehash(std::size_t res_arg)
  {
    ROCKET_ASSERT(res_arg >= this->m_stor.size());
    // Allocate a new vector.
    rocket::cow_vector<Bucket> stor;
    stor.resize(res_arg | 2);
    this->m_stor.swap(stor);
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Clear the new table.
    pre->next = end;
    end->prev = pre;
    // Move elements into the new table.
    if(ROCKET_EXPECT(stor.empty())) {
      return;
    }
    // Drop the last reserved bucket.
    stor.pop_back();
    // In reality the first reserved bucket need not be moved, either.
    // But `.empty()` is faster than `.size()`...
    while(!stor.empty()) {
      auto &rbkt = stor.mut_back();
      if(ROCKET_UNEXPECT(rbkt)) {
        // Find a bucket for the new element.
        const auto origin = rocket::get_probing_origin(pre + 1, end, rbkt.name.rdhash());
        const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false; });
        ROCKET_ASSERT(bkt);
        // Insert it into the new bucket.
        ROCKET_ASSERT(!*bkt);
        bkt->name.swap(rbkt.name);
        rocket::construct_at(bkt->refv, std::move(rbkt.refv[0]));
        rocket::destroy_at(rbkt.refv);
        auto prev = end->prev;
        auto next = end;
        bkt->prev = prev;
        prev->next = bkt;
        bkt->next = next;
        next->prev = bkt;
        // Update the number of elements.
        pre->size++;
      }
      stor.pop_back();
    }
  }

void Reference_dictionary::do_check_relocation(Bucket *to, Bucket *from)
  {
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Check them.
    rocket::linear_probe(
      // Only probe non-erased buckets.
      pre + 1, to, from, end,
      // Relocate every bucket found.
      [&](Bucket &rbkt)
        {
          rocket::prehashed_string name;
          // Release the old element.
          auto prev = rbkt.prev;
          auto next = rbkt.next;
          prev->next = next;
          next->prev = prev;
          name.swap(rbkt.name);
          // Find a new bucket for it using linear probing.
          const auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
          const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false; });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->name = std::move(name);
          if(bkt != &rbkt) {
            rocket::construct_at(bkt->refv, std::move(rbkt.refv[0]));
            rocket::destroy_at(rbkt.refv);
          }
          prev = end->prev;
          next = end;
          bkt->prev = prev;
          prev->next = bkt;
          bkt->next = next;
          next->prev = bkt;
          return false;
        }
      );
  }

Reference & Reference_dictionary::open(const rocket::prehashed_string &name)
  {
    if(name.empty()) {
      ASTERIA_THROW_RUNTIME_ERROR("Empty names are not allowed in a `Reference_dictionary`.");
    }
    if(ROCKET_UNEXPECT(this->size() >= this->m_stor.size() / 2)) {
      this->do_rehash(this->m_stor.size() * 2 | 19);
    }
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find a bucket for the new element.
    const auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.name == name; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(*bkt) {
      // A duplicate key has been found.
      return bkt->refv[0];
    }
    // Insert it into the new bucket.
    bkt->name = name;
    const auto templ = this->do_get_template_opt(name);
    if(ROCKET_UNEXPECT(templ)) {
      static_assert(std::is_nothrow_copy_constructible<Reference>::value, "??");
      rocket::construct_at(bkt->refv, *templ);
    } else {
      // Default-construct a null reference.
      rocket::construct_at(bkt->refv);
    }
    auto prev = end->prev;
    auto next = end;
    bkt->prev = prev;
    prev->next = bkt;
    bkt->next = next;
    next->prev = bkt;
    // Update the number of elements.
    pre->size++;
    return bkt->refv[0];
  }

bool Reference_dictionary::unset(const rocket::prehashed_string &name) noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    const auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.name == name; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      return false;
    }
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    auto prev = bkt->prev;
    auto next = bkt->next;
    prev->next = next;
    next->prev = prev;
    bkt->name.clear();
    rocket::destroy_at(bkt->refv);
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return true;
  }

}
