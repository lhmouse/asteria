// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_dictionary.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_Dictionary::Bucket::~Bucket()
  {
    // Be careful, VERY careful.
    if(ROCKET_UNEXPECT(*this)) {
      rocket::destroy_at(this->second);
    }
  }

Reference_Dictionary::Bucket::operator bool () const noexcept
  {
    return this->first.empty() == false;
  }

void Reference_Dictionary::Bucket::do_attach(Reference_Dictionary::Bucket* ipos) noexcept
  {
    auto iprev = ipos->prev;
    auto inext = ipos;
    // Set up pointers.
    this->prev = iprev;
    prev->next = this;
    this->next = inext;
    next->prev = this;
  }

void Reference_Dictionary::Bucket::do_detach() noexcept
  {
    auto iprev = this->prev;
    auto inext = this->next;
    // Set up pointers.
    prev->next = inext;
    next->prev = iprev;
  }

void Reference_Dictionary::do_clear() noexcept
  {
    ROCKET_ASSERT(this->m_stor.size() >= 2);
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Clear all buckets.
    for(auto bkt = pre->next; bkt != end; bkt = bkt->next) {
      ROCKET_ASSERT(*bkt);
      bkt->first.clear();
      rocket::destroy_at(bkt->second);
    }
    // Clear the table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
  }

void Reference_Dictionary::do_rehash(std::size_t res_arg)
  {
    ROCKET_ASSERT(res_arg >= this->m_stor.size());
    // Allocate a new vector.
    Cow_Vector<Bucket> stor;
    stor.resize(res_arg | 3);
    this->m_stor.swap(stor);
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Clear the new table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
    // Move elements into the new table, if any.
    if(stor.empty()) {
      return;
    }
    std::for_each(
      // Get the range of valid buckets.
      stor.mut_begin() + 1, stor.mut_end() - 1,
      // Move each bucket in the range.
      [&](Bucket& rbkt)
        {
          if(ROCKET_EXPECT(!rbkt)) {
            return;
          }
          // Find a bucket for the new element.
          auto origin = rocket::get_probing_origin(pre + 1, end, rbkt.first.rdhash());
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket&) { return false;  });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->first.swap(rbkt.first);
          rocket::construct_at(bkt->second, rocket::move(rbkt.second[0]));
          rocket::destroy_at(rbkt.second);
          bkt->do_attach(end);
          // Update the number of elements.
          pre->size++;
        }
      );
  }

void Reference_Dictionary::do_check_relocation(Bucket* to, Bucket* from)
  {
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Check them.
    rocket::linear_probe(
      // Only probe non-erased buckets.
      pre + 1, to, from, end,
      // Relocate every bucket found.
      [&](Bucket& rbkt)
        {
          PreHashed_String name;
          // Release the old element.
          rbkt.do_detach();
          name.swap(rbkt.first);
          // Find a new bucket for it using linear probing.
          auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket&) { return false;  });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->first = rocket::move(name);
          if(bkt != &rbkt) {
            rocket::construct_at(bkt->second, rocket::move(rbkt.second[0]));
            rocket::destroy_at(rbkt.second);
          }
          bkt->do_attach(end);
          return false;
        }
      );
  }

const Reference* Reference_Dictionary::get_opt(const PreHashed_String& name) const noexcept
  {
    if(ROCKET_UNEXPECT(name.empty())) {
      return nullptr;
    }
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
      return nullptr;
    }
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == name;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return nullptr;
    }
    return bkt->second;
  }

Reference& Reference_Dictionary::open(const PreHashed_String& name)
  {
    if(ROCKET_UNEXPECT(name.empty())) {
      ASTERIA_THROW_RUNTIME_ERROR("Empty names are not allowed in a `Reference_Dictionary`.");
    }
    if(ROCKET_UNEXPECT(this->size() >= this->m_stor.size() / 2)) {
      this->do_rehash(this->m_stor.size() * 2 | 11);
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find a bucket for the new element.
    auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == name;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(*bkt) {
      // A duplicate key has been found.
      return bkt->second[0];
    }
    // Insert it into the new bucket.
    bkt->first = name;
    rocket::construct_at(bkt->second);
    bkt->do_attach(end);
    // Update the number of elements.
    pre->size++;
    return bkt->second[0];
  }

bool Reference_Dictionary::remove(const PreHashed_String& name) noexcept
  {
    if(ROCKET_UNEXPECT(name.empty())) {
      return false;
    }
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == name;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      return false;
    }
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    bkt->do_detach();
    bkt->first.clear();
    rocket::destroy_at(bkt->second);
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return true;
  }

}  // namespace Asteria
