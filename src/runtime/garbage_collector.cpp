// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "garbage_collector.hpp"
#include "variable.hpp"
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

bool
do_pop_variable(rcptr<Variable>& var, Variable_HashMap& map)
  {
    for(;;)
      if(!map.erase_random(nullptr, &var))
        return false;
      else if(var)
        return true;
  }

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

    size_t nvars = 0;
    rcptr<Variable> var;

    auto& tracked = this->m_tracked.mut(gMax-gen);
    const auto next_opt = this->m_tracked.mut_ptr(gMax-gen-1);
    const auto count_opt = this->m_counts.mut_ptr(gMax-gen-1);

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreach.clear();

    // This algorithm is described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

    // Collect all variables from `tracked` into `m_staged`.
    this->m_temp_1.merge(tracked);

    while(do_pop_variable(var, this->m_temp_1)) {
      // Each variable that is encountered here shall have a direct reference
      // from either `tracked` or `m_staged`, so its `gc_ref` counter is
      // initialized to one.
      var->set_gc_ref(1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);
      var->get_value().get_variables(this->m_staged, this->m_temp_1);
    }

    while(do_pop_variable(var, this->m_staged)) {
      // Each key in `m_staged` denotes an internal reference, so its `gc_ref`
      // counter shall be incremented.
      var->set_gc_ref(var->get_gc_ref() + 1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);
      this->m_temp_1.insert(var.get(), var);
    }

    // Mark all variables that have been collected so far.
    this->m_temp_1.merge(tracked);

    if(next_opt)
      next_opt->reserve_more(this->m_temp_1.size());

    while(do_pop_variable(var, this->m_temp_1)) {
      // Each variable whose `gc_ref` counter equals its reference count is
      // marked as possibly unreachable. Note `var` here owns a reference
      // which must be excluded.
      if(ROCKET_EXPECT(var->get_gc_ref() == var->use_count() - 1)) {
        this->m_unreach.insert(var.get(), var);
        continue;
      }

      // This variable is reachable.
      this->m_temp_2.insert(var.get(), var);

      while(do_pop_variable(var, this->m_temp_2)) {
        // Mark this indirectly reachable variable, too.
        var->set_gc_ref(0);
        this->m_temp_1.erase(var.get());
        this->m_unreach.erase(var.get());

        var->get_value().get_variables(this->m_staged, this->m_temp_2);

        if(!next_opt)
          continue;

        // Foreign variables must not be transferred.
        if(!tracked.erase(var.get()))
          continue;

        // Transfer this variable to the next generation.
        // Note that storage has been reserved so this shall not cause
        // exceptions.
        ROCKET_ASSERT(next_opt->size() < next_opt->capacity());
        next_opt->insert(var.get(), var);
        *count_opt += 1;
      }
    }

    // Collect all variables from `m_unreach`.
    while(do_pop_variable(var, this->m_unreach)) {
      // Foreign variables must not be collected.
      if(!tracked.erase(var.get()))
        continue;

      // Cache the variable for later use.
      // If an exception is thrown during uninitialization, the variable
      // shall be collected immediately.
      try {
        ROCKET_ASSERT(var->get_gc_ref() != 0);
        var->uninitialize();
        nvars += 1;
        this->m_pool.insert(var.get(), var);
      }
      catch(exception& stdex) {
        ::fprintf(stderr,
            "WARNING: An unusual exception that was thrown during garbage "
            "collection has been caught and ignored. If this issue persists, "
            "please file a bug report.\n"
            "\n"
            "  exception class: %s\n"
            "  what(): %s\n",
            typeid(stdex).name(), stdex.what());
      }
    }

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreach.clear();

    // Reset the GC counter to zero only if the operation completes
    // normally i.e. don't reset it if an exception is thrown.
    this->m_counts[gMax-gen] = 0;

    // Return the number of variables that have been collected.
    return nvars;
  }

rcptr<Variable>
Garbage_Collector::
create_variable(GC_Generation gen_hint)
  {
    // Perform automatic garbage collection.
    for(size_t gen = 0;  gen <= gMax;  ++gen)
      if(this->m_counts[gMax-gen] >= this->m_thres[gMax-gen])
        this->do_collect_generation(gen);

    // Get a cached variable.
    // If the pool has been exhausted, allocate a new one.
    rcptr<Variable> var;
    this->m_pool.erase_random(nullptr, &var);
    if(!var)
      var = ::rocket::make_refcnt<Variable>();

    // Track it.
    size_t gen = gMax - gen_hint;
    this->m_tracked.mut(gen).insert(var.get(), var);
    this->m_counts[gen] += 1;
    return var;
  }

size_t
Garbage_Collector::
collect_variables(GC_Generation gen_limit)
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
      ASTERIA_TERMINATE(("Garbage collector not finalizable while in use"));

    size_t nvars = 0;
    rcptr<Variable> var;

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreach.clear();

    // Wipe out all tracked variables. Indirect ones may be foreign so they
    // must not be wiped.
    for(size_t gen = 0;  gen <= gMax;  ++gen) {
      auto& tracked = this->m_tracked.mut(gMax-gen);
      nvars += tracked.size();

      while(tracked.erase_random(nullptr, &var))
        var->uninitialize();
    }

    // Clear cached variables.
    nvars += this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace asteria
