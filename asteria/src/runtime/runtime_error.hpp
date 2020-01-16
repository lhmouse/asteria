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
    // Create a comprehensive string that is also human-readable.
    cow_string m_what;

  public:
    template<typename XValT, ASTERIA_SFINAE_CONSTRUCT(Value, XValT&&)> Runtime_Error(const Source_Location& sloc,
                                                                                     XValT&& xval)
      :
        m_value(::rocket::forward<XValT>(xval))
      {
        this->m_frames.emplace_back(frame_type_throw, sloc, this->m_value);
        this->do_compose_message();
      }
    explicit Runtime_Error(const exception& stdex)
      :
        m_value(G_string(stdex.what()))
      {
        this->m_frames.emplace_back(frame_type_native, ::rocket::sref("<native code>"), -1, this->m_value);
        this->do_compose_message();
      }
    ~Runtime_Error() override;

  private:
    void do_compose_message();

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

    template<typename XValT> Backtrace_Frame& push_frame_throw(const Source_Location& sloc, XValT&& xval)
      {
        // The value also replaces the one in `*this`.
        this->m_value = ::rocket::forward<XValT>(xval);
        auto& frm = this->m_frames.emplace_back(frame_type_throw, sloc, this->m_value);
        this->do_compose_message();
        return frm;
      }
    Backtrace_Frame& push_frame_catch(const Source_Location& sloc)
      {
        // The value is the one stored in `*this` at this point.
        auto& frm = this->m_frames.emplace_back(frame_type_catch, sloc, this->m_value);
        this->do_compose_message();
        return frm;
      }
    Backtrace_Frame& push_frame_func(const Source_Location& sloc, const cow_string& func)
      {
        // The value is the signature of the enclosing function.
        auto& frm = this->m_frames.emplace_back(frame_type_func, sloc, func);
        this->do_compose_message();
        return frm;
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
