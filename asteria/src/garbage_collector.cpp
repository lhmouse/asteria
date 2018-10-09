// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "garbage_collector.hpp"
#include "variable.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Garbage_collector::~Garbage_collector()
  {
  }

bool Garbage_collector::track_variable(const Rcptr<Variable> &var)
  {
    ROCKET_ASSERT(var);
    const auto range = std::equal_range(this->m_vars.begin(), this->m_vars.end(), var);
    if(range.first != range.second) {
      // The variable already exists.
      return false;
    }
    if((this->m_vars.size() > 127) && (this->m_vars.size() == this->m_vars.capacity())) {
      ASTERIA_DEBUG_LOG("Performing automatic garbage collection: variable_count = ", this->m_vars.size());
      this->collect();
    }
    this->m_vars.insert(range.second, var);
    return true;
  }

bool Garbage_collector::untrack_variable(const Rcptr<Variable> &var) noexcept
  {
    ROCKET_ASSERT(this->m_vars.unique());
    const auto range = std::equal_range(this->m_vars.begin(), this->m_vars.end(), var);
    if(range.first == range.second) {
      // The variable does not exist.
      return false;
    }
    this->m_vars.erase(range.first);
    return true;
  }

namespace {

  // Algorithm:
  //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

  using Gcref_vector = Bivector<Rcptr<Variable>, long>;

  struct Gcref_comparator
    {
      bool operator()(const Gcref_vector::value_type &lhs, const Gcref_vector::value_type &rhs) const noexcept
        {
          return lhs.first < rhs.first;
        }
      bool operator()(const Gcref_vector::value_type &lhs, const Rcptr<Variable> &rhs) const noexcept
        {
          return lhs.first < rhs;
        }
      bool operator()(const Rcptr<Variable> &lhs, const Gcref_vector::value_type &rhs) const noexcept
        {
          return lhs < rhs.first;
        }
    };

  bool do_insert_gcref(void *param, const Rcptr<Variable> &var)
    {
      auto &gcrefs = *(static_cast<Gcref_vector *>(param));
      const auto range = std::equal_range(gcrefs.mut_begin(), gcrefs.mut_end(), var, Gcref_comparator());
      if(range.first != range.second) {
        // The variable has already been recorded.
        // Don't descend into it.
        return false;
      }
      gcrefs.insert(range.second, std::make_pair(var, 0));
      // Descend into it.
      return true;
    }

  bool do_decrement_gcref(void *param, const Rcptr<Variable> &var)
    {
      auto &gcrefs = *(static_cast<Gcref_vector *>(param));
      const auto range = std::equal_range(gcrefs.mut_begin(), gcrefs.mut_end(), var, Gcref_comparator());
      if(range.first != range.second) {
        --(range.first->second);
      }
      // This callback is not recursive.
      return false;
    }

}

void Garbage_collector::collect()
  {
    ASTERIA_DEBUG_LOG("GC begins: variable_count = ", this->m_vars.size());
    Gcref_vector gcrefs;
    gcrefs.reserve(this->m_vars.size() * 4);
    // Find all reachable references.
    for(const auto &root : this->m_vars) {
      // Add `root` itself.
      do_insert_gcref(&gcrefs, root);
      // Add all variables reachable from `root`.
      root->get_value().collect_variables(do_insert_gcref, &gcrefs);
    }
    ASTERIA_DEBUG_LOG("  Number of variables found: ", gcrefs.size());
    // Initialize each gcref counter to the reference count of its variable.
    for(auto it = gcrefs.mut_begin(); it != gcrefs.mut_end(); ++it) {
      it->second = it->first.use_count();
    }
    // Exclude references in `m_vars`.
    for(const auto &root : this->m_vars) {
      do_decrement_gcref(&gcrefs, root);
    }
    // Exclude reference indirectly from `gcrefs`.
    for(const auto &pair : gcrefs) {
      pair.first->get_value().collect_variables(do_decrement_gcref, &gcrefs);
    }
    // Collect each variable whose gcref counter equals one.
    for(const auto &pair : gcrefs) {
      if(pair.second > 1) {
        continue;
      }
      ASTERIA_DEBUG_LOG("  Collecting unreachale variable: ", pair.first->get_value());
      pair.first->reset(D_null(), false);
      this->untrack_variable(pair.first);
    }
    ASTERIA_DEBUG_LOG("GC ends: variable_count = ", this->m_vars.size());
  }

}
