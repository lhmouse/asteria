// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Air_Queue::do_clear_nodes() noexcept
  {
    auto qnode = this->m_stor.head;
    while(ROCKET_EXPECT(qnode)) {
      delete std::exchange(qnode, qnode->m_next);
    }
  }

Air_Node::Status Air_Queue::execute(Executive_Context& ctx) const
  {
    auto status = Air_Node::status_next;
    auto qnode = this->m_stor.head;
    while(ROCKET_EXPECT(qnode && (status == Air_Node::status_next))) {
      status = std::exchange(qnode, qnode->m_next)->execute(ctx);
    }
    return status;
  }

void Air_Queue::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    auto qnode = this->m_stor.head;
    while(ROCKET_EXPECT(qnode)) {
      std::exchange(qnode, qnode->m_next)->enumerate_variables(callback);
    }
  }

}  // namespace Asteria
