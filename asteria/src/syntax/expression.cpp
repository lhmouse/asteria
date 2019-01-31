// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Expression::do_compile()
  {
    CoW_Vector<Compiled_Instruction> cinsts;
    cinsts.reserve(this->m_nodes.size());
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.compile(cinsts); });
    this->m_cinsts = std::move(cinsts);
  }

Expression Expression::bind(const Global_Context &global, const Analytic_Context &ctx) const
  {
    CoW_Vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.bind(nodes_bnd, global, ctx); });
    return std::move(nodes_bnd);
  }

bool Expression::evaluate_partial(Reference_Stack &stack_io, Global_Context &global, const CoW_String &func, const Executive_Context &ctx) const
  {
    auto count = this->m_cinsts.size();
    if(count == 0) {
      return false;
    }
    // Evaluate nodes one by one.
    const auto stack_size_old = stack_io.size();
    const auto params = std::tie(stack_io, global, func, ctx);
    auto cptr = this->m_cinsts.data();
    while(--count != 0) {
      (*cptr)(params);
      ROCKET_ASSERT_MSG(stack_io.size() >= stack_size_old, "The expression evaluation stack is corrupted.");
      ++cptr;
    }
    (*cptr)(params);
    ROCKET_ASSERT(stack_io.size() - stack_size_old == 1);
    return true;
  }

void Expression::evaluate(Reference &ref_out, Global_Context &global, const CoW_String &func, const Executive_Context &ctx) const
  {
    Reference_Stack stack;
    if(!this->evaluate_partial(stack, global, func, ctx)) {
      ref_out = Reference_Root::S_null();
      return;
    }
    ROCKET_ASSERT(stack.size() == 1);
    ref_out = std::move(stack.mut_top());
  }

void Expression::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.enumerate_variables(callback); });
  }

}
