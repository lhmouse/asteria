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

void Reference_Dictionary::Bucket::do_attach(Reference_Dictionary::Bucket *ipos) noexcept
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

const Reference * Reference_Dictionary::do_get_template_nonempty_opt(const PreHashed_String &name) const noexcept
  {
    ROCKET_ASSERT(this->m_templ_size != 0);
#ifdef ROCKET_DEBUG
    // The table must have been sorted.
    ROCKET_ASSERT(std::is_sorted(this->m_templ_data, this->m_templ_data + this->m_templ_size,
                                 [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first;  }));
#endif
    // Get template table range.
    auto base = this->m_templ_data;
    std::size_t lower = 0;
    std::size_t upper = this->m_templ_size;
    for(;;) {
      // This is a handwritten binary search, utilizing 3-way comparison results of strings.
      std::size_t middle = (lower + upper) / 2;
      const auto &pivot = base[middle];
      int cmp = name.rdstr().compare(pivot.first);
      if(ROCKET_EXPECT(cmp == 0)) {
        // Found.
        return std::addressof(pivot.second);
      }
      if(cmp > 0) {
        lower = middle + 1;
      } else {
        upper = middle;
      }
      if(ROCKET_UNEXPECT(lower == upper)) {
        // Not found.
        return nullptr;
      }
    }
  }

const Reference * Reference_Dictionary::do_get_dynamic_nonempty_opt(const PreHashed_String &name) const noexcept
  {
    ROCKET_ASSERT(!this->m_stor.empty());
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == name;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return nullptr;
    }
    return bkt->second;
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
    stor.resize(res_arg | 2);
    this->m_stor.swap(stor);
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Clear the new table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
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
        auto origin = rocket::get_probing_origin(pre + 1, end, rbkt.first.rdhash());
        auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false;  });
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
      stor.pop_back();
    }
  }

void Reference_Dictionary::do_check_relocation(Bucket *to, Bucket *from)
  {
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Check them.
    rocket::linear_probe(
      // Only probe non-erased buckets.
      pre + 1, to, from, end,
      // Relocate every bucket found.
      [&](Bucket &rbkt)
        {
          PreHashed_String name;
          // Release the old element.
          rbkt.do_detach();
          name.swap(rbkt.first);
          // Find a new bucket for it using linear probing.
          auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false;  });
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

Reference & Reference_Dictionary::open(const PreHashed_String &name)
  {
    if(name.empty()) {
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
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == name;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(*bkt) {
      // A duplicate key has been found.
      return bkt->second[0];
    }
    // Insert it into the new bucket.
    bkt->first = name;
    // Find the template to copy from.
    auto templ = this->do_get_template_opt(name);
    if(ROCKET_UNEXPECT(templ)) {
       // Copy the static template.
      static_assert(std::is_nothrow_copy_assignable<Reference>::value, "??");
      rocket::construct_at(bkt->second, *templ);
    } else {
      // Construct a null reference.
      static_assert(std::is_nothrow_constructible<Reference>::value, "??");
      rocket::construct_at(bkt->second);
    }
    bkt->do_attach(end);
    // Update the number of elements.
    pre->size++;
    return bkt->second[0];
  }

bool Reference_Dictionary::remove(const PreHashed_String &name) noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, name.rdhash());
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == name;  });
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
