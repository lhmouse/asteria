// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_EXPRESSION_HPP_
#define ASTERIA_SYNTAX_EXPRESSION_HPP_

#include "../fwd.hpp"
#include "../rocket/bind_front.hpp"

namespace Asteria {

class Expression
  {
  public:
    // TODO: In the future we will add JIT support.
    using Compiled_Instruction =
      rocket::bind_front_result<
        void (*)(const void *,
                 const std::tuple<Reference_Stack &, const CoW_String &, const Global_Context &, const Executive_Context &> &),
        const void *>;

  private:
    CoW_Vector<Xpnode> m_nodes;
    CoW_Vector<Compiled_Instruction> m_cinsts;

  public:
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(CoW_Vector<Xpnode> &&nodes)
      : m_nodes(std::move(nodes))
      {
        this->do_compile();
      }

  private:
    void do_compile();

  public:
    bool empty() const noexcept
      {
        return this->m_nodes.empty();
      }

    Expression bind(const Global_Context &global, const Analytic_Context &ctx) const;
    bool evaluate_partial(Reference_Stack &stack_io, const CoW_String &func, const Global_Context &global, const Executive_Context &ctx) const;
    void evaluate(Reference &ref_out, const CoW_String &func, const Global_Context &global, const Executive_Context &ctx) const;

    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
