// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Expression::do_compile()
  {
    Cow_Vector<Compiled_Instruction> cinsts;
    cinsts.reserve(this->m_nodes.size());
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.compile(cinsts);  });
    this->m_cinsts = rocket::move(cinsts);
  }

Expression Expression::bind(const Global_Context &global, const Analytic_Context &ctx) const
  {
    Cow_Vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.bind(nodes_bnd, global, ctx);  });
    return rocket::move(nodes_bnd);
  }

bool Expression::evaluate_partial(Reference_Stack &stack_io, const Cow_String &func, const Global_Context &global, const Executive_Context &ctx) const
  {
    auto count = this->m_cinsts.size();
    if(count == 0) {
      return false;
    }
    // Evaluate nodes one by one.
    auto stack_size_old = stack_io.size();
    auto params = std::tie(stack_io, func, global, ctx);
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

void Expression::evaluate(Reference &ref_out, const Cow_String &func, const Global_Context &global, const Executive_Context &ctx) const
  {
    Reference_Stack stack;
    if(!this->evaluate_partial(stack, func, global, ctx)) {
      ref_out = Reference_Root::S_undefined();
      return;
    }
    ROCKET_ASSERT(stack.size() == 1);
    ref_out = rocket::move(stack.mut_top());
  }

void Expression::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_nodes, [&](const Xpnode &node) { node.enumerate_variables(callback);  });
  }

}
