// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"

namespace Asteria {

Block::~Block()
  {
  }

void Block::do_compile()
  {
    this->m_jinsts.clear();
    this->m_jinsts.reserve(this->m_stmts.size());
    for(const auto &stmt : this->m_stmts) {
      this->m_jinsts.emplace_back(stmt.compile());
    }
  }

void Block::fly_over_in_place(Abstract_context &ctx_io) const
  {
    for(const auto &stmt : this->m_stmts) {
      stmt.fly_over_in_place(ctx_io);
    }
  }

Block Block::bind_in_place(Analytic_context &ctx_io, const Global_context &global) const
  {
    rocket::cow_vector<Statement> stmts_bnd;
    stmts_bnd.reserve(this->m_stmts.size());
    for(const auto &stmt : this->m_stmts) {
      auto alt_bnd = stmt.bind_in_place(ctx_io, global);
      stmts_bnd.emplace_back(std::move(alt_bnd));
    }
    return std::move(stmts_bnd);
  }

Block::Status Block::execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context &global) const
  {
    auto rptr = this->m_jinsts.data();
    const auto eptr = rptr + this->m_jinsts.size();
    if(rptr == eptr) {
      return status_next;
    }
    auto status = status_next;
    // Unroll the loop using Duff's Device.
    const auto rem = static_cast<std::uintptr_t>(eptr - rptr - 1) % 8;
    rptr += rem + 1;
    switch(rem) {
      do {
        rptr += 8;
        // Fallthrough.
    case 7:
        status = rptr[-8](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 6:
        status = rptr[-7](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 5:
        status = rptr[-6](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 4:
        status = rptr[-5](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 3:
        status = rptr[-4](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 2:
        status = rptr[-3](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    case 1:
        status = rptr[-2](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
        // Fallthrough.
    default:
        status = rptr[-1](ref_out, ctx_io, global);
        if(ROCKET_UNEXPECT(status != status_next)) {
          break;
        }
      } while(rptr != eptr);
    }
    return status;
  }

Block Block::bind(const Global_context &global, const Analytic_context &ctx) const
  {
    Analytic_context ctx_next(&ctx);
    return this->bind_in_place(ctx_next, global);
  }

Block::Status Block::execute(Reference &ref_out, Global_context &global, const Executive_context &ctx) const
  {
    Executive_context ctx_next(&ctx);
    return this->execute_in_place(ref_out, ctx_next, global);
  }

Instantiated_function Block::instantiate_function(Global_context &global, const Executive_context &ctx, const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params) const
  {
    Analytic_context ctx_next(&ctx);
    ctx_next.initialize_for_function(params);
    // Bind the body recursively.
    auto body_bnd = this->bind_in_place(ctx_next, global);
    return Instantiated_function(loc, name, params, std::move(body_bnd));
  }

void Block::execute_as_function(Reference &self_io, Global_context &global, const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params, rocket::cow_vector<Reference> &&args) const
  {
    Executive_context ctx_next(nullptr);
    ctx_next.initialize_for_function(loc, name, params, std::move(self_io), std::move(args));
    // Execute the body.
    const auto status = this->execute_in_place(self_io, ctx_next, global);
    switch(status) {
      case status_next: {
        // Return `null` if the control flow reached the end of the function.
        self_io = Reference_root::S_null();
        // Fallthrough.
      case status_return:
        return;
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
      default: {
        ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
      }
    }
  }

void Block::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &stmt : this->m_stmts) {
      stmt.enumerate_variables(callback);
    }
  }

}
