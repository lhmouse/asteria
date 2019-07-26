// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Air_Queue::do_clear_nodes() const noexcept
  {
    auto next = this->m_stor.head;
    while(ROCKET_EXPECT(next)) {
      auto qnode = std::exchange(next, next->m_next);
      // Destroy and deallocate the node.
      delete qnode;
    }
  }

Air_Node::Status Air_Queue::execute(Executive_Context& ctx) const
  {
    auto next = this->m_stor.head;
    while(ROCKET_EXPECT(next)) {
      auto qnode = std::exchange(next, next->m_next);
      // Execute this node and return any status code unexpected to the caller verbatim.
      auto status = qnode->execute(ctx);
      if(ROCKET_UNEXPECT(status != Air_Node::status_next)) {
        return status;
      }
    }
    return Air_Node::status_next;
  }

void Air_Queue::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    auto next = this->m_stor.head;
    while(ROCKET_EXPECT(next)) {
      auto qnode = std::exchange(next, next->m_next);
      // Enumerate varables in this node recursively.
      qnode->enumerate_variables(callback);
    }
  }

}  // namespace Asteria
