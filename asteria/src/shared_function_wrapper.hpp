// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SHARED_FUNCTION_WRAPPER_HPP_
#define ASTERIA_SHARED_FUNCTION_WRAPPER_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"
#include "abstract_function.hpp"

namespace Asteria {

class Shared_function_wrapper
  {
  private:
    rocket::refcounted_ptr<Abstract_function> m_ptr;

  public:
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_function, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit Shared_function_wrapper(ElementT &&elem)
        : m_ptr(rocket::make_refcounted<typename std::decay<ElementT>::type>(std::forward<ElementT>(elem)))
        {
        }

  public:
    const Abstract_function * get() const noexcept
      {
        return this->m_ptr.get();
      }
    const Abstract_function & operator*() const noexcept
      {
        return *(this->get());
      }
    const Abstract_function * operator->() const noexcept
      {
        return this->get();
      }
  };

}

#endif
