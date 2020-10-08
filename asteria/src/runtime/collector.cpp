// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "collector.hpp"
#include "variable.hpp"
#include "variable_callback.hpp"
#include "../util.hpp"

namespace asteria {
namespace {

class Sentry
  {
  private:
    ref<long> m_ref;
    long m_old;

  public:
    explicit
    Sentry(long& ref)
    noexcept
      : m_ref(ref), m_old(ref)
      { this->m_ref++;  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Sentry)
      { this->m_ref--;  }

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

    // We initialize `gcref` to zero then increment it, rather than initialize `gcref` to
    // the reference count then decrement it. This saves a phase below for us.
    Collector* next = nullptr;
    auto output = this->m_output_opt;
    auto tied = this->m_tied_opt;
    this->m_staging.clear();

    // Add variables that are either tracked or reachable indirectly into the staging area.
    do_traverse(this->m_tracked,
      [&](const rcptr<Variable>& root) {
        // Add a variable that is reachable directly.
        // The reference from `m_tracked` should be excluded, so we initialize the gcref
        // counter to 1.
        root->reset_gcref(1);

        // If this variable has been inserted indirectly, finish.
        if(!this->m_staging.insert(root))
          return false;

        // If `root` is the last reference to this variable, it can be marked for collection
        // immediately.
        auto nref = root->use_count();
        if(nref <= 1) {
          root->uninitialize();
          return false;
        }

        // Enumerate variables that are reachable from `root` indirectly.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // If this variable has been inserted indirectly, finish.
            if(!this->m_staging.insert(child))
              return false;

            // Initialize the gcref counter.
            // N.B. If this variable is encountered later from `m_tracked`, the gcref counter
            // will be overwritten with 1.
            child->reset_gcref(0);
            return true;
          });
        return false;
      });

    // Drop references directly or indirectly from `m_staging`.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // Drop a direct reference.
        root->increment_gcref(1);
        ROCKET_ASSERT(root->get_gcref() <= root->use_count());

        // Skip variables that cannot have any children.
        auto split = root->gcref_split();
        if(split <= 0)
          return false;

        // Enumerate variables that are reachable from `root` indirectly.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // Drop an indirect reference.
            child->increment_gcref(split);
            ROCKET_ASSERT(child->get_gcref() <= child->use_count());
            return false;
          });
        return false;
      });

    // Mark variables reachable indirectly from those reachable directly.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // Skip variables that are possibly unreachable.
        if(root->get_gcref() >= root->use_count())
          return false;

        // Make this variable reachable, ...
        root->reset_gcref(-1);
        // ... as well as all children.
        do_traverse(*root,
          [&](const rcptr<Variable>& child) {
            // Skip variables that have already been marked.
            if(child->get_gcref() < 0)
              return false;

            // Mark it, ...
            child->reset_gcref(-1);
            // ... as well as all grandchildren.
            return true;
          });
        return false;
      });

    // Wipe out variables whose `gcref` counters have excceeded their reference counts.
    do_traverse(this->m_staging,
      [&](const rcptr<Variable>& root) {
        // All reachable variables will have negative gcref counters.
        if(root->get_gcref() >= 0) {
          // Break reference cycles.
          root->uninitialize();

          // Cache this variable if a pool is specified.
          if(output)
            output->insert(root);

          this->m_tracked.erase(root);
          return false;
        }

        if(tied) {
          // Transfer this variable to the next generational collector, if one has been tied.
          tied->m_tracked.insert(root);

          // Check whether the next generation needs to be checked as well.
          if(tied->m_counter++ >= tied->m_threshold)
            next = tied;

          this->m_tracked.erase(root);
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
