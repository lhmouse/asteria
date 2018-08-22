// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BACKTRACER_HPP_
#define ASTERIA_BACKTRACER_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria
{

class Backtracer
  : public virtual std::nested_exception
  {
  private:
    String m_file;
    Unsigned m_line;

  public:
    Backtracer(String file, Unsigned line) noexcept
      : m_file(std::move(file)), m_line(line)
      {
      }
    Backtracer(const Backtracer &) noexcept;
    Backtracer & operator=(const Backtracer &) noexcept;
    Backtracer(Backtracer &&) noexcept;
    Backtracer & operator=(Backtracer &&) noexcept;
    ~Backtracer();

  public:
    const String & get_file() const noexcept
      {
        return m_file;
      }
    Unsigned get_line() const noexcept
      {
        return m_line;
      }
  };

[[noreturn]] extern void unpack_backtrace_and_rethrow(Bivector<String, Unsigned> &btv_out, const std::exception_ptr &etop);

}

#endif
