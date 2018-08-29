// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ERROR_HPP_
#define ASTERIA_RUNTIME_ERROR_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

class Runtime_error : public virtual std::exception
  {
  private:
    String m_msg;

  public:
    explicit Runtime_error(String msg) noexcept
      : m_msg(std::move(msg))
      {
      }
    ~Runtime_error();

  public:
    const char * what() const noexcept override;
  };

}

#endif
