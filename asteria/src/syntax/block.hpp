// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_BLOCK_HPP_
#define ASTERIA_SYNTAX_BLOCK_HPP_

#include "../fwd.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/bind_first.hpp"
#include "../rocket/refcnt_object.hpp"

namespace Asteria {

class Block
  {
  public:
    enum Status : std::uint8_t
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
    using Compiled_Instruction = rocket::binder_first<Status (*)(const void *, Reference &, Executive_Context &, Global_Context &),
                                                      const void *>;

  private:
    rocket::cow_vector<Statement> m_stmts;
    rocket::cow_vector<Compiled_Instruction> m_cinsts;

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

    void fly_over_in_place(Abstract_Context &ctx_io) const;
    Block bind_in_place(Analytic_Context &ctx_io, const Global_Context &global) const;
    Status execute_in_place(Reference &ref_out, Executive_Context &ctx_io, Global_Context &global) const;

    Block bind(const Global_Context &global, const Analytic_Context &ctx) const;
    Status execute(Reference &ref_out, Global_Context &global, const Executive_Context &ctx) const;

    Instantiated_Function instantiate_function(Global_Context &global, const Executive_Context &ctx, const Source_Location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params) const;
    void execute_as_function(Reference &self_io, Global_Context &global, const rocket::refcnt_object<Variadic_Arguer> &zvarg, const rocket::cow_vector<rocket::prehashed_string> &params, rocket::cow_vector<Reference> &&args) const;

    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
