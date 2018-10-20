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
    if(this->m_counter <= this->m_threshold) {
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
      ~Sentry()
        {
          this->m_ref -= 1;
        }

      Sentry(const Sentry &)
        = delete;
      Sentry & operator=(const Sentry &)
        = delete;

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
    // https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
    ASTERIA_DEBUG_LOG("Garbage collection begins: this = ", static_cast<void *>(this), ", tracked_variables = ", this->m_tracked.size());
    ///////////////////////////////////////////////////////////////////////////
    // Phase 1
    //   Add variables that are either tracked or reachable from tracked ones
    //   into the staging area.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.clear();
    this->m_staging.reserve(this->m_tracked.size() * 9 / 8);
    this->m_tracked.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          // Add those directly reachable.
          this->m_staging.insert(root);
          // Add those indirectly reachable.
          root->enumerate_variables(do_make_variable_callback(
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                return this->m_staging.insert(var);
              }
            ));
        }
      );
    ASTERIA_DEBUG_LOG("  Number of variables gathered in total: ", this->m_staging.size());
    ///////////////////////////////////////////////////////////////////////////
    // Phase 2
    //   Initialize `gcref` of each staged variable to its reference count,
    //   not including the one of `m_staging`.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          root->set_gcref(root.use_count() - 1);
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Drop references directly from `m_tracked`.
    ///////////////////////////////////////////////////////////////////////////
    this->m_tracked.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          root->set_gcref(root->get_gcref() - 1);
          return false;
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Drop references directly or indirectly from `m_staging`.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          root->enumerate_variables(do_make_variable_callback(
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                var->set_gcref(var->get_gcref() - 1);
                return false;
              }
            ));
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Wipe out variables whose `gcref` counters have reached zero.
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.for_each(
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          ROCKET_ASSERT(root->get_gcref() >= 0);
          if(root->get_gcref() > 0) {
            return;
          }
          ASTERIA_DEBUG_LOG("  Collecting unreachable variable: ", root->get_value());
          root->reset(D_null(), true);
          this->m_tracked.erase(root);
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 6
    //   Transfer surviving variables to the tied collector, if any.
    ///////////////////////////////////////////////////////////////////////////
    const auto tied = this->m_tied_opt;
    bool collect_next = false;
    if(tied) {
      this->m_staging.for_each(
        [&](const rocket::refcounted_ptr<Variable> &root)
          {
            tied->m_tracked.insert(root);
            ++(tied->m_counter);
            collect_next |= tied->m_counter > tied->m_threshold;
            this->m_tracked.erase(root);
          }
        );
    }
    this->m_staging.clear();
    if(collect_next) {
      tied->auto_collect();
    }
    ASTERIA_DEBUG_LOG("Garbage collection ends: this = ", static_cast<void *>(this));
    this->m_counter = 0;
  }

}
