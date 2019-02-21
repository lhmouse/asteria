// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable_hashset.hpp"
#include "abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variable_HashSet::Bucket::~Bucket()
  {
  }

Variable_HashSet::Bucket::operator bool () const noexcept
  {
    return this->first != nullptr;
  }

void Variable_HashSet::Bucket::do_attach(Variable_HashSet::Bucket *ipos) noexcept
  {
    auto iprev = ipos->prev;
    auto inext = ipos;
    // Set up pointers.
    this->prev = iprev;
    prev->next = this;
    this->next = inext;
    next->prev = this;
  }

void Variable_HashSet::Bucket::do_detach() noexcept
  {
    auto iprev = this->prev;
    auto inext = this->next;
    // Set up pointers.
    prev->next = inext;
    next->prev = iprev;
  }

void Variable_HashSet::do_clear() noexcept
  {
    ROCKET_ASSERT(this->m_stor.size() >= 2);
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Clear all buckets.
    for(auto bkt = pre->next; bkt != end; bkt = bkt->next) {
      ROCKET_ASSERT(*bkt);
      bkt->first.reset();
    }
    // Clear the table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
  }

void Variable_HashSet::do_rehash(std::size_t res_arg)
  {
    ROCKET_ASSERT(res_arg >= this->m_stor.size());
    // Allocate a new vector.
    CoW_Vector<Bucket> stor;
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
        auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(rbkt.first.get()));
        auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false;  });
        ROCKET_ASSERT(bkt);
        // Insert it into the new bucket.
        ROCKET_ASSERT(!*bkt);
        bkt->first.swap(rbkt.first);
        bkt->do_attach(end);
        // Update the number of elements.
        pre->size++;
      }
      stor.pop_back();
    }
  }

void Variable_HashSet::do_check_relocation(Bucket *to, Bucket *from)
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
          RefCnt_Ptr<Variable> var;
          // Release the old element.
          rbkt.do_detach();
          var.swap(rbkt.first);
          // Find a new bucket for it using linear probing.
          auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false;  });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->first = std::move(var);
          bkt->do_attach(end);
          return false;
        }
      );
  }

bool Variable_HashSet::has(const RefCnt_Ptr<Variable> &var) const noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == var;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return false;
    }
    return true;
  }

void Variable_HashSet::for_each(const Abstract_Variable_Callback &callback) const
  {
    if(this->m_stor.empty()) {
      return;
    }
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Enumerate all buckets. The return value of `callback(bkt->first)` is ignored.
    for(auto bkt = pre->next; bkt != end; bkt = bkt->next) {
      ROCKET_ASSERT(*bkt);
      callback(bkt->first);
    }
  }

bool Variable_HashSet::insert(const RefCnt_Ptr<Variable> &var)
  {
    if(!var) {
      ASTERIA_THROW_RUNTIME_ERROR("Null variable pointers are not allowed in a `Variable_HashSet`.");
    }
    if(ROCKET_UNEXPECT(this->size() >= this->m_stor.size() / 2)) {
      this->do_rehash(this->m_stor.size() * 2 | 97);
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find a bucket for the new element.
    auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == var;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(*bkt) {
      // A duplicate key has been found.
      return false;
    }
    // Insert it into the new bucket.
    bkt->first = var;
    bkt->do_attach(end);
    // Update the number of elements.
    pre->size++;
    return true;
  }

bool Variable_HashSet::erase(const RefCnt_Ptr<Variable> &var) noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.first == var;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      return false;
    }
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    bkt->do_detach();
    bkt->first.reset();
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return true;
  }

RefCnt_Ptr<Variable> Variable_HashSet::erase_random_opt() noexcept
  {
    if(this->m_stor.empty()) {
      return nullptr;
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Get the first non-empty bucket.
    auto bkt = pre->next;
    if(bkt == end) {
      return nullptr;
    }
    ROCKET_ASSERT(*bkt);
    RefCnt_Ptr<Variable> var;
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    bkt->do_detach();
    var.swap(bkt->first);
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return std::move(var);
  }

}
