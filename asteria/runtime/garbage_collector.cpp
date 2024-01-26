// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "garbage_collector.hpp"
#include "variable.hpp"
#include "../utils.hpp"
namespace asteria {

Garbage_Collector::
~Garbage_Collector()
  {
  }

size_t
Garbage_Collector::
do_collect_generation(uint32_t gen)
  {
    // Ignore recursive requests.
    if(this->m_recur > 0)
      return 0;

    this->m_recur ++;
    const ::rocket::unique_ptr<int, void (int*)> rguard(&(this->m_recur), *[](int* ptr) { -- *ptr;  });

    // This algorithm is described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
    size_t nvars = 0;
    refcnt_ptr<Variable> var;
    auto& tracked = this->m_tracked.at(gMax - gen);
    const auto next_opt = (gen >= gMax) ? nullptr : &(this->m_tracked.at(gMax - gen - 1));
    const auto count_opt = (gen >= gMax) ? nullptr : &(this->m_counts.at(gMax - gen - 1));

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreach.clear();
    tracked.merge_into(this->m_temp_1);

    while(this->m_temp_1.extract_variable(var)) {
      // Each variable that is encountered here shall have a direct reference
      // from either `tracked` or `m_staged`, so its `gc_ref` counter shall be
      // initialized to one.
      var->set_gc_ref(1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);
      var->get_value().collect_variables(this->m_staged, this->m_temp_1);
    }

    this->m_temp_1.clear();

    while(this->m_staged.extract_variable(var)) {
      // Each key in `m_staged` denotes an internal reference, so its `gc_ref`
      // counter shall be incremented.
      var->set_gc_ref(var->get_gc_ref() + 1);
      ROCKET_ASSERT(var->get_gc_ref() <= var->use_count() - 1);
      this->m_temp_1.insert(var.get(), var);
    }

    this->m_staged.clear();
    tracked.merge_into(this->m_temp_1);

    while(this->m_temp_1.extract_variable(var)) {
      // Each variable whose `gc_ref` counter equals its reference count is
      // marked as possibly unreachable. Note `var` here owns a reference
      // which must be excluded.
      if(var->get_gc_ref() == var->use_count() - 1)
        this->m_unreach.insert(var.get(), var);
      else
        this->m_temp_2.insert(var.get(), var);
    }

    this->m_temp_1.clear();

    while(this->m_temp_2.extract_variable(var)) {
      // Mark this indirectly reachable variable, too.
      var->set_gc_ref(0);
      this->m_temp_1.erase(var.get());
      this->m_unreach.erase(var.get());

      var->get_value().collect_variables(this->m_staged, this->m_temp_2);

      if(next_opt && tracked.erase(var.get()))
        try {
          // Move the variable to the next generation.
          next_opt->insert(var.get(), var);
          *count_opt += 1;
        }
        catch(...) {
          tracked.insert(var.get(), var);
        }
    }

    this->m_temp_2.clear();

    while(this->m_unreach.extract_variable(var)) {
      // This variable is unreachable now, so collect it.
      if(!tracked.erase(var.get()))
        continue;

      ROCKET_ASSERT(var->get_gc_ref() != 0);
      nvars += 1;

      try {
        // Cache the variable for later use.
        // If an exception is thrown during uninitialization, the variable
        // shall be collected immediately.
        var->uninitialize();
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

    this->m_unreach.clear();

    // Reset the GC counter to zero only if the operation completes
    // normally i.e. don't reset it if an exception is thrown.
    this->m_counts[gMax-gen] = 0;

    // Return the number of variables that have been collected.
    return nvars;
  }

refcnt_ptr<Variable>
Garbage_Collector::
create_variable(GC_Generation gen_hint)
  {
    // Perform automatic garbage collection.
    for(uint32_t gen = 0;  gen <= gMax;  ++gen)
      if(this->m_counts[gMax-gen] >= this->m_thres[gMax-gen])
        this->do_collect_generation(gen);

    // Get a cached variable.
    refcnt_ptr<Variable> var;
    this->m_pool.extract_variable(var);
    if(!var)
      var = ::rocket::make_refcnt<Variable>();

    // Track it.
    size_t gen = gMax - gen_hint;
    this->m_tracked.at(gen).insert(var.get(), var);
    this->m_counts[gen] += 1;
    return var;
  }

size_t
Garbage_Collector::
collect_variables(GC_Generation gen_limit)
  {
    // Collect all variables up to generation `gen_limit`.
    size_t nvars = 0;
    for(uint32_t gen = 0;  (gen <= gMax) && (gen <= gen_limit);  ++gen)
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
    if(this->m_recur > 0)
      ASTERIA_TERMINATE(("Garbage collector not finalizable while in use"));

    size_t nvars = 0;
    refcnt_ptr<Variable> var;

    this->m_staged.clear();
    this->m_temp_1.clear();
    this->m_temp_2.clear();
    this->m_unreach.clear();

    for(size_t gen = 0;  gen <= gMax;  ++gen)
      nvars += this->m_tracked.at(gMax-gen).size();

    // Wipe out all tracked variables. Indirect ones may be foreign so they
    // must not be wiped.
    for(size_t gen = 0;  gen <= gMax;  ++gen)
      while(this->m_tracked.at(gMax-gen).extract_variable(var))
        var->uninitialize();

    // Clear cached variables.
    nvars += this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace asteria
