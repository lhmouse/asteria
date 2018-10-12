// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "instantiated_function.hpp"
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

Block::Status Block::execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context *global_opt) const
  {
    for(const auto &stmt : this->m_stmts) {
      const auto status = stmt.execute_in_place(ref_out, ctx_io, global_opt);
      if(status != status_next) {
        return status;
      }
    }
    return status_next;
  }

Block Block::bind(const Global_context *global_opt, const Analytic_context &ctx) const
  {
    Analytic_context ctx_next(&ctx);
    return this->bind_in_place(ctx_next, global_opt);
  }

Block::Status Block::execute(Reference &ref_out, Global_context *global_opt, const Executive_context &ctx) const
  {
    Executive_context ctx_next(&ctx);
    return this->execute_in_place(ref_out, ctx_next, global_opt);
  }

Instantiated_function Block::instantiate_function(Global_context *global_opt, const Executive_context &ctx, String file, Uint32 line, String name, Vector<String> params) const
  {
    Analytic_context ctx_next(&ctx);
    ctx_next.initialize_for_function(params);
    // Bind the body recursively.
    auto body_bnd = this->bind_in_place(ctx_next, global_opt);
    return Instantiated_function(std::move(file), line, std::move(name), std::move(params), std::move(body_bnd));
  }

Reference Block::execute_as_function(Global_context *global_opt, String file, Uint32 line, String name, const Vector<String> &params, Reference self, Vector<Reference> args) const
  {
    Executive_context ctx_next(nullptr);
    ctx_next.initialize_for_function(std::move(file), line, std::move(name), params, std::move(self), std::move(args));
    // Execute the body.
    Reference result;
    const auto status = this->execute_in_place(result, ctx_next, global_opt);
    switch(status) {
      case status_next: {
        // Return `null` if the control flow reached the end of the function.
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
        return std::move(result);
      }
      default: {
        ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
      }
    }
  }

void Block::collect_variables(bool (*callback)(void *, const Rcptr<Variable> &), void *param) const
  {
    for(const auto &stmt : this->m_stmts) {
      stmt.collect_variables(callback, param);
    }
  }

}
