// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_
#define ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Traceable_Exception : public virtual std::exception
  {
  private:
    Source_Location m_loc;
    Value m_value;
    CoW_Vector<Source_Location> m_backtrace;

  public:
    Traceable_Exception(const Source_Location &loc, Value value)
      : m_loc(loc), m_value(std::move(value))
      {
      }
    explicit Traceable_Exception(const std::exception &stdex)
      : m_loc(rocket::sref("<native code>"), 0), m_value(D_string(stdex.what()))
      {
      }
    ~Traceable_Exception() override;

  public:
    const char * what() const noexcept override
      {
        return "Asteria::Traceable_Exception";
      }

    const Source_Location & get_location() const noexcept
      {
        return this->m_loc;
      }
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }

    const CoW_Vector<Source_Location> & get_backtrace() const noexcept
      {
        return this->m_backtrace;
      }
    void append_backtrace(const Source_Location &loc)
      {
        this->m_backtrace.emplace_back(loc);
      }
  };

}

#endif
