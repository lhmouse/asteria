// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_Context::~Global_Context()
  {
  }

void Global_Context::do_initialize_runtime(void * /*reserved*/)
  {
    // Initialize the global garbage collector.
    auto collector = rocket::make_refcnt<Generational_Collector>();
    this->do_tie_collector(collector);
    this->m_collector = collector;
    // Create the `std` variable.
    auto std_var = collector->create_variable();
    std_var->reset(Source_Location(rocket::sref("<builtin>"), 0), D_object(), true);
    Reference_Root::S_variable std_ref_c = { std_var };
    this->open_named_reference(rocket::sref("std")) = rocket::move(std_ref_c);
    this->m_std_var = std_var;
    // Add standard library components.
    auto &std_obj = std_var->open_value().check<D_object>();
    ASTERIA_DEBUG_LOG("TODO add std library");
  }

Rcptr<Variable> Global_Context::create_variable() const
  {
    auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_collector);
    ROCKET_ASSERT(collector);
    return collector->create_variable();
  }

bool Global_Context::collect_variables(unsigned gen_limit) const
  {
    auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_collector);
    ROCKET_ASSERT(collector);
    return collector->collect_variables(gen_limit);
  }

const Value & Global_Context::get_std_member(const PreHashed_String &name) const
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->get_value().check<D_object>().get_or(name, Value::get_null());
  }

Value & Global_Context::open_std_member(const PreHashed_String &name)
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->open_value().check<D_object>()[name];
  }

bool Global_Context::remove_std_member(const PreHashed_String &name)
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->open_value().check<D_object>().erase(name);
  }

}
