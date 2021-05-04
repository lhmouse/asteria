// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "garbage_collector.hpp"
#include "variable.hpp"
#include "variable_callback.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

class Sentry
  {
  private:
    long* m_ptr;
    long m_old;

  public:
    explicit
    Sentry(long& ref) noexcept
      : m_ptr(&ref), m_old(ref)
      { ++*(this->m_ptr);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Sentry)
      { --*(this->m_ptr);  }

  public:
    explicit operator
    bool() const noexcept
      { return this->m_old == 0;  }
  };

struct S1_init_gc_ref_1 : Variable_Callback
  {
    Variable_HashSet* staging;

    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        // Add the variable. Do nothing if it has already been added.
        bool added = staging->insert(var);
        if(!added)
          return false;

        // Initialize `gc_ref` to one. This can be overwritten later if
        // the variable is reached again from `m_tracked`. All variables
        // reachable indirectly are collected via this callback.
        var->set_gc_ref(1);
        return true;
      }
  };

struct S1_init_gc_ref_2_NR : Variable_Callback
  {
    Variable_HashSet* staging;

    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        // If `var` is a unique reference, mark the variable for collection
        // immediately. It is however not safe to erase it when iterating.
        if(var.unique())
          var->uninitialize();

        // Try adding the variable first.
        bool added = staging->insert(var);

        // No matter whether it succeeds or not, there shall be at least
        // two references to `var` (one is from `m_tracked` and the other
        // is from `m_staging`), so initialize `gc_ref` to two.
        var->set_gc_ref(2);

        // If the variable had been added already, do nothing.
        if(!added)
          return false;

        // Collect indirectly reachable variables with another recursive
        // function. The current one is not recursive.
        S1_init_gc_ref_1 s1_init;
        s1_init.staging = staging;
        var->get_value().enumerate_variables(s1_init);
        return false;
      }
  };

struct S2_add_gc_ref_frac_NR : Variable_Callback
  {
    Pointer_HashSet* id_ptrs;

    bool
    do_process_one(const void* id_ptr, const rcptr<Variable>& var) final
      {
        // Ensure each reference pointer is added only once.
        bool added = id_ptrs->insert(id_ptr);
        if(!added)
          return false;

        // Drop the reference count from this pointer.
        var->set_gc_ref(var->get_gc_ref() + 1);
        return false;
      }
  };

struct S2_iref_internal_NR : Variable_Callback
  {
    Pointer_HashSet* id_ptrs;

    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        // Drop indirect references from `m_staging`.
        S2_add_gc_ref_frac_NR s2_gcadd;
        s2_gcadd.id_ptrs = id_ptrs;
        var->get_value().enumerate_variables(s2_gcadd);
        return false;
      }
  };

struct S3_mark_reachable : Variable_Callback
  {
    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        // Skip unrachable variables.
        if(var->get_gc_ref() >= var->use_count())
          return false;

        // Skip marked variables.
        if(var->get_gc_ref() == 0)
          return false;

        // Mark this variable reachable, as well as all children.
        // This is recursive.
        var->set_gc_ref(0);
        return true;
      }
  };

struct S4_reap_unreachable : Variable_Callback
  {
    Variable_HashSet* pool;
    Variable_HashSet* tracked;
    Variable_HashSet* next_opt;
    size_t nvars = 0;

    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        try {
          // Reachable variables shall have `gc_ref` counters set to zero.
          if(var->get_gc_ref() != 0) {
            // Wipe the value out.
            var->uninitialize();
            bool erased = tracked->erase(var);
            nvars++;

            // Pool the variable. This shall be the last operation due to
            // possible exceptions. If the variable cannot be pooled, it is
            // deallocated immediately.
            if(erased)
              pool->insert(var);
          }
          else if(next_opt) {
            // Transfer this reachable variable to the next generation.
            // Note exception safety.
            next_opt->insert(var);
            bool erased = tracked->erase(var);
            if(!erased)
              next_opt->erase(var);
          }
        }
        catch(exception& stdex) {
          ::fprintf(stderr,
              "WARNING: an exception was thrown during garbage collection. "
              "If this problem persists, please file a bug report.\n"
              "\n"
              "  exception class: %s\n"
              "  what(): %s\n",
              typeid(stdex).name(), stdex.what());
        }
        return false;
      }
  };

