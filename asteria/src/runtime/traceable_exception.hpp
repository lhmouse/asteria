// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_
#define ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "backtrace_frame.hpp"

namespace Asteria {

class Traceable_Exception : public virtual std::exception
  {
  private:
    Value m_value;
    CoW_Vector<Backtrace_Frame> m_frames;

  public:
    template<typename XvalueT> Traceable_Exception(XvalueT &&xvalue,
                                                   const Source_Location &sloc, const CoW_String &func)
      : m_value(std::forward<XvalueT>(xvalue)),
        m_frames(1, sloc, func)
      {
      }
    ~Traceable_Exception() override;

  public:
    const char * what() const noexcept override
      {
        return "Asteria::Traceable_Exception";
      }

    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & mut_value() noexcept
      {
        return this->m_value;
      }

    std::size_t get_frame_count() const noexcept
      {
        return this->m_frames.size();
      }
    const Backtrace_Frame & get_frame(std::size_t index) const
      {
        return this->m_frames.at(index);
      }
    void append_frame(const Source_Location &sloc, const CoW_String &func)
      {
        this->m_frames.emplace_back(sloc, func);
      }
  };

template<typename ExceptionT> Traceable_Exception trace_exception(ExceptionT &&except)
  {
    // Is `except` an lvalue reference or a const reference (i.e. it isn't a non-const rvalue reference)?
    constexpr bool copy_or_move = std::is_lvalue_reference<ExceptionT &&>::value || std::is_const<typename std::remove_reference<ExceptionT &&>::type>::value;
    // Is `except` derived from `Traceable_Exception`?
    const auto traceable = dynamic_cast<typename std::conditional<copy_or_move, const Traceable_Exception *, Traceable_Exception *>::type>(std::addressof(except));
    if(!traceable) {
      // Say the exception was thrown from native code.
      return Traceable_Exception(D_string(except.what()), Source_Location(rocket::sref("<native code>"), 0), rocket::sref("<native code>"));
    }
    // Copy or move it.
    return static_cast<typename std::conditional<copy_or_move, const Traceable_Exception &, Traceable_Exception &&>::type>(*traceable);
  }

}

#endif
