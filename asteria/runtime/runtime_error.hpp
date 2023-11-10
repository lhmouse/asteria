// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RUNTIME_ERROR_
#define ASTERIA_RUNTIME_RUNTIME_ERROR_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../value.hpp"
#include "../../rocket/tinyfmt_str.hpp"
#include <exception>
namespace asteria {

class Runtime_Error
  :
    public virtual exception
  {
  public:
    struct Frame
      {
        Frame_Type type;
        Source_Location sloc;
        Value value;
      };

    enum class M_throw;
    enum class M_assert;
    enum class M_format;

  private:
    Value m_value;
    cow_vector<Frame> m_frames;
    size_t m_frame_ins = 0;
    ::rocket::tinyfmt_str m_tempf;

    ::rocket::tinyfmt_str m_fmt;  // human-readable message

  public:
    template<typename XValT>
    explicit
    Runtime_Error(M_throw, XValT&& xval, const Source_Location& xsloc)
      :
        m_value(::std::forward<XValT>(xval))
      {
        this->do_backtrace();
        this->do_insert_frame(frame_type_throw, &xsloc, this->m_value);
      }

    explicit
    Runtime_Error(M_assert, const Source_Location& xsloc, stringR msg)
      :
        m_value("assertion failure: " + msg)
      {
        this->do_backtrace();
        this->do_insert_frame(frame_type_assert, &xsloc, this->m_value);
      }

    template<typename... ParamsT>
    explicit
    Runtime_Error(M_format, const char* templ, const ParamsT&... params)
      :
        m_value()
      {
        format(this->m_fmt, templ, params...);
        this->m_value = this->m_fmt.extract_string();

        this->do_backtrace();
        this->do_insert_frame(frame_type_native, nullptr, this->m_value);
      }

  private:
    void
    do_backtrace();

    void
    do_insert_frame(Frame_Type type, const Source_Location* sloc_opt, const Value& val);

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Runtime_Error);

    const char*
    what() const noexcept override
      { return this->m_fmt.c_str();  }

    const Value&
    value() const noexcept
      { return this->m_value;  }

    size_t
    count_frames() const noexcept
      { return this->m_frames.size();  }

    const Frame&
    frame(size_t index) const
      { return this->m_frames.at(index);  }

    void
    push_frame_catch(const Source_Location& sloc, const Value& val)
      {
        this->do_insert_frame(frame_type_catch, &sloc, val);
        this->m_frame_ins = this->m_frames.size();
      }

    void
    push_frame_defer(const Source_Location& sloc)
      {
        this->do_insert_frame(frame_type_defer, &sloc, this->m_value);
        this->m_frame_ins = this->m_frames.size();
      }

    void
    push_frame_plain(const Source_Location& sloc, stringR remarks = cow_string())
      {
        this->do_insert_frame(frame_type_plain, &sloc, remarks);
      }

    void
    push_frame_function(const Source_Location& sloc, stringR name)
      {
        this->do_insert_frame(frame_type_func, &sloc, name);
      }

    void
    push_frame_try(const Source_Location& sloc)
      {
        this->do_insert_frame(frame_type_try, &sloc, this->m_value);
      }
  };

}  // namespace asteria
#endif