struct Sz_wipe_variable_NR : Variable_Callback
  {
    bool
    do_process_one(const void* /*id_ptr*/, const rcptr<Variable>& var) final
      {
        // Destroy the value. This shall not be recursive.
        var->uninitialize();
        return false;
      }
  };

}  // namespace

Garbage_Collector::
~Garbage_Collector()
  {
  }

size_t
Garbage_Collector::
do_collect_generation(size_t gen)
  {
    // Ignore recursive requests.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      return 0;

    auto& tracked = this->m_tracked.mut(gMax-gen);
    this->m_staging.clear();
    this->m_id_ptrs.clear();

    // This algorithm is described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

    // Collect all variables from `tracked` into `m_staging`, recursively.
    // The references from `tracked` and `m_staging` shall be excluded, so the
    // `gc_ref` counter is initialized to two if a variable is reached from
    // `tracked`, and to one if it is reached indirectly.
    S1_init_gc_ref_2_NR s1_init;
    s1_init.staging = &(this->m_staging);
    tracked.enumerate_variables(s1_init);

    // For each variable that is exactly one-level indirectly reachable from
    // those in `m_staging`, the `gc_ref` counter is incremented by one.
    S2_iref_internal_NR s2_iref;
    s2_iref.id_ptrs = &(this->m_id_ptrs);
    this->m_staging.enumerate_variables(s2_iref);

    // A variable whose `gc_ref` counter is less than its reference count is
    // reachable. Variables that are reachable from it are reachable, too.
    S3_mark_reachable s3_mark;
    this->m_staging.enumerate_variables(s3_mark);

    // Collect each variable whose `gc_ref` counter is zero.
    S4_reap_unreachable s4_reap;
    s4_reap.pool = &(this->m_pool);
    s4_reap.tracked = &tracked;
    s4_reap.next_opt = this->m_tracked.mut_ptr(gMax-gen-1);
    this->m_staging.enumerate_variables(s4_reap);

    // Reset the GC counter to zero, only if the operation completes normally
    // i.e. don't reset it if an exception is thrown.
    this->m_counts[gMax-gen] = 0;
    return s4_reap.nvars;
  }

rcptr<Variable>
Garbage_Collector::
create_variable(uint8_t gen_hint)
  {
    // Perform automatic garbage collection.
    for(size_t gen = 0;  gen <= gMax;  ++gen)
      if(this->m_counts[gMax-gen] >= this->m_thres[gMax-gen])
        this->do_collect_generation(gen);

    this->m_staging.clear();
    this->m_id_ptrs.clear();

    // Get a cached variable.
    // If the pool has been exhausted, allocate a new one.
    auto var = this->m_pool.erase_random_opt();
    if(!var)
      var = ::rocket::make_refcnt<Variable>();

    // Track it.
    size_t gen = gMax - gen_hint;
    this->m_tracked.mut(gen).insert(var);
    this->m_counts[gen] += 1;
    return var;
  }

size_t
Garbage_Collector::
collect_variables(uint8_t gen_limit)
  {
    // Collect all variables up to generation `gen_limit`.
    size_t nvars = 0;
    for(size_t gen = 0;  (gen <= gMax) && (gen <= gen_limit);  ++gen)
      nvars += this->do_collect_generation(gen);

    // Clear cached variables.
    // Return the number of variables that have been collected.
    this->m_pool.clear();
    return nvars;
  }

size_t
Garbage_Collector::
finalize() noexcept
  {
    // Ensure no garbage collection is in progress.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      ASTERIA_TERMINATE("garbage collector not finalizable while in use");

    // Wipe out all tracked variables.
    size_t nvars = 0;
    for(size_t gen = 0;  gen <= gMax;  ++gen) {
      auto& tracked = this->m_tracked.mut(gMax-gen);

      // Wipe variables that are reachable directly. Indirect ones may
      // be foreign so they must not be wiped.
      Sz_wipe_variable_NR wipe;
      tracked.enumerate_variables(wipe);

      nvars += tracked.size();
      tracked.clear();
    }

    this->m_staging.clear();
    this->m_id_ptrs.clear();

    // Clear cached variables.
    nvars += this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace asteria
