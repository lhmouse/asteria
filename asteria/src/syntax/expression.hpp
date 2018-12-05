// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_EXPRESSION_HPP_
#define ASTERIA_SYNTAX_EXPRESSION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Expression
  {
  private:
    rocket::cow_vector<Xpnode> m_nodes;
    rocket::cow_vector<rocket::binder_first<void (*)(const void *, Reference_stack &, Global_context &, const Executive_context &), const void *>> m_jinsts;

  public:
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(rocket::cow_vector<Xpnode> &&nodes)
      : m_nodes(std::move(nodes))
      {
        this->do_compile();
      }
    ROCKET_COPYABLE_DESTRUCTOR(Expression);

  private:
    void do_compile();

  public:
    Expression bind(const Global_context &global, const Analytic_context &ctx) const;
    bool empty() const noexcept;
    bool evaluate_partial(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const;
    void evaluate(Reference &ref_out, Global_context &global, const Executive_context &ctx) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
