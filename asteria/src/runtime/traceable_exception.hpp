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
    Traceable_Exception(Value &&value, const Source_Location &sloc, const CoW_String &func)
      {
        this->m_value = std::move(value);
        this->m_frames.emplace_back(sloc, func);
      }
    Traceable_Exception(std::exception &&except, const Source_Location &sloc, const CoW_String &func)
      {
        const auto other = dynamic_cast<Traceable_Exception *>(std::addressof(except));
        if(other) {
          this->m_value = std::move(other->m_value);
          this->m_frames.assign(std::move(other->m_frames));
        } else {
          this->m_value = D_string(except.what());
          this->m_frames.emplace_back(rocket::sref("<native code>"), 0, rocket::sref("<native code>"));
        }
        this->m_frames.emplace_back(sloc, func);
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

}

#endif
