// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RUNTIME_ERROR_HPP_
#define ASTERIA_RUNTIME_RUNTIME_ERROR_HPP_

#include "../fwd.hpp"
#include "backtrace_frame.hpp"
#include <exception>

namespace asteria {

class Runtime_Error
  : public virtual exception
  {
  public:
    enum class F_native  { };
    enum class F_throw   { };
    enum class F_assert  { };

  private:
    Value m_value;
    cow_vector<Backtrace_Frame> m_frames;
    size_t m_ipos = 0;  // where to insert new frames

    cow_string m_what;  // a comprehensive string that is human-readable.

  public:
    Runtime_Error(F_native, const exception& stdex)
      : m_value(cow_string(stdex.what()))
      { this->do_backtrace(),
        this->do_insert_frame(frame_type_native, nullopt, this->m_value);  }

    template<typename XValT,
    ROCKET_ENABLE_IF(::std::is_constructible<Value, XValT&&>::value)>
    Runtime_Error(F_throw, XValT&& xval, const Source_Location& sloc)
      : m_value(::std::forward<XValT>(xval))
      { this->do_backtrace(),
        this->do_insert_frame(frame_type_throw, sloc, this->m_value);  }

    Runtime_Error(F_assert, const Source_Location& sloc, const cow_string& msg)
      : m_value("assertion failure: " + msg)
      { this->do_backtrace(),
        this->do_insert_frame(frame_type_assert, sloc, this->m_value);  }

    ASTERIA_COPYABLE_DESTRUCTOR(Runtime_Error);

  private:
    void
    do_backtrace();

    void
    do_compose_message();

    template<typename... ParamsT>
    void
    do_insert_frame(ParamsT&&... params)
      {
        // Insert the frame. Note exception safety.
        size_t ipos = this->m_ipos;
        this->m_frames.insert(ipos, Backtrace_Frame(::std::forward<ParamsT>(params)...));
        this->m_ipos = ipos + 1;

        // Rebuild the message using new frames.
        this->do_compose_message();
      }

  public:
    const char*
    what()
    const noexcept override
      { return this->m_what.c_str();  }

    const Value&
    value()
    const noexcept
      { return this->m_value;  }

    size_t
    count_frames()
    const noexcept
      { return this->m_frames.size();  }

    const Backtrace_Frame&
    frame(size_t index)
    const
      { return this->m_frames.at(index);  }

    template<typename XValT>
    Runtime_Error&
    push_frame_throw(const Source_Location& sloc, XValT&& xval)
      {
        // Start a new backtrace.
        this->m_value = ::std::forward<XValT>(xval);
        this->m_ipos = 0;

        // Append the first frame to the current backtrace.
        this->do_insert_frame(frame_type_throw, sloc, this->m_value);
        return *this;
      }

    template<typename XValT>
    Runtime_Error&
    push_frame_catch(const Source_Location& sloc, XValT&& xval)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_catch, sloc, ::std::forward<XValT>(xval));
        return *this;
      }

    Runtime_Error&
    push_frame_plain(const Source_Location& sloc, const cow_string& remarks)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_plain, sloc, remarks);
        return *this;
      }

    Runtime_Error&
    push_frame_func(const Source_Location& sloc, const cow_string& func)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_func, sloc, func);
        return *this;
      }

    Runtime_Error&
    push_frame_defer(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_defer, sloc, this->m_value);
        return *this;
      }

    Runtime_Error&
    push_frame_try(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_try, sloc, this->m_value);
        return *this;
      }
  };

#define ASTERIA_RUNTIME_TRY           try {  \
                                        try
                                          // Perform operations that might throw
                                          // exceptions here.
#define ASTERIA_RUNTIME_CATCH(...)      catch(::asteria::Runtime_Error&)  \
                                          { throw;  }  \
                                        catch(::std::exception& zTrSrvgK_)  \
                                          { throw ::asteria::Runtime_Error(  \
                                                ::asteria::Runtime_Error::F_native(),  \
                                                zTrSrvgK_); }  \
                                      }  \
                                      catch(__VA_ARGS__)

}  // namespace asteria

#endif
