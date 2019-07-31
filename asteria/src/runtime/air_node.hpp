// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Air_Node
  {
  public:
    enum Status : uint8_t
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
    constexpr Air_Node() noexcept
      {
      }
    virtual ~Air_Node();

  public:
    virtual Status execute(Executive_Context& ctx) const = 0;
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;
  };

}  // namespace Asteria

#endif
