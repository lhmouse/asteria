// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include "reference.hpp"
#include <exception>

namespace Asteria {

class Exception : public virtual std::exception
  {
  private:
    Reference m_ref;

  public:
    explicit Exception(Reference ref) noexcept
      : m_ref(std::move(ref))
      {
      }
    ~Exception();

  public:
    const char * what() const noexcept override;

    const Reference & get_reference() const noexcept
      {
        return this->m_ref;
      }
    Reference & get_reference() noexcept
      {
        return this->m_ref;
      }

    Reference & set_reference(Reference ref) noexcept;
  };

}

#endif
