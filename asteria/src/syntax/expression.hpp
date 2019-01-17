// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_EXPRESSION_HPP_
#define ASTERIA_SYNTAX_EXPRESSION_HPP_

#include "../fwd.hpp"
#include "../rocket/bind_first.hpp"

namespace Asteria {

class Expression
  {
  public:
    // TODO: In the future we will add JIT support.
    using Compiled_Instruction = rocket::binder_first<void (*)(const void *, Reference_Stack &, Global_Context &, const Executive_Context &),
                                                      const void *>;

  private:
    Cow_Vector<Xpnode> m_nodes;
    Cow_Vector<Compiled_Instruction> m_cinsts;

  public:
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(Cow_Vector<Xpnode> &&nodes)
      : m_nodes(std::move(nodes))
      {
        this->do_compile();
      }
    ROCKET_COPYABLE_DESTRUCTOR(Expression);

  private:
    void do_compile();

  public:
    bool empty() const noexcept
      {
        return this->m_nodes.empty();
      }

    Expression bind(const Global_Context &global, const Analytic_Context &ctx) const;
    bool evaluate_partial(Reference_Stack &stack_io, Global_Context &global, const Executive_Context &ctx) const;
    void evaluate(Reference &ref_out, Global_Context &global, const Executive_Context &ctx) const;

    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
