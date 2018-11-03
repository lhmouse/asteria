// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "abstract_variable_callback.hpp"
#include "utilities.hpp"

namespace Asteria {

Collector::~Collector()
  {
  }

bool Collector::track_variable(const rocket::refcounted_ptr<Variable> &var)
  {
    if(this->m_tracked.empty()) {
      this->m_tracked.reserve(this->m_threshold | 20);
    }
    if(!this->m_tracked.insert(var)) {
      return false;
    }
    this->m_counter += 1;
    this->auto_collect();
    return true;
  }

bool Collector::untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    if(!this->m_tracked.erase(var)) {
      return false;
    }
    return true;
  }

bool Collector::auto_collect()
  {
    if(this->m_counter < this->m_threshold) {
      return false;
    }
    this->collect();
    return true;
  }

    namespace {

    class Sentry
      {
      private:
        long m_old;
        std::reference_wrapper<long> m_ref;

      public:
        explicit Sentry(long &ref) noexcept
          : m_old(ref), m_ref(ref)
          {
            this->m_ref += 1;
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Sentry)
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

    template<typename FunctionT>
      inline Variable_callback<FunctionT> do_make_variable_callback(FunctionT &&func)
        {
          return Variable_callback<FunctionT>(std::forward<FunctionT>(func));
        }

    }

void Collector::collect()
  {
    // Ignore recursive requests.
    const Sentry sentry(this->m_recur);
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
          root->set_gcref(1);
          if(!this->m_staging.insert(root)) {
            return;
          }
          // Add variables reachable indirectly.
          root->enumerate_variables(do_make_variable_callback(
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(!this->m_staging.insert(var)) {
                  return false;
                }
                var->set_gcref(0);
                return true;
              }
            ));
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
          root->set_gcref(root->get_gcref() + 1);
          // Drop indirect references. This is not going to be recursive.
          root->enumerate_variables(do_make_variable_callback(
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                var->set_gcref(var->get_gcref() + 1);
                return false;
              }
            ));
        }
      );
#ifdef ROCKET_DEBUG
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          ROCKET_ASSERT(root->get_gcref() >= 0);
          ROCKET_ASSERT(root->get_gcref() <= root->use_count());
        }
      );
#endif
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Mark variables indirectly reachable from directly reachable ones.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(root->get_gcref() >= root->use_count()) {
            return;
          }
          // Mark a variable that is reachable and will not be collected.
          root->set_gcref(-1);
          // Mark variables reachable indirectly.
          root->enumerate_variables(do_make_variable_callback(
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(var->get_gcref() < 0) {
                  return false;
                }
                var->set_gcref(-1);
                return true;
              }
            ));
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Wipe out variables whose `gcref` counters have excceeded their
    //   reference counts.
    ///////////////////////////////////////////////////////////////////////////
    bool collect_next = false;
    const auto tied = this->m_tied_opt;
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(root->get_gcref() >= root->use_count()) {
            ASTERIA_DEBUG_LOG("  Collecting unreachable variable: ", root->get_value());
            root->reset(D_null(), true);
            this->m_tracked.erase(root);
            return;
          }
          if(tied) {
            ASTERIA_DEBUG_LOG("  Transferring variable to the next generation: ", root->get_value());
            tied->m_tracked.insert(root);
            ++(tied->m_counter);
            collect_next |= tied->m_counter > tied->m_threshold;
            this->m_tracked.erase(root);
            return;
          }
        }
      );
    if(collect_next) {
      tied->auto_collect();
    }
    ASTERIA_DEBUG_LOG("Garbage collection ends: this = ", static_cast<void *>(this));
    this->m_counter = 0;
    this->m_staging.clear();
  }

}
