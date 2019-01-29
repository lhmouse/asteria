// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/function_analytic_context.hpp"
#include "../runtime/function_executive_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Block::do_compile()
  {
    CoW_Vector<Compiled_Instruction> cinsts;
    cinsts.reserve(this->m_stmts.size());
    rocket::for_each(this->m_stmts, [&](const Statement &stmt) { stmt.compile(cinsts); });
    this->m_cinsts = std::move(cinsts);
  }

void Block::fly_over_in_place(Abstract_Context &ctx_io) const
  {
    rocket::for_each(this->m_stmts, [&](const Statement &stmt) { stmt.fly_over_in_place(ctx_io); });
  }

Block Block::bind_in_place(Analytic_Context &ctx_io, const Global_Context &global) const
  {
    CoW_Vector<Statement> stmts_bnd;
    stmts_bnd.reserve(this->m_stmts.size());
    rocket::for_each(this->m_stmts, [&](const Statement &stmt) { stmt.bind_in_place(stmts_bnd, ctx_io, global); });
    return std::move(stmts_bnd);
  }

Block::Status Block::execute_in_place(Reference &ref_out, Executive_Context &ctx_io, Global_Context &global) const
  {
    auto rptr = this->m_cinsts.data();
    const auto eptr = rptr + this->m_cinsts.size();
    if(rptr == eptr) {
      return status_next;
    }
    // Execute statements one by one.
    const auto params = std::tie(ref_out, ctx_io, global);
    for(;;) {
      const auto status = (*rptr)(params);
      if(ROCKET_UNEXPECT(status != status_next)) {
        // Forward anything unexpected recursively.
        return status;
      }
      ++rptr;
      if(rptr == eptr) {
        break;
      }
    }
    // The current control flow has reached the end of this block.
    return status_next;
  }

Block Block::bind(const Global_Context &global, const Analytic_Context &ctx) const
  {
    Analytic_Context ctx_next(&ctx);
    return this->bind_in_place(ctx_next, global);
  }

Block::Status Block::execute(Reference &ref_out, Global_Context &global, const Executive_Context &ctx) const
  {
    Executive_Context ctx_next(&ctx);
    return this->execute_in_place(ref_out, ctx_next, global);
  }

Instantiated_Function Block::instantiate_function(Global_Context &global, const Executive_Context &ctx, const Source_Location &sloc, const PreHashed_String &name, const CoW_Vector<PreHashed_String> &params) const
  {
    Function_Analytic_Context ctx_next(&ctx);
    ctx_next.initialize(params);
    // Bind the body recursively.
    auto body_bnd = this->bind_in_place(ctx_next, global);
    return Instantiated_Function(sloc, name, params, std::move(body_bnd));
  }

void Block::execute_as_function(Reference &self_io, Global_Context &global, const RefCnt_Object<Variadic_Arguer> &zvarg, const CoW_Vector<PreHashed_String> &params, CoW_Vector<Reference> &&args) const
  {
    Function_Executive_Context ctx_next(nullptr);
    ctx_next.initialize(zvarg, params, std::move(self_io), std::move(args));
    // Execute the body.
    const auto status = this->execute_in_place(self_io, ctx_next, global);
    switch(status) {
    case status_next:
      {
        // Return `null` if the control flow reached the end of the function.
        self_io = Reference_Root::S_null();
        return;
      }
    case status_return:
      {
        // Return the reference in `self_io`.
        return;
      }
    case status_break_unspec:
    case status_break_switch:
    case status_break_while:
    case status_break_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
      }
    case status_continue_unspec:
    case status_continue_while:
    case status_continue_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
      }
    default:
      ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
    }
  }

void Block::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_stmts, [&](const Statement &stmt) { stmt.enumerate_variables(callback); });
  }

}
