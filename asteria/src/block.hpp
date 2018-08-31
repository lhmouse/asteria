// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BLOCK_HPP_
#define ASTERIA_BLOCK_HPP_

#include "fwd.hpp"

namespace Asteria {

class Block
  {
  public:
    enum Status : std::uint8_t
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
    Block(Vector<Statement> &&stmts) noexcept;
    ~Block();

    Block(Block &&) noexcept;
    Block & operator=(Block &&) noexcept;

  public:
    bool empty() const noexcept;

    void fly_over_in_place(Abstract_context &ctx_inout) const;
    Block bind_in_place(Analytic_context &ctx_inout) const;
    Status execute_in_place(Reference &ref_out, Executive_context &ctx_inout) const;

    Block bind(const Analytic_context &ctx) const;
    Status execute(Reference &ref_out, const Executive_context &ctx) const;
  };

}

#endif
