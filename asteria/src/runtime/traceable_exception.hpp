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
    Cow_Vector<Backtrace_Frame> m_frames;

  public:
    template<typename XvalueT> Traceable_Exception(XvalueT&& xvalue,
                                                   const Source_Location& sloc, const Cow_String& func)
      : m_value(rocket::forward<XvalueT>(xvalue)),
        m_frames(1, sloc, func)
      {
      }
    ~Traceable_Exception() override;

  public:
    const char* what() const noexcept override
      {
        return "Asteria::Traceable_Exception";
      }

    const Value& value() const noexcept
      {
        return this->m_value;
      }
    Value& mut_value() noexcept
      {
        return this->m_value;
      }

    std::size_t frame_count() const noexcept
      {
        return this->m_frames.size();
      }
    const Backtrace_Frame& frame(std::size_t index) const
      {
        return this->m_frames.at(index);
      }
    void append_frame(const Source_Location& sloc, const Cow_String& func)
      {
        this->m_frames.emplace_back(sloc, func);
      }
  };

template<typename ExceptionT> Traceable_Exception trace_exception(ExceptionT&& except)
  {
    // Should `except` be copied or moved?
    using Source = typename std::conditional<(std::is_lvalue_reference<ExceptionT>::value || std::is_const<ExceptionT>::value),
                                             const Traceable_Exception&,    // lvalue or const rvalue
                                             Traceable_Exception&&>::type;  // non-const rvalue
    // Is the dynamic type of `except` derived from `Traceable_Exception`?
    auto qsource = dynamic_cast<typename std::remove_reference<Source>::type*>(std::addressof(except));
    if(!qsource) {
      // Say the exception was thrown from native code.
      return Traceable_Exception(G_string(except.what()),
                                 Source_Location(rocket::sref("<native code>"), 0),
                                 rocket::sref("<native code>"));
    }
    // Forward `except`.
    return static_cast<Source>(*qsource);
  }

}  // namespace Asteria

#endif
