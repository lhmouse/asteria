// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable_hashset.hpp"
#include "abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variable_hashset::~Variable_hashset()
  {
  }

void Variable_hashset::do_clear() noexcept
  {
    ROCKET_ASSERT(this->m_stor.size() >= 2);
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Clear all buckets.
    for(auto ptr = pre->next; ptr != end; ptr = ptr->next) {
      ptr->var.reset();
    }
    // Clear the table.
    pre->next = end;
    end->prev = pre;
    // Update the number of elements.
    pre->size = 0;
    end->resv = 0;
  }

void Variable_hashset::do_rehash(std::size_t res_arg)
  {
    ROCKET_ASSERT(res_arg >= 2);
    ROCKET_ASSERT(res_arg >= this->m_stor.size());
    // Allocate a new vector.
    rocket::cow_vector<Bucket> stor;
    stor.reserve(res_arg);
    stor.append(stor.capacity());
    // Get new table bounds.
    const auto pre = stor.mut_data();
    const auto end = pre + (stor.size() - 1);
    // Clear the new table.
    pre->next = end;
    end->prev = pre;
    // Move elements into the new table.
    while(!this->m_stor.empty()) {
      auto &rbkt = this->m_stor.mut_back();
      if(rbkt) {
        // Find a bucket for the new element.
        const auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(rbkt.var.get()));
        const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false; });
        ROCKET_ASSERT(bkt);
        // Insert it into the new bucket.
        ROCKET_ASSERT(!*bkt);
        bkt->var = std::move(rbkt.var);
        bkt->prev = end->prev;
        bkt->next = end;
        end->prev->next = bkt;
        end->prev = bkt;
        // Update the number of elements.
        pre->size++;
      }
      this->m_stor.pop_back();
    }
    // Set up the new table.
    this->m_stor = std::move(stor);
  }

void Variable_hashset::do_check_relocation(Bucket *from, Bucket *to)
  {
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Check them.
    rocket::linear_probe(
      // Only probe non-erased buckets.
      pre + 1, from, to, end,
      // Relocate every bucket found.
      [&](Bucket &rbkt)
        {
          // Release the old element.
          rbkt.prev->next = rbkt.next;
          rbkt.next->prev = rbkt.prev;
          auto var = rocket::exchange(rbkt.var, nullptr);
          // Find a new bucket for it using linear probing.
          const auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
          const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &) { return false; });
          ROCKET_ASSERT(bkt);
          // Insert it into the new bucket.
          ROCKET_ASSERT(!*bkt);
          bkt->var = std::move(var);
          bkt->prev = end->prev;
          bkt->next = end;
          end->prev->next = bkt;
          end->prev = bkt;
          return false;
        }
      );
  }

bool Variable_hashset::has(const rocket::refcounted_ptr<Variable> &var) const noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    const auto pre = this->m_stor.data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    const auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.var == var; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
      return false;
    }
    return true;
  }

void Variable_hashset::for_each(const Abstract_variable_callback &callback) const
  {
    if(this->m_stor.empty()) {
      return;
    }
    // Get table bounds.
    const auto pre = this->m_stor.data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Enumerate all buckets. The return value of `callback(ptr->var)` is ignored.
    for(auto ptr = pre->next; ptr != end; ptr = ptr->next) {
      callback(ptr->var);
    }
  }

bool Variable_hashset::insert(const rocket::refcounted_ptr<Variable> &var)
  {
    if(ROCKET_UNEXPECT(this->size() >= this->m_stor.size() / 2)) {
      this->do_rehash(this->m_stor.size() * 2 | 127);
    }
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find a bucket for the new element.
    const auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.var == var; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(*bkt) {
      // A duplicate key has been found.
      return false;
    }
    // Insert it into the new bucket.
    bkt->var = var;
    bkt->prev = end->prev;
    bkt->next = end;
    end->prev->next = bkt;
    end->prev = bkt;
    // Update the number of elements.
    pre->size++;
    return true;
  }

bool Variable_hashset::erase(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    if(this->m_stor.empty()) {
      return false;
    }
    // Get table bounds.
    const auto pre = this->m_stor.mut_data();
    const auto end = pre + (this->m_stor.size() - 1);
    // Find the element using linear probing.
    const auto origin = rocket::get_probing_origin(pre + 1, end, reinterpret_cast<std::uintptr_t>(var.get()));
    const auto bkt = rocket::linear_probe(pre + 1, origin, origin, end, [&](const Bucket &rbkt) { return rbkt.var == var; });
    // There will always be some empty buckets in the table.
    ROCKET_ASSERT(bkt);
    if(!*bkt) {
      return false;
    }
    // Update the number of elements.
    pre->size--;
    // Empty the bucket.
    bkt->prev->next = bkt->next;
    bkt->next->prev = bkt->prev;
    bkt->var.reset();
    // Relocate elements that are not placed in their immediate locations.
    this->do_check_relocation(bkt, bkt + 1);
    return true;
  }

}
