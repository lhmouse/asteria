// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Collector::~Collector()
  {
  }

bool Collector::track_variable(const rocket::refcounted_ptr<Variable> &var)
  {
    ROCKET_ASSERT(var);
    if(this->m_vars.size() == this->m_vars.capacity()) {
      ASTERIA_DEBUG_LOG("Performing automatic garbage collection: variable_count = ", this->m_vars.size());
      this->collect(false);
      this->m_vars.reserve(this->m_vars.size() + 127);
    }
    const auto range = std::equal_range(this->m_vars.begin(), this->m_vars.end(), var);
    if(range.first != range.second) {
      // The variable already exists.
      return false;
    }
    this->m_vars.insert(range.second, var);
    return true;
  }

bool Collector::untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept
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

  struct Gcref_comparator
    {
      bool operator()(const std::pair<rocket::refcounted_ptr<Variable>, long> &lhs, const std::pair<rocket::refcounted_ptr<Variable>, long> &rhs) const noexcept
        {
          return std::less<rocket::refcounted_ptr<Variable>>()(lhs.first, rhs.first);
        }
      bool operator()(const std::pair<rocket::refcounted_ptr<Variable>, long> &lhs, const rocket::refcounted_ptr<Variable> &rhs) const noexcept
        {
          return std::less<rocket::refcounted_ptr<Variable>>()(lhs.first, rhs);
        }
      bool operator()(const rocket::refcounted_ptr<Variable> &lhs, const std::pair<rocket::refcounted_ptr<Variable>, long> &rhs) const noexcept
        {
          return std::less<rocket::refcounted_ptr<Variable>>()(lhs, rhs.first);
        }
    };

  }

void Collector::collect(bool unreserve)
  {
    // https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
    ASTERIA_DEBUG_LOG("Garbage collection begins.");
    // Add variables that are either tracked or reachable indirectly from tracked ones.
    const auto gather_gcref =
      [](void *param, const rocket::refcounted_ptr<Variable> &var)
        {
          const auto self = static_cast<Collector *>(param);
          const auto range = std::equal_range(self->m_gcrefs.mut_begin(), self->m_gcrefs.mut_end(), var, Gcref_comparator());
          if(range.first != range.second) {
            return false;
          }
          self->m_gcrefs.insert(range.second, std::make_pair(var, 0));
          return true;
        };
    for(const auto &root : this->m_vars) {
      gather_gcref(this, root);
      root->get_value().collect_variables(gather_gcref, this);
    }
    ASTERIA_DEBUG_LOG("  Number of variables gathered in total: ", this->m_gcrefs.size());
    // Initialize each gcref counter to the reference count of its variable.
    for(auto it = this->m_gcrefs.mut_begin(); it != this->m_gcrefs.mut_end(); ++it) {
      it->second = it->first.use_count();
    }
    // Drop references from `m_vars` or `m_gcrefs`, either directly or indirectly.
    const auto decrement_gcref =
      [](void *param, const rocket::refcounted_ptr<Variable> &var)
        {
          const auto self = static_cast<Collector *>(param);
          const auto range = std::equal_range(self->m_gcrefs.mut_begin(), self->m_gcrefs.mut_end(), var, Gcref_comparator());
          if(range.first != range.second) {
            --(range.first->second);
          }
          return false;
        };
    for(const auto &root : this->m_vars) {
      decrement_gcref(this, root);
    }
    for(const auto &pair : this->m_gcrefs) {
      decrement_gcref(this, pair.first);
      pair.first->get_value().collect_variables(decrement_gcref, this);
    }
    // Collect each variable whose gcref counter has reached zero.
    while(!this->m_gcrefs.empty()) {
      const auto &pair = this->m_gcrefs.back();
      if(pair.second <= 0) {
        ASTERIA_DEBUG_LOG("  Collecting unreachable variable: ", pair.first->get_value());
        pair.first->reset(D_null(), false);
        this->untrack_variable(pair.first);
      }
      this->m_gcrefs.pop_back();
    }
    ASTERIA_DEBUG_LOG("  Number of variables uncollected in total: ", this->m_vars.size());
    // Transfer surviving variables to the tied collector, if any.
    const auto tied = this->m_tied_opt;
    if(tied) {
      // Take care about exception safety.
      while(!this->m_vars.empty()) {
        tied->track_variable(this->m_vars.back());
        this->m_vars.pop_back();
      }
    }
    // Dispose reserved storage if requested.
    if(unreserve) {
      this->m_vars.shrink_to_fit();
      this->m_gcrefs.shrink_to_fit();
    }
    ASTERIA_DEBUG_LOG("Garbage collection ends.");
  }

}
