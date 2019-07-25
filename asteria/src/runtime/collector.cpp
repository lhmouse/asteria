// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

bool Collector::track_variable(const Rcptr<Variable>& var)
  {
    if(!this->m_tracked.insert(var)) {
      return false;
    }
    if(ROCKET_UNEXPECT(this->m_counter++ >= this->m_threshold)) {
      // Perform automatic garbage collection on `*this`.
      auto qnext = this;
      do {
        qnext = qnext->collect_single_opt();
      } while(qnext);
    }
    return true;
  }

bool Collector::untrack_variable(const Rcptr<Variable>& var) noexcept
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
        explicit Recursion_Sentry(long& ref) noexcept
          : m_old(ref), m_ref(ref)
          {
            this->m_ref++;
          }
        ~Recursion_Sentry()
          {
            this->m_ref--;
          }

        Recursion_Sentry(const Recursion_Sentry&)
          = delete;
        Recursion_Sentry& operator=(const Recursion_Sentry&)
          = delete;

      public:
        explicit operator bool () const noexcept
          {
            return this->m_old == 0;
          }
      };

    template<typename FuncT> class Variable_Callback : public Abstract_Variable_Callback
      {
      private:
        FuncT m_func;  // If `FuncT` is a reference type then this is a reference.

      public:
        explicit Variable_Callback(FuncT&& func)
          : m_func(rocket::forward<FuncT>(func))
          {
          }

      public:
        bool operator()(const Rcptr<Variable>& var) const override
          {
            return this->m_func(var);
          }
      };

    template<typename FuncT> void do_enumerate_variables(Variable_HashSet& set, FuncT&& func)
      {
        set.enumerate(Variable_Callback<FuncT>(rocket::forward<FuncT>(func)));
      }
    template<typename FuncT> void do_enumerate_variables(const Rcptr<Variable>& var, FuncT&& func)
      {
        var->enumerate_variables(Variable_Callback<FuncT>(rocket::forward<FuncT>(func)));
      }

    }  // namespace

Collector* Collector::collect_single_opt()
  {
    // Ignore recursive requests.
    Recursion_Sentry sentry(this->m_recur);
    if(!sentry) {
      return nullptr;
    }
    auto output = this->m_output_opt;
    auto tied = this->m_tied_opt;
    bool collect_tied = false;
    // The algorithm here is basically described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
    // We initialize `gcref` to zero then increment it, rather than initialize `gcref` to the reference count then decrement it.
    // This saves a phase below for us.
    ASTERIA_DEBUG_LOG("Garbage collection begins: this = ", static_cast<void*>(this), ", tracked_variables = ", this->m_tracked.size());
    this->m_staging.clear();
    ///////////////////////////////////////////////////////////////////////////
    // Phase 1
    //   Add variables that are either tracked or reachable from tracked ones
    //   into the staging area.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_tracked,
      [&](const Rcptr<Variable>& root) {
        // Add a variable that is reachable directly.
        // The reference from `m_tracked` should be excluded, so we initialize the gcref counter to 1.
        root->reset_gcref(1);
        // If this variable has been inserted indirectly, finish.
        if(!this->m_staging.insert(root)) {
          return false;
        }
        // If `root` is the last reference to this variable, it can be marked for collection immediately.
        auto nref = root->use_count();
        if(nref <= 1) {
          root->reset(Source_Location(rocket::sref("<defunct-1>"), 0), G_integer(0xFEEDFACECAFEBEEF), true);
          return false;
        }
        // Enumerate variables that are reachable from `root` indirectly.
        do_enumerate_variables(root,
          [&](const Rcptr<Variable>& child) {
            // If this variable has been inserted indirectly, finish.
            if(!this->m_staging.insert(child)) {
              return false;
            }
            // Initialize the gcref counter.
            // N.B. If this variable is encountered later from `m_tracked`, the gcref counter will be overwritten with 1.
            child->reset_gcref(0);
            // Decend into grandchildren.
            return true;
          });
        return false;
      });
    ///////////////////////////////////////////////////////////////////////////
    // Phase 2
    //   Drop references directly or indirectly from `m_staging`.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const Rcptr<Variable>& root) {
        // Drop a direct reference.
        root->increment_gcref(1);
        ROCKET_ASSERT(root->get_gcref() <= root->use_count());
        // Skip variables that cannot have any children.
        auto split = root->gcref_split();
        if(split <= 0) {
          return false;
        }
        // Enumerate variables that are reachable from `root` indirectly.
        do_enumerate_variables(root,
          [&](const Rcptr<Variable>& child) {
            // Drop an indirect reference.
            child->increment_gcref(split);
            ROCKET_ASSERT(child->get_gcref() <= child->use_count());
            // This is not going to be recursive.
            return false;
          });
        return false;
      });
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Mark variables reachable indirectly from those reachable directly.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const Rcptr<Variable>& root) {
        // Skip variables that are possibly unreachable.
        if(root->get_gcref() >= root->use_count()) {
          return false;
        }
        // Make this variable reachable, ...
        root->reset_gcref(-1);
        // ... as well as all children.
        do_enumerate_variables(root,
          [&](const Rcptr<Variable>& child) {
            // Skip variables that have already been marked.
            if(child->get_gcref() < 0) {
              return false;
            }
            // Mark it, ...
            child->reset_gcref(-1);
            // ... as well as all grandchildren.
            return true;
          });
        return false;
      });
    ///////////////////////////////////////////////////////////////////////////
    // Phase 4
    //   Wipe out variables whose `gcref` counters have excceeded their
    //   reference counts.
    ///////////////////////////////////////////////////////////////////////////
    do_enumerate_variables(this->m_staging,
      [&](const Rcptr<Variable>& root) {
        // All reachable variables will have negative gcref counters.
        if(root->get_gcref() >= 0) {
          // Overwrite the value of this variable with a scalar value to break reference cycles.
          ASTERIA_DEBUG_LOG("\tCollecting unreachable variable: ", root->get_value());
          root->reset(Source_Location(rocket::sref("<defunct-2>"), 0), G_integer(0xFEEDFACECAFEBEEF), true);
          // Cache this variable if a pool is specified.
          if(output) {
            output->insert(root);
          }
          this->m_tracked.erase(root);
          return false;
        }
        // Transfer this variable to the next generational collector, if one has been tied.
        if(!tied) {
          ASTERIA_DEBUG_LOG("\tKeeping reachable variable: ", root->get_value());
          return false;
        }
        ASTERIA_DEBUG_LOG("\tTransferring variable to the next generation: ", root->get_value());
        tied->m_tracked.insert(root);
        // Check whether the next generation needs to be checked as well.
        if(tied->m_counter++ >= tied->m_threshold) {
          collect_tied = true;
        }
        this->m_tracked.erase(root);
        return false;
      });
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    this->m_staging.clear();
    this->m_counter = 0;
    ASTERIA_DEBUG_LOG("Garbage collection ends: this = ", static_cast<void*>(this));
    return collect_tied ? tied : nullptr;
  }

}  // namespace Asteria
