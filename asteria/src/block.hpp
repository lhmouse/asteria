// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BLOCK_HPP_
#define ASTERIA_BLOCK_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Block
  {
  public:
    enum Status : Uint8
      {
        status_next             = 0,
        status_break_unspec     = 1,
        status_break_switch     = 2,
        status_break_while      = 3,
        status_break_for        = 4,
        status_continue_unspec  = 5,
        status_continue_while   = 6,
        status_continue_for     = 7,
        status_return           = 8,
      };

  private:
    Vector<Statement> m_stmts;

  public:
    Block() noexcept
      : m_stmts()
      {
      }
    Block(Vector<Statement> &&stmts) noexcept
      : m_stmts(std::move(stmts))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Block);

  public:
    void fly_over_in_place(Abstract_context &ctx_io) const;
    Block bind_in_place(Analytic_context &ctx_io, const Global_context &global) const;
    Status execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context &global) const;

    Block bind(const Global_context &global, const Analytic_context &ctx) const;
    Status execute(Reference &ref_out, Global_context &global, const Executive_context &ctx) const;

    Instantiated_function instantiate_function(Global_context &global, const Executive_context &ctx, const Function_header &head) const;
    Reference execute_as_function(Global_context &global, const Function_header &head, const Shared_function_wrapper *zvarg_opt, Reference self, Vector<Reference> args) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
