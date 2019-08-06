// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXCEPTION_HPP_
#define ASTERIA_RUNTIME_EXCEPTION_HPP_

#include "../fwd.hpp"
#include "backtrace_frame.hpp"
#include <exception>

namespace Asteria {

class Exception : public std::exception
  {
  private:
    Value m_value;
    cow_vector<Backtrace_Frame> m_frames;

  public:
    template<typename XvalueT, ASTERIA_SFINAE_CONSTRUCT(Value, XvalueT&&)> Exception(const Source_Location& sloc, XvalueT&& xvalue)
      : m_value(rocket::forward<XvalueT>(xvalue)),
        m_frames(1, Backtrace_Frame::ftype_throw, sloc, this->m_value)
      {
      }
    explicit Exception(const std::exception& stdex)
      : m_value(G_string(stdex.what())),
        m_frames(1, Backtrace_Frame::ftype_native, Source_Location(rocket::sref("<native code>"), -1), this->m_value)
      {
      }
    ~Exception() override;

  public:
    const char* what() const noexcept override
      {
        return "Asteria::Exception";
      }

    const Value& get_value() const noexcept
      {
        return this->m_value;
      }
    Value& open_value()
      {
        return this->m_value;
      }
    size_t count_frames() const noexcept
      {
        return this->m_frames.size();
      }
    const Backtrace_Frame& get_frame(size_t i) const
      {
        return this->m_frames.at(i);
      }
    template<typename XvalueT> Backtrace_Frame& push_frame_throw(const Source_Location& sloc, XvalueT&& xvalue)
      {
        return this->m_frames.emplace_back(Backtrace_Frame::ftype_throw, sloc, this->m_value = rocket::forward<XvalueT>(xvalue));
      }
    Backtrace_Frame& push_frame_catch(const Source_Location& sloc)
      {
        return this->m_frames.emplace_back(Backtrace_Frame::ftype_catch, sloc, this->m_value);
      }
    Backtrace_Frame& push_frame_func(const Source_Location& sloc, const G_string& func)
      {
        return this->m_frames.emplace_back(Backtrace_Frame::ftype_func, sloc, func);
      }
  };

inline std::ostream& operator<<(std::ostream& ostrm, const Exception& except)
  {
    return ostrm << except.get_value();
  }

}  // namespace Asteria

#endif
