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
    Cow_Vector<Backtrace_Frame> m_frames;

  public:
    Exception() noexcept
      {
      }
    explicit Exception(const std::exception& other)
      {
        this->dynamic_copy(other);
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
    std::size_t count_frames() const noexcept
      {
        return this->m_frames.size();
      }
    const Backtrace_Frame& get_frame(std::size_t i) const
      {
        return this->m_frames.at(i);
      }
    template<typename... ParamsT> Backtrace_Frame& push_frame(ParamsT&&... params)
      {
        return this->m_frames.emplace_back(rocket::forward<ParamsT>(params)...);
      }

    // Note that if `other` is not derived from `Exception`, this function copies `other.what()`,
    // which is subjecjt to memory allocation failures.
    Exception& dynamic_copy(const std::exception& other);
  };

inline std::ostream& operator<<(std::ostream& os, const Exception& except)
  {
    return os << except.get_value();
  }

}  // namespace Asteria

#endif
