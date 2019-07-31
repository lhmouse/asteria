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
      auto node = std::exchange(next, next->m_next);
      // Destroy and deallocate the node.
      delete node;
    }
  }

void Air_Queue::execute(Air_Node::Status& status, Executive_Context& ctx) const
  {
    auto next = this->m_stor.head;
    while(ROCKET_EXPECT(next)) {
      auto node = std::exchange(next, next->m_next);
      // Execute this node and return any status code unexpected to the caller verbatim.
      status = node->execute(ctx);
      if(ROCKET_UNEXPECT(status != Air_Node::status_next)) {
        return;
      }
    }
  }

void Air_Queue::enumerate_variables(Variable_Callback& callback) const
  {
    auto next = this->m_stor.head;
    while(ROCKET_EXPECT(next)) {
      auto node = std::exchange(next, next->m_next);
      // Enumerate varables in this node recursively.
      node->enumerate_variables(callback);
    }
  }

}  // namespace Asteria
