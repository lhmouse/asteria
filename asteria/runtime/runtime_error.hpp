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

        Frame frm;
        frm.type = frame_type_throw;
        frm.sloc = xsloc;
        frm.value = this->m_value;
        this->do_insert_backtrace_frame(::std::move(frm));
      }

    explicit
    Runtime_Error(M_assert, const Source_Location& xsloc, stringR msg)
      :
        m_value("assertion failure: " + msg)
      {
        this->do_backtrace();

        Frame frm;
        frm.type = frame_type_assert;
        frm.sloc = xsloc;
        frm.value = this->m_value;
        this->do_insert_backtrace_frame(::std::move(frm));
      }

    template<typename... ParamsT>
    explicit
    Runtime_Error(M_format, const char* templ, const ParamsT&... params)
      :
        m_value()
      {
        this->do_backtrace();

        format(this->m_fmt, templ, params...);
        this->m_value = this->m_fmt.extract_string();

        Frame frm;
        frm.type = frame_type_native;
        frm.value = this->m_value;
        this->do_insert_backtrace_frame(::std::move(frm));
      }

  private:
    void
    do_backtrace();

    void
    do_insert_backtrace_frame(Frame&& frm);

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

    template<typename XValT>
    void
    push_frame_catch(const Source_Location& sloc, XValT&& xval)
      {
        Frame frm;
        frm.type = frame_type_catch;
        frm.sloc = sloc;
        frm.value = ::std::forward<XValT>(xval);
        this->do_insert_backtrace_frame(::std::move(frm));

        // This means an exception has been thrown again from a `catch` block.
        // Subsequent frames shoud be appended to its parent.
        this->m_frame_ins = this->m_frames.size();
      }

    void
    push_frame_defer(const Source_Location& sloc)
      {
        Frame frm;
        frm.type = frame_type_defer;
        frm.sloc = sloc;
        frm.value = this->m_value;
        this->do_insert_backtrace_frame(::std::move(frm));

        // This means an exception has been thrown again during execution of a
        // deferred expression. Subsequent frames shoud be appended to its parent.
        this->m_frame_ins = this->m_frames.size();
      }

    void
    push_frame_plain(const Source_Location& sloc, stringR remarks = cow_string())
      {
        Frame frm;
        frm.type = frame_type_catch;
        frm.sloc = sloc;
        frm.value = remarks;
        this->do_insert_backtrace_frame(::std::move(frm));
      }

    void
    push_frame_function(const Source_Location& sloc, stringR name)
      {
        Frame frm;
        frm.type = frame_type_func;
        frm.sloc = sloc;
        frm.value = name;
        this->do_insert_backtrace_frame(::std::move(frm));
      }

    void
    push_frame_try(const Source_Location& sloc)
      {
        Frame frm;
        frm.type = frame_type_try;
        frm.sloc = sloc;
        frm.value = this->m_value;
        this->do_insert_backtrace_frame(::std::move(frm));
      }
  };

}  // namespace asteria
#endif
