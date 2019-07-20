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

void Variable_HashSet::Bucket::do_attach(Variable_HashSet::Bucket* ipos) noexcept
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
          auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(rbkt.first.get()));
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket&) { return false;  });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->first.swap(rbkt.first);
          bkt->do_attach(end);
          // Update the number of elements.
          pre->size++;
        }
      );
  }

void Variable_HashSet::do_check_relocation(Bucket* to, Bucket* from)
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
          Rcptr<Variable> var;
          // Release the old element.
          rbkt.do_detach();
          var.swap(rbkt.first);
          // Find a new bucket for it using linear probing.
          auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
          auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket&) { return false;  });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->first = rocket::move(var);
          bkt->do_attach(end);
          return false;
        }
      );
  }

bool Variable_HashSet::has(const Rcptr<Variable>& var) const noexcept
  {
    if(ROCKET_UNEXPECT(!var)) {
      return false;
    }
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == var;  });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return false;
    }
    return true;
  }

void Variable_HashSet::enumerate(const Abstract_Variable_Callback& callback) const
  {
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
      return;
    }
    // Get table bounds.
    auto pre = this->m_stor.data();
    auto end = pre + (this->m_stor.size() - 1);
    // Enumerate all buckets. The return value of `callback(bkt->first)` is ignored.
    for(auto bkt = pre->next; bkt != end; bkt = bkt->next) {
      ROCKET_ASSERT(*bkt);
      if(!callback(bkt->first)) {
        continue;
      }
      // Descend into this variable recursively when the callback returns `true`.
      bkt->first->enumerate_variables(callback);
    }
  }

bool Variable_HashSet::insert(const Rcptr<Variable>& var)
  {
    if(ROCKET_UNEXPECT(!var)) {
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
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == var;  });
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

bool Variable_HashSet::erase(const Rcptr<Variable>& var) noexcept
  {
    if(ROCKET_UNEXPECT(!var)) {
      return false;
    }
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
      return false;
    }
    // Get table bounds.
    auto pre = this->m_stor.mut_data();
    auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket& rbkt) { return rbkt.first == var;  });
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

Rcptr<Variable> Variable_HashSet::erase_random_opt() noexcept
  {
    if(ROCKET_UNEXPECT(this->m_stor.empty())) {
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
    Rcptr<Variable> var;
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    bkt->do_detach();
    var.swap(bkt->first);
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return rocket::move(var);
  }

}  // namespace Asteria
