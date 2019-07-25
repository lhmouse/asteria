// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Air_Queue::do_clear_nodes() noexcept
  {
    auto ptr = this->m_stor.head;
    while(ROCKET_EXPECT(ptr)) {
      delete std::exchange(ptr, ptr->m_next);
    }
  }

Air_Node::Status Air_Queue::execute(Executive_Context& ctx) const
  {
    auto status = Air_Node::status_next;
    auto ptr = this->m_stor.head;
    while(ROCKET_EXPECT(ptr && (status == Air_Node::status_next))) {
      status = std::exchange(ptr, ptr->m_next)->execute(ctx);
    }
    return status;
  }

void Air_Queue::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    auto ptr = this->m_stor.head;
    while(ROCKET_EXPECT(ptr)) {
      std::exchange(ptr, ptr->m_next)->enumerate_variables(callback);
    }
  }

}  // namespace Asteria
