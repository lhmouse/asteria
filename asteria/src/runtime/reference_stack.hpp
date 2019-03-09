// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_STACK_HPP_
#define ASTERIA_RUNTIME_REFERENCE_STACK_HPP_

#include "../fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Reference_Stack
  {
  private:
    Cow_Vector<Reference> m_stor;
    Reference *m_btop;  // This points to the next element of the top.

  public:
    Reference_Stack() noexcept
      : m_stor(), m_btop(this->m_stor.mut_data())
      {
      }
    ~Reference_Stack();

    Reference_Stack(const Reference_Stack &)
      = delete;
    Reference_Stack & operator=(const Reference_Stack &)
      = delete;

  public:
    bool empty() const noexcept
      {
        return this->m_btop == this->m_stor.data();
      }
    std::size_t size() const noexcept
      {
        return static_cast<std::size_t>(this->m_btop - this->m_stor.data());
      }
    void clear() noexcept
      {
        this->m_btop = this->m_stor.mut_data();
      }

    const Reference & get(std::size_t index) const noexcept
      {
        ROCKET_ASSERT(index < this->size());
        return this->m_btop[~index];
      }
    Reference & mut(std::size_t index) noexcept
      {
        ROCKET_ASSERT(index < this->size());
        return this->m_btop[~index];
      }
    template<typename ParamT> Reference & push(ParamT &&param)
      {
        auto btop = this->m_btop;
        if(ROCKET_EXPECT(btop != this->m_stor.data() + this->m_stor.size())) {
          // Overwrite an existent element.
          *btop = std::forward<ParamT>(param);
        } else {
          // Construct a new element.
          btop = std::addressof(this->m_stor.emplace_back(std::forward<ParamT>(param)));
        }
        // Set up the past-the-top pointer.
        ++btop;
        this->m_btop = btop;
        return btop[-1];
      }
    void pop() noexcept
      {
        auto btop = this->m_btop;
        ROCKET_ASSERT(btop != this->m_stor.data());
        // Set up the past-the-top pointer.
        --btop;
        this->m_btop = btop;
      }
  };

}  // namespace Asteria

#endif
