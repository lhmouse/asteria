// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Air_Node : public virtual RefCnt_Base
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

  public:
    Air_Node() noexcept
      {
      }
    ~Air_Node() override;

  public:
    virtual Status execute(Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback &callback) const = 0;
  };

}

#endif
