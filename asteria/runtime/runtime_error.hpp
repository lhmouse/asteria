// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RUNTIME_ERROR_
#define ASTERIA_RUNTIME_RUNTIME_ERROR_

#include "../fwd.hpp"
#include "backtrace_frame.hpp"
#include "../../rocket/tinyfmt_str.hpp"
#include <exception>
namespace asteria {

class Runtime_Error
  : public virtual exception
  {
  public:
    enum class M_throw;
    enum class M_assert;
    enum class M_native;

  private:
    Value m_value;
    cow_vector<Backtrace_Frame> m_frames;
    size_t m_ins_at = 0;  // where to insert new frames

    ::rocket::tinyfmt_str m_fmt;  // human-readable message

  public:
    template<typename XValT>
    explicit
    Runtime_Error(M_throw, XValT&& xval, const Source_Location& sloc)
      : m_value(::std::forward<XValT>(xval))
      { this->do_backtrace({ frame_type_throw, sloc, this->m_value });  }

    explicit
    Runtime_Error(M_assert, const Source_Location& sloc, stringR msg)
      : m_value("assertion failure: " + msg)
      { this->do_backtrace({ frame_type_assert, sloc, this->m_value });  }

    explicit
    Runtime_Error(M_native, stringR msg)
      : m_value(msg)
      { this->do_backtrace({ frame_type_native, { }, this->m_value });  }

  private:
    void
    do_backtrace(Backtrace_Frame&& new_frm);

    void
    do_insert_frame(Backtrace_Frame&& new_frm);

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

    const Backtrace_Frame&
    frame(size_t index) const
      { return this->m_frames.at(index);  }

    template<typename XValT>
    void
    push_frame_throw(const Source_Location& sloc, XValT&& xval)
      {
        // Start a new backtrace.
        this->m_value = ::std::forward<XValT>(xval);
        this->m_ins_at = 0;

        // Append the first frame to the current backtrace.
        this->do_insert_frame({ frame_type_throw, sloc, this->m_value });
      }

    template<typename XValT>
    void
    push_frame_catch(const Source_Location& sloc, XValT&& xval)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame({ frame_type_catch, sloc, ::std::forward<XValT>(xval) });

        // This means an exception was thrown again from a `catch` block.
        // Subsequent frames shoud be appended to its parent.
        this->m_ins_at = this->m_frames.size();
      }

    void
    push_frame_plain(const Source_Location& sloc, stringR remarks)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame({ frame_type_plain, sloc, remarks });
      }

    void
    push_frame_func(const Source_Location& sloc, stringR func)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame({ frame_type_func, sloc, func });
      }

    void
    push_frame_defer(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame({ frame_type_defer, sloc, this->m_value });

        // This means an exception was thrown again during execution of deferred
        // expression. Subsequent frames shoud be appended to its parent.
        this->m_ins_at = this->m_frames.size();
      }

    void
    push_frame_try(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame({ frame_type_try, sloc, this->m_value });
      }
  };

// Note the string template must be parenthesized.
#define ASTERIA_THROW_RUNTIME_ERROR(TEMPLATE, ...)  \
    throw ::asteria::Runtime_Error(  \
        ::asteria::Runtime_Error::M_native(),  \
        ::asteria::format_string(  \
          (::asteria::make_string_template TEMPLATE) ,##__VA_ARGS__))

}  // namespace asteria
#endif
