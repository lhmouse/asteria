// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RUNTIME_ERROR_HPP_
#define ASTERIA_RUNTIME_RUNTIME_ERROR_HPP_

#include "../fwd.hpp"
#include "backtrace_frame.hpp"
#include <exception>

namespace Asteria {

class Runtime_Error : public exception
  {
  private:
    Value m_value;
    cow_vector<Backtrace_Frame> m_frames;
    size_t m_ipos = 0;  // where to insert new frames
    // Create a comprehensive string that is also human-readable.
    cow_string m_what;

  public:
    template<typename XValT, ASTERIA_SFINAE_CONSTRUCT(Value, XValT&&)>
                                               Runtime_Error(const Source_Location& sloc, XValT&& xval)
      :
        m_value(::rocket::forward<XValT>(xval))
      {
        this->do_backtrace();
        this->do_insert_frame(frame_type_throw, sloc, this->m_value);
      }
    explicit Runtime_Error(const exception& stdex)
      :
        m_value(G_string(stdex.what()))
      {
        this->do_backtrace();
        this->do_insert_frame(frame_type_native, ::rocket::sref("<native code>"), -1, this->m_value);
      }
    ~Runtime_Error() override;

  private:
    void do_backtrace();
    void do_compose_message();

    template<typename... ParamsT> void do_insert_frame(ParamsT&&... params)
      {
        // Insert the frame.
        size_t ipos = this->m_ipos;
        this->m_frames.insert(ipos, Backtrace_Frame(::rocket::forward<ParamsT>(params)...));
        this->m_ipos = ipos + 1;
        // Rebuild the message using new frames.
        this->do_compose_message();
      }

  public:
    const char* what() const noexcept override
      {
        return this->m_what.c_str();
      }

    const Value& value() const noexcept
      {
        return this->m_value;
      }
    size_t count_frames() const noexcept
      {
        return this->m_frames.size();
      }
    const Backtrace_Frame& frame(size_t index) const
      {
        return this->m_frames.at(index);
      }

    template<typename XValT> Runtime_Error& push_frame_throw(const Source_Location& sloc, XValT&& xval)
      {
        // Start a new backtrace.
        this->m_value = ::rocket::forward<XValT>(xval);
        this->m_ipos = 0;
        // Append the first frame to the current backtrace.
        this->do_insert_frame(frame_type_throw, sloc, this->m_value);
        return *this;
      }
    Runtime_Error& push_frame_catch(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_catch, sloc, this->m_value);
        return *this;
      }
    Runtime_Error& push_frame_call(const Source_Location& sloc, const cow_string& inside)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_call, sloc, inside);
        return *this;
      }
    Runtime_Error& push_frame_func(const Source_Location& sloc, const cow_string& func)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_func, sloc, func);
        return *this;
      }
    Runtime_Error& push_frame_defer(const Source_Location& sloc)
      {
        // Append a new frame to the current backtrace.
        this->do_insert_frame(frame_type_defer, sloc, this->m_value);
        return *this;
      }
  };

#define ASTERIA_RUNTIME_TRY           try {  \
                                        try
                                          // Perform operations that might throw exceptions here.
#define ASTERIA_RUNTIME_CATCH(...)      catch(::Asteria::Runtime_Error&)  \
                                          { throw;  }  \
                                        catch(::std::exception& zTrSr9grT)  \
                                          { throw ::Asteria::Runtime_Error(zTrSr9grT); }  \
                                      }  \
                                      catch(__VA_ARGS__)

}  // namespace Asteria

#endif
