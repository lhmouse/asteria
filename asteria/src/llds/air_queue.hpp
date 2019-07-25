// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AIR_QUEUE_HPP_
#define ASTERIA_LLDS_AIR_QUEUE_HPP_

#include "../fwd.hpp"
#include "../runtime/air_node.hpp"

namespace Asteria {

class Air_Queue
  {
  private:
    const Air_Node* m_head;
    Air_Node* m_tail;

  public:
    constexpr Air_Queue() noexcept
      : m_head(nullptr),
        m_tail(nullptr)
      {
      }
    Air_Queue(Air_Queue&& other) noexcept
      : m_head(std::exchange(other.m_head, nullptr)),
        m_tail(std::exchange(other.m_tail, nullptr))
      {
      }
    Air_Queue& operator=(Air_Queue&& other) noexcept
      {
        std::swap(this->m_head, other.m_head);
        std::swap(this->m_tail, other.m_tail);
        return *this;
      }
    ~Air_Queue();

  public:
    constexpr bool empty() const noexcept
      {
        return this->m_head == nullptr;
      }
    template<typename XnodeT, typename... ParamsT> XnodeT& push(ParamsT&&... params)
      {
        // Allocate a new node.
        auto ptr = new XnodeT(rocket::forward<ParamsT>(params)...);
        // Append it to the end. Now the node is owned by `*this`.
        auto tail = std::exchange(this->m_tail, ptr);
        (tail ? tail->m_next : this->m_head) = ptr;
        // Return a reference to the node.
        return *ptr;
      }

    Air_Node::Status execute(Executive_Context& ctx) const;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

}

#endif
