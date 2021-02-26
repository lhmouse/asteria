// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "variable_callback.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

class Sentry
  {
  private:
    long m_old;
    long* m_ptr;

  public:
    explicit
    Sentry(long& ref)
      noexcept
      : m_old(ref), m_ptr(::std::addressof(ref))
      { ++*(this->m_ptr);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Sentry)
      { --*(this->m_ptr);  }

  public:
    explicit operator
    bool()
      const noexcept
      { return this->m_old == 0;  }
  };

template<typename FuncT>
class Variable_Walker
  final
  : public Variable_Callback
  {
  private:
    ::std::reference_wrapper<FuncT> m_func;

  public:
    explicit
    Variable_Walker(FuncT& func)
      noexcept
      : m_func(func)
      { }

  protected:
    bool
    do_process_one(const rcptr<Variable>& var)
      override
      { return this->m_func(var);  }
  };

template<typename ContT, typename FuncT>
void
do_traverse(const ContT& cont, FuncT&& func)
  {
    Variable_Walker<FuncT> walker(func);
    cont.enumerate_variables(walker);
  }

template<typename FuncT>
void
do_traverse(const Variable& var, FuncT&& func)
  {
    if(ROCKET_EXPECT(var.get_value().is_scalar()))
      return;

    Variable_Walker<FuncT> walker(func);
    var.enumerate_variables_descent(walker);
  }

class Variable_Wiper
  final
  : public Variable_Callback
  {
  protected:
    bool
    do_process_one(const rcptr<Variable>& var)
      override
      {
        // Don't modify variables in place which might have side effects.
        auto value = ::std::move(var->open_value());
        var->uninitialize();

        // Uninitialize all children.
        value.enumerate_variables(*this);
        return false;
      }
  };

}  // namespace

Collector::
~Collector()
  {
  }

bool
Collector::
track_variable(const rcptr<Variable>& var)
  {
    this->m_counter++;
    if(ROCKET_UNEXPECT(this->m_counter > this->m_threshold))
      this->auto_collect();

    return this->m_tracked.insert(var);
  }

bool
Collector::
untrack_variable(const rcptr<Variable>& var)
  noexcept
  {
    return this->m_tracked.erase(var);
  }

Collector*
Collector::
collect_single_opt()
  {
    // Ignore recursive requests.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      return nullptr;

    // The algorithm here is described at
    //   https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

    // We initialize `gc_ref` to zero then increment it, rather than initialize
    // `gc_ref` to the reference count then decrement it. This saves us a phase
    // below.
    Collector* next = nullptr;
    auto output = this->m_output_opt;
    auto tied = this->m_tied_opt;
    this->m_staging.clear();

    // Add variables that are either tracked or reachable indirectly into the
    // staging area.
    do_traverse(this->m_tracked,
      [&](const rcptr<Variable>& root) {
        // Add a variable that is reachable directly.
        // The reference from `m_tracked` should be excluded, so we initialize
        // the `gc_ref` counter to 1.
        root->reset_gc_ref(1);

        // If this variable has been inserted indirectly, finish.
        if(!this->m_staging.insert(root))
          return false;

        // If `root` is the last reference to this variable, it can be marked
        // for collection immediately.
        if(root->use_count() <= 2) {
          root->uninitialize();
          return false;
        }

        // Enumerate variables that are reachable from `root` indirectly.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // If this variable has been inserted indirectly, finish.
            if(!this->m_staging.insert(child))
              return false;

            // Initialize the `gc_ref` counter.
            // N.B. If this variable is encountered later from `m_tracked`,
            // the `gc_ref` counter will be overwritten with 1.
            child->reset_gc_ref(0);
            return true;
          });
        return false;
      });

    // Drop references directly or indirectly from `m_staging`.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // Drop a direct reference.
        bool marked = root->add_gc_ref();
        ROCKET_ASSERT(root->get_gc_ref() <= root->use_count());
        if(marked)
          return false;

        if(!root->is_initialized()) {
          // Zombie variables can now be erased safely.
          root->uninitialize();
          this->m_tracked.erase(root);

          // Cache this variable for reallocation.
          if(output)
            output->insert(root);
          return false;
        }

        // Enumerate variables that are reachable from `root` indirectly.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // Drop an indirect reference.
            child->add_gc_ref();
            ROCKET_ASSERT(child->get_gc_ref() <= child->use_count());
            return false;
          });
        return false;
      });

    // Mark variables reachable indirectly from those reachable directly.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // Skip variables that are possibly unreachable.
        if(root->get_gc_ref() >= root->use_count())
          return false;

        // Mark this variable reachable.
        root->reset_gc_ref(-1);

        // Mark all children reachable as well.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // Skip variables that have already been marked.
            if(child->get_gc_ref() < 0)
              return false;

            // Mark it and its children recursively.
            child->reset_gc_ref(-1);
            return true;
          });
        return false;
      });

    // Collect unreachable variables.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // All variables that are reachable shall have negative `gc_ref` values.
        if(root->get_gc_ref() >= 0) {
          // Break reference cycles.
          root->uninitialize();
          this->m_tracked.erase(root);

          // Cache this variable for reallocation.
          if(output)
            output->insert(root);
          return false;
        }

        if(tied) {
          // Transfer this variable to the next generational collector, if one
          // has been tied.
          tied->m_tracked.insert(root);
          this->m_tracked.erase(root);

          // Check whether the next generation needs to be checked as well.
          if(tied->m_counter++ >= tied->m_threshold)
            next = tied;
          return false;
        }

        // Leave this variable intact.
        return false;
      });

    this->m_staging.clear();
    this->m_counter = 0;
    return next;
  }

void
Collector::
auto_collect()
  {
    auto qnext = this;
    do
      qnext = qnext->collect_single_opt();
    while(qnext);
  }

Collector&
Collector::
wipe_out_variables()
  noexcept
  {
    // Ignore recursive requests.
    const Sentry sentry(this->m_recur);
    if(!sentry)
      return *this;

    // Wipe all variables recursively.
    Variable_Wiper wiper;
    this->m_tracked.enumerate_variables(wiper);
    return *this;
  }

}  // namespace asteria
