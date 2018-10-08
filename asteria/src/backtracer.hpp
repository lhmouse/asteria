// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BACKTRACER_HPP_
#define ASTERIA_BACKTRACER_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

class Backtracer : public virtual std::exception, public virtual std::nested_exception
  {
  public:
    [[noreturn]] static void unpack_and_rethrow(Vector<Backtracer> &btv_out, std::exception_ptr etop);

  private:
    String m_file;
    Uint32 m_line;
    String m_func;

  public:
    Backtracer(String xfile, Uint32 xline, String xfunc) noexcept
      : m_file(std::move(xfile)), m_line(xline), m_func(std::move(xfunc))
      {
      }
    ~Backtracer();

  public:
    const String & file() const noexcept
      {
        return this->m_file;
      }
    Uint32 line() const noexcept
      {
        return this->m_line;
      }
    const String & func() const noexcept
      {
        return this->m_func;
      }

    const char * what() const noexcept override;
  };

}

#endif
