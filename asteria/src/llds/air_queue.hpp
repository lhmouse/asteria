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
    struct Storage
      {
        Air_Node* head;  // first element (or null if empty)
        Air_Node* tail;  // last element (or null if empty)
      };
    Storage m_stor;

  public:
    constexpr Air_Queue() noexcept
      : m_stor()
      {
      }
    Air_Queue(Air_Queue&& other) noexcept
      : m_stor()
      {
        std::swap(this->m_stor, other.m_stor);
      }
    Air_Queue& operator=(Air_Queue&& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
        return *this;
      }
    ~Air_Queue()
      {
        if(this->m_stor.head) {
          this->do_clear_nodes();
        }
#ifdef ROCKET_DEBUG
        std::memset(std::addressof(this->m_stor), 0xF9, sizeof(this->m_stor));
#endif
      }

  private:
    void do_clear_nodes() const noexcept;

  public:
    bool empty() const noexcept
      {
        return this->m_stor.head == nullptr;
      }
    void clear() noexcept
      {
        if(this->m_stor.head) {
          this->do_clear_nodes();
        }
        // Clean invalid data up.
        this->m_stor.head = nullptr;
        this->m_stor.tail = nullptr;
      }

    void swap(Air_Queue& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
      }

    template<typename XnodeT, typename... ParamsT> XnodeT& push(ParamsT&&... params)
      {
        // Allocate a new node.
        auto qnode = new XnodeT(rocket::forward<ParamsT>(params)...);
        // Append it to the end. Now the node is owned by `*this`.
        auto tail = std::exchange(this->m_stor.tail, qnode);
        (tail ? tail->m_nx : this->m_stor.head) = qnode;
        // Return a reference to the node.
        return *qnode;
      }

    void execute(Air_Node::Status& status, Executive_Context& ctx) const;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Air_Queue& lhs, Air_Queue& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
