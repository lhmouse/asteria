// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

Collector::~Collector()
  {
  }

bool Collector::track_variable(const rocket::refcounted_ptr<Variable> &var)
  {
    if(this->m_tracked.empty()) {
      this->m_tracked.reserve(static_cast<unsigned long>(this->m_threshold | 20));
    }
    if(!this->m_tracked.insert(var)) {
      return false;
    }
    if(++(this->m_counter) > this->m_threshold) {
      this->collect();
    }
    return true;
  }

bool Collector::untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    if(!this->m_tracked.erase(var)) {
      return false;
    }
    --(this->m_counter);
    return true;
  }

    namespace {

    class Recursion_sentry
      {
      private:
        long m_old;
        std::reference_wrapper<long> m_ref;

      public:
        explicit Recursion_sentry(long &ref) noexcept
          : m_old(ref), m_ref(ref)
          {
            this->m_ref += 1;
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Recursion_sentry)
          {
            this->m_ref -= 1;
          }

      public:
        explicit operator bool () const noexcept
          {
            return this->m_old == 0;
          }
      };

    template<typename FunctionT>
      class Variable_callback : public Abstract_variable_callback
      {
      private:
        FunctionT m_func;  // If `FunctionT` is a reference type then this is a reference.

      public:
        explicit Variable_callback(FunctionT &&func)
          : m_func(std::forward<FunctionT>(func))
          {
          }

      public:
        bool accept(const rocket::refcounted_ptr<Variable> &var) const override
          {
            return this->m_func(var);
          }
      };

    template<typename PointerT, typename FunctionT>
      void do_enumerate_variables(const PointerT &ptr, FunctionT &&func)
      {
        ptr->enumerate_variables(Variable_callback<FunctionT>(std::forward<FunctionT>(func)));
      }

    constexpr long do_lcast(double value) noexcept
      {
        return static_cast<long>(value + 1e-9);
      }

    }

void Collector::collect()
  {
    // Ignore recursive requests.
    const Recursion_sentry sentry(this->m_recur);
    if(!sentry) {
      return;
    }
    // The algorithm here is basically described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
    // However, we initialize `gcref` to zero then increment it, rather than initialize `gcref` to the reference count then decrement it.
    // This saves a phase below for us.
    ASTERIA_DEBUG_LOG("Garbage collection begins: this = ", static_cast<void *>(this), ", tracked_variables = ", this->m_tracked.size());
    this->m_staging.clear();
    ///////////////////////////////////////////////////////////////////////////
    // Phase 1
    //   Add variables that are either tracked or reachable from tracked ones
    //   into the staging area.
    ///////////////////////////////////////////////////////////////////////////
    this->m_tracked.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          // Add a variable reachable directly. Do not include references from `m_tracked`.
          root->set_gcref(1.0);
          if(!this->m_staging.insert(root)) {
            return;
          }
          // Add variables reachable indirectly.
          do_enumerate_variables(root,
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(!this->m_staging.insert(var)) {
                  return false;
                }
                var->set_gcref(0.0);
                return true;
              }
            );
        }
      );
    ASTERIA_DEBUG_LOG("  Number of variables gathered in total: ", this->m_staging.size());
    ///////////////////////////////////////////////////////////////////////////
    // Phase 2
    //   Drop references directly or indirectly from `m_staging`.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          // Drop a direct reference.
          root->set_gcref(root->get_gcref() + 1.0);
          ROCKET_ASSERT(do_lcast(root->get_gcref()) <= root->use_count());
          const auto weight = 1.0 / static_cast<double>(rocket::max(root->get_value().use_count(), 1));
          // Drop indirect references.
          do_enumerate_variables(root,
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                var->set_gcref(var->get_gcref() + weight);
                ROCKET_ASSERT(do_lcast(var->get_gcref()) <= var->use_count());
                // This is not going to be recursive.
                return false;
              }
            );
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Mark variables reachable indirectly from ones reachable directly.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(do_lcast(root->get_gcref()) >= root->use_count()) {
            // This variable is possibly unreachable.
            return;
          }
          // Mark a variable that is reachable and will not be collected.
          root->set_gcref(-1.0);
          // Mark variables reachable indirectly.
          do_enumerate_variables(root,
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(var->get_gcref() < 0) {
                  // This variable has already been marked.
                  return false;
                }
                var->set_gcref(-1.0);
                // Mark all children recursively.
                return true;
              }
            );
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Wipe out variables whose `gcref` counters have excceeded their
    //   reference counts.
    ///////////////////////////////////////////////////////////////////////////
    const auto tied = this->m_tied_opt;
    bool collect_tied = false;
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(do_lcast(root->get_gcref()) >= root->use_count()) {
            ASTERIA_DEBUG_LOG("  Collecting unreachable variable: ", root->get_value());
            root->reset(D_null(), true);
            this->m_tracked.erase(root);
            return;
          }
          if(tied) {
            ASTERIA_DEBUG_LOG("  Transferring variable to the next generation: ", root->get_value());
            // Strong exception safety is paramount here.
            tied->m_tracked.insert(root);
            this->m_tracked.erase(root);
            if(++(tied->m_counter) > tied->m_threshold) {
              collect_tied = true;
            }
            return;
          }
        }
      );
    if(collect_tied) {
      tied->collect();
    }
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    ASTERIA_DEBUG_LOG("Garbage collection ends: this = ", static_cast<void *>(this));
    this->m_counter = 0;
    this->m_staging.clear();
  }

}
