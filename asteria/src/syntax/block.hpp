// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_BLOCK_HPP_
#define ASTERIA_SYNTAX_BLOCK_HPP_

#include "../fwd.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/bind_first.hpp"
#include "../rocket/refcounted_object.hpp"

namespace Asteria {

class Block
  {
  public:
    enum Status : std::size_t
      {
        status_next             = 0,
        status_return           = 1,
        status_break_unspec     = 2,
        status_break_switch     = 3,
        status_break_while      = 4,
        status_break_for        = 5,
        status_continue_unspec  = 6,
        status_continue_while   = 7,
        status_continue_for     = 8,
      };

    // TODO: In the future we will add JIT support.
    using Compiled_instruction = rocket::binder_first<Status (*)(const void *, Reference &, Executive_context &, Global_context &), const void *>;

  private:
    rocket::cow_vector<Statement> m_stmts;
    rocket::cow_vector<Compiled_instruction> m_cinsts;

  public:
    Block() noexcept
      : m_stmts()
      {
      }
    Block(rocket::cow_vector<Statement> &&stmts) noexcept
      : m_stmts(std::move(stmts))
      {
        this->do_compile();
      }
    ROCKET_COPYABLE_DESTRUCTOR(Block);

  private:
    void do_compile();

  public:
    bool empty() const noexcept
      {
        return this->m_stmts.empty();
      }

    void fly_over_in_place(Abstract_context &ctx_io) const;
    Block bind_in_place(Analytic_context &ctx_io, const Global_context &global) const;
    Status execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context &global) const;

    Block bind(const Global_context &global, const Analytic_context &ctx) const;
    Status execute(Reference &ref_out, Global_context &global, const Executive_context &ctx) const;

    Instantiated_function instantiate_function(Global_context &global, const Executive_context &ctx, const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params) const;
    void execute_as_function(Reference &self_io, Global_context &global, const rocket::refcounted_object<Variadic_arguer> &zvarg, const rocket::cow_vector<rocket::prehashed_string> &params, rocket::cow_vector<Reference> &&args) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
