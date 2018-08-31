// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::Expression(Vector<Xpnode> &&nodes) noexcept
  : m_nodes(std::move(nodes))
  {
  }
Expression::~Expression()
  {
  }

Expression::Expression(Expression &&) noexcept
  = default;
Expression & Expression::operator=(Expression &&) noexcept
  = default;

bool Expression::empty() const noexcept
  {
    return this->m_nodes.empty();
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

Reference Expression::evaluate(const Executive_context &ctx) const
  {
    Vector<Reference> stack;
    for(const auto &node : this->m_nodes) {
      node.evaluate(stack, ctx);
    }
    switch(stack.size()) {
    case 0:
      return { };
    case 1:
      return std::move(stack.mut_front());
    default:
      ASTERIA_THROW_RUNTIME_ERROR("The expression is unbalanced.");
    }
  }

}
