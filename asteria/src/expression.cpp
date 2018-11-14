// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "reference_stack.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::~Expression()
  {
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
    if(this->m_nodes.empty()) {
      return false;
    }
    const auto stack_size_old = stack_io.size();
    for(const auto &node : this->m_nodes) {
      node.evaluate(stack_io, global, ctx);
      ROCKET_ASSERT(stack_io.size() >= stack_size_old);
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
      Reference_root::S_constant ref_c = { D_null() };
      ref_out = std::move(ref_c);
      return;
    }
    ROCKET_ASSERT(!stack.empty());
    ref_out = std::move(stack.top());
  }

void Expression::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &node : this->m_nodes) {
      node.enumerate_variables(callback);
    }
  }

}
