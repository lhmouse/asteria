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
    Cow_Vector<Compiled_Instruction> cinsts;
    cinsts.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      node.compile(cinsts);
    }
    this->m_cinsts = std::move(cinsts);
  }

Expression Expression::bind(const Global_Context &global, const Analytic_Context &ctx) const
  {
    Cow_Vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      node.bind(nodes_bnd, global, ctx);
    }
    return std::move(nodes_bnd);
  }

bool Expression::evaluate_partial(Reference_Stack &stack_io, Global_Context &global, const Executive_Context &ctx) const
  {
    auto rptr = this->m_cinsts.data();
    const auto eptr = rptr + this->m_cinsts.size();
    if(rptr == eptr) {
      return false;
    }
    const auto stack_size_old = stack_io.size();
    do {
      // Evaluate nodes one by one.
      (*rptr)(stack_io, global, ctx);
      ROCKET_ASSERT(stack_io.size() >= stack_size_old);
    } while(ROCKET_EXPECT(++rptr != eptr));
    // The value of this expression shall be on the top of `stack_io`. There will be no more elements pushed after all.
    ROCKET_ASSERT(stack_io.size() - stack_size_old == 1);
    return true;
  }

void Expression::evaluate(Reference &ref_out, Global_Context &global, const Executive_Context &ctx) const
  {
    Reference_Stack stack;
    if(!this->evaluate_partial(stack, global, ctx)) {
      ref_out = Reference_Root::S_null();
      return;
    }
    ROCKET_ASSERT(stack.size() == 1);
    ref_out = std::move(stack.mut_top());
  }

void Expression::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    for(const auto &node : this->m_nodes) {
      node.enumerate_variables(callback);
    }
  }

}
