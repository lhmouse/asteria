// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

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
    if(!this->m_tracked.insert(var)) {
      return false;
    }
    if(this->m_counter++ >= this->m_threshold) {
      this->collect();
    }
    return true;
  }

bool Collector::untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    if(!this->m_tracked.erase(var)) {
      return false;
    }
    this->m_counter--;
    return true;
  }

    namespace {

    class Recursion_Sentry
      {
      private:
        long m_old;
        std::reference_wrapper<long> m_ref;

      public:
        explicit Recursion_Sentry(long &ref) noexcept
          : m_old(ref), m_ref(ref)
          {
            this->m_ref++;
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Recursion_Sentry)
          {
            this->m_ref--;
          }

      public:
        explicit operator bool () const noexcept
          {
            return this->m_old == 0;
          }
      };

    template<typename FunctionT>
      class Variable_Callback : public Abstract_Variable_Callback
      {
      private:
        FunctionT m_func;  // If `FunctionT` is a reference type then this is a reference.

      public:
        explicit Variable_Callback(FunctionT &&func)
          : m_func(std::forward<FunctionT>(func))
          {
          }

      public:
        bool operator()(const rocket::refcounted_ptr<Variable> &var) const override
          {
            return this->m_func(var);
          }
      };

    template<typename PointerT, typename FunctionT>
      void do_enumerate_variables(const PointerT &ptr, FunctionT &&func)
      {
        ptr->enumerate_variables(Variable_Callback<FunctionT>(std::forward<FunctionT>(func)));
      }

    template<typename FunctionT>
      void do_enumerate_variables(const Variable_Hashset &set, FunctionT &&func)
      {
        set.for_each(Variable_Callback<FunctionT>(std::forward<FunctionT>(func)));
      }

    }

void Collector::collect()
  {
    // Ignore recursive requests.
    const Recursion_Sentry sentry(this->m_recur);
    if(!sentry) {
      return;
    }
    const auto output = this->m_output_opt;
    const auto tied = this->m_tied_opt;
    bool collect_tied = false;
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
    do_enumerate_variables(this->m_tracked,
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          // Add a variable reachable directly. Do not include references from `m_tracked`.
          root->init_gcref(1);
          if(!this->m_staging.insert(root)) {
            return false;
          }
          // Add variables reachable indirectly.
          do_enumerate_variables(root,
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(!this->m_staging.insert(var)) {
                  return false;
                }
                var->init_gcref(0);
                return true;
              }
            );
          return false;
        }
      );
    ASTERIA_DEBUG_LOG("  Number of variables gathered in total: ", this->m_staging.size());
    ///////////////////////////////////////////////////////////////////////////
    // Phase 2
    //   Drop references directly or indirectly from `m_staging`.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          // Drop a direct reference.
          root->add_gcref(1);
          ROCKET_ASSERT(root->get_gcref() <= root->use_count());
          // Drop indirect references.
          const auto value_nref = root->get_value().use_count();
          switch(value_nref) {
          case 0:
            {
              break;
            }
          case 1:
            {
              // `root->get_value()` is unique.
              do_enumerate_variables(root,
                [&](const rocket::refcounted_ptr<Variable> &var)
                  {
                    var->add_gcref(1);
                    ROCKET_ASSERT(var->get_gcref() <= var->use_count());
                    // This is not going to be recursive.
                    return false;
                  }
                );
              break;
            }
          default:
            {
              // `root->get_value()` is shared.
              do_enumerate_variables(root,
                [&](const rocket::refcounted_ptr<Variable> &var)
                  {
                    var->add_gcref(1 / static_cast<double>(value_nref));
                    ROCKET_ASSERT(var->get_gcref() <= var->use_count());
                    // This is not going to be recursive.
                    return false;
                  }
                );
              break;
            }
          }
          return false;
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Mark variables reachable indirectly from ones reachable directly.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(root->get_gcref() >= root->use_count()) {
            // This variable is possibly unreachable.
            return false;
          }
          // Mark this variable reachable so it will not be collected.
          root->init_gcref(-1);
          // Mark variables reachable indirectly.
          do_enumerate_variables(root,
            [&](const rocket::refcounted_ptr<Variable> &var)
              {
                if(var->get_gcref() < 0) {
                  // This variable has already been marked.
                  return false;
                }
                var->init_gcref(-1);
                // Mark all children recursively.
                return true;
              }
            );
          return false;
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Wipe out variables whose `gcref` counters have excceeded their
    //   reference counts.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const rocket::refcounted_ptr<Variable> &root)
        {
          if(root->get_gcref() >= root->use_count()) {
            ASTERIA_DEBUG_LOG("  Collecting unreachable variable: ", root->get_value());
            root->reset(D_null(), true);
            if(output) {
              output->insert(root);
            }
            this->m_tracked.erase(root);
            return false;
          }
          if(!tied) {
            ASTERIA_DEBUG_LOG("  Keeping reachable variable: ", root->get_value());
            return false;
          }
          ASTERIA_DEBUG_LOG("  Transferring variable to the next generation: ", root->get_value());
          // Strong exception safety is paramount here.
          tied->m_tracked.insert(root);
          collect_tied |= tied->m_counter++ >= tied->m_threshold;
          this->m_tracked.erase(root);
          return false;
        }
      );
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.clear();
    ASTERIA_DEBUG_LOG("Garbage collection ends: this = ", static_cast<void *>(this));
    if(collect_tied) {
      // N.B. This is subject to exceptions.
      tied->collect();
    }
    this->m_counter = 0;
  }

}
