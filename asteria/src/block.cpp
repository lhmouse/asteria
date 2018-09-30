// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::~Block()
  {
  }

void Block::fly_over_in_place(Abstract_context &ctx_io) const
  {
    for(const auto &stmt : this->m_stmts) {
      stmt.fly_over_in_place(ctx_io);
    }
  }

Block Block::bind_in_place(Analytic_context &ctx_io, const Global_context *global_opt) const
  {
    Vector<Statement> stmts_bnd;
    stmts_bnd.reserve(this->m_stmts.size());
    for(const auto &stmt : this->m_stmts) {
      auto alt_bnd = stmt.bind_in_place(ctx_io, global_opt);
      stmts_bnd.emplace_back(std::move(alt_bnd));
    }
    return std::move(stmts_bnd);
  }

Block Block::bind(const Global_context *global_opt, const Analytic_context &ctx) const
  {
    Analytic_context ctx_next(&ctx);
    return this->bind_in_place(ctx_next, global_opt);
  }

Block::Status Block::execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context *global_opt) const
  {
    for(const auto &stmt : this->m_stmts) {
      const auto status = stmt.execute_in_place(ref_out, ctx_io, global_opt);
      if(status != status_next) {
        // Forward anything unexpected to the caller.
        return status;
      }
    }
    return status_next;
  }

Reference Block::execute_as_function_in_place(Executive_context &ctx_io, Global_context *global_opt) const
  {
    Reference ref;
    const auto status = this->execute_in_place(ref, ctx_io, global_opt);
    switch(status) {
      case status_next: {
        // Return `null` because the control flow reached the end of the function.
        return { };
      }
      case status_break_unspec:
      case status_break_switch:
      case status_break_while:
      case status_break_for: {
        ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
      }
      case status_continue_unspec:
      case status_continue_while:
      case status_continue_for: {
        ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
      }
      case status_return: {
        // Forward the result reference.
        return std::move(ref);
      }
      default: {
        ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
      }
    }
  }

Block::Status Block::execute(Reference &ref_out, Global_context *global_opt, const Executive_context &ctx) const
  {
    Executive_context ctx_next(&ctx);
    return this->execute_in_place(ref_out, ctx_next, global_opt);
  }

}
