// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_EXPRESSION_HPP_
#define ASTERIA_SYNTAX_EXPRESSION_HPP_

#include "../fwd.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Expression
  {
  private:
    rocket::cow_vector<Xpnode> m_nodes;

  public:
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(rocket::cow_vector<Xpnode> &&nodes) noexcept
      : m_nodes(std::move(nodes))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Expression);

  public:
    Expression bind(const Global_context &global, const Analytic_context &ctx) const;
    bool empty() const noexcept;
    bool evaluate_partial(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const;
    void evaluate(Reference &ref_out, Global_context &global, const Executive_context &ctx) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
