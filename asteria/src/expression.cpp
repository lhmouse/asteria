// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::~Expression()
  {
  }

Expression Expression::bind(const Analytic_context &ctx) const
  {
    Vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      auto node_bnd = node.bind(ctx);
      nodes_bnd.emplace_back(std::move(node_bnd));
    }
    return std::move(nodes_bnd);
  }

bool Expression::empty() const noexcept
  {
    return this->m_nodes.empty();
  }

bool Expression::evaluate_partial(Vector<Reference> &stack_io, const Executive_context &ctx) const
  {
    if(this->m_nodes.empty()) {
      return false;
    }
    const auto stack_size_old = stack_io.size();
    for(const auto &node : this->m_nodes) {
      node.evaluate(stack_io, ctx);
      ROCKET_ASSERT(stack_io.size() >= stack_size_old);
    }
    if(stack_io.size() - stack_size_old != 1) {
      ASTERIA_THROW_RUNTIME_ERROR("The expression is unbalanced.");
    }
    return true;
  }

Reference Expression::evaluate(const Executive_context &ctx) const
  {
    Vector<Reference> stack;
    if(this->evaluate_partial(stack, ctx) == false) {
      return { };
    }
    return std::move(stack.mut_back());
  }

}
