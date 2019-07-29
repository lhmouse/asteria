// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Air_Node
  {
    friend Air_Queue;

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

  private:
    Air_Node* m_next;  // pointer to the next node in the queue
    Air_Node* m_prev;  // pointer to the previous node in the queue

  public:
    constexpr Air_Node() noexcept
      : m_next(), m_prev()
      {
      }
    virtual ~Air_Node();

  public:
    virtual Status execute(Executive_Context& ctx) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback& callback) const = 0;
  };

}  // namespace Asteria

#endif
