// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Expression
  {
  private:
    Vector<Xpnode> m_nodes;

  public:
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(Vector<Xpnode> &&nodes) noexcept
      : m_nodes(std::move(nodes))
      {
      }
    ~Expression();

  public:
    Expression bind(const Global_context &global, const Analytic_context &ctx) const;
    bool empty() const noexcept;
    bool evaluate_partial(Vector<Reference> &stack_io, Global_context &global, const Executive_context &ctx) const;
    Reference evaluate(Global_context &global, const Executive_context &ctx) const;

    void collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const;
  };

}

#endif
