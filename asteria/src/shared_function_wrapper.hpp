// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SHARED_FUNCTION_WRAPPER_HPP_
#define ASTERIA_SHARED_FUNCTION_WRAPPER_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "rocket/refcounted_ptr.hpp"
#include <functional>

namespace Asteria {

class Shared_function_wrapper
  {
  private:
    rocket::refcounted_ptr<Abstract_function> m_owns;
    std::reference_wrapper<const Abstract_function> m_ref;

  public:
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_function, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit constexpr Shared_function_wrapper(std::reference_wrapper<ElementT> elem) noexcept
      : m_owns(nullptr), m_ref(std::move(elem))
      {
      }
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_function, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit Shared_function_wrapper(ElementT &&elem)
      : m_owns(rocket::make_refcounted<typename std::decay<ElementT>::type>(std::forward<ElementT>(elem))), m_ref(*(this->m_owns))
      {
      }

  public:
    const Abstract_function & get() const noexcept
      {
        return this->m_ref;
      }
    const Abstract_function & operator*() const noexcept
      {
        return this->m_ref;
      }
    const Abstract_function * operator->() const noexcept
      {
        return std::addressof(this->m_ref.get());
      }
  };

}

#endif
