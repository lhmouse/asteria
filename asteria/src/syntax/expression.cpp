// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Expression::~Expression()
  {
  }

void Expression::do_compile()
  {
    this->m_jinsts.clear();
    this->m_jinsts.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      this->m_jinsts.emplace_back(node.compile());
    }
  }

Expression Expression::bind(const Global_context &global, const Analytic_context &ctx) const
  {
    rocket::cow_vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      auto node_bnd = node.bind(global, ctx);
      nodes_bnd.emplace_back(std::move(node_bnd));
    }
    return std::move(nodes_bnd);
  }

bool Expression::empty() const noexcept
  {
    return this->m_nodes.empty();
  }

bool Expression::evaluate_partial(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const
  {
    auto rptr = this->m_jinsts.data();
    const auto eptr = rptr + this->m_jinsts.size();
    if(rptr == eptr) {
      return false;
    }
    const auto stack_size_old = stack_io.size();
    // Unroll the loop using Duff's Device.
    const auto rem = static_cast<std::uintptr_t>(eptr - rptr) % 4;
    rptr += rem;
    switch(rem) {
      do {
    default:
        rptr += 4;
        rptr[-4](stack_io, global, ctx);
        ROCKET_ASSERT(stack_io.size() >= stack_size_old);
        // Fallthrough.
    case 3:
        rptr[-3](stack_io, global, ctx);
        ROCKET_ASSERT(stack_io.size() >= stack_size_old);
        // Fallthrough.
    case 2:
        rptr[-2](stack_io, global, ctx);
        ROCKET_ASSERT(stack_io.size() >= stack_size_old);
        // Fallthrough.
    case 1:
        rptr[-1](stack_io, global, ctx);
        ROCKET_ASSERT(stack_io.size() >= stack_size_old);
      } while(rptr != eptr);
    }
    if(stack_io.size() - stack_size_old != 1) {
      ASTERIA_THROW_RUNTIME_ERROR("The expression is unbalanced.");
    }
    return true;
  }

void Expression::evaluate(Reference &ref_out, Global_context &global, const Executive_context &ctx) const
  {
    Reference_stack stack;
    if(!this->evaluate_partial(stack, global, ctx)) {
      ref_out = Reference_root::S_null();
      return;
    }
    ROCKET_ASSERT(stack.size() == 1);
    ref_out = std::move(stack.top());
  }

void Expression::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &node : this->m_nodes) {
      node.enumerate_variables(callback);
    }
  }

}
