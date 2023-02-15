// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_COMPILER_ERROR_
#define ASTERIA_COMPILER_COMPILER_ERROR_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../../rocket/tinyfmt_str.hpp"
#include <exception>
namespace asteria {

class Compiler_Error
  : public virtual exception
  {
  public:
    enum class M_status;
    enum class M_format;
    enum class M_add_format;

  private:
    Compiler_Status m_stat;
    Source_Location m_sloc;
    cow_string m_desc;

    ::rocket::tinyfmt_str m_fmt;  // human-readable message

  public:
    explicit
    Compiler_Error(M_status, Compiler_Status xstat, const Source_Location& xsloc)
      : m_stat(xstat), m_sloc(xsloc),
        m_desc(::rocket::sref(describe_compiler_status(xstat)))
      { this->do_compose_message();  }

    template<typename... ParamsT>
    explicit
    Compiler_Error(M_format, Compiler_Status xstat, const Source_Location& xsloc,
                   const char* templ, const ParamsT&... params)
      : m_stat(xstat), m_sloc(xsloc)
      {
        format(this->m_fmt, templ, params...);
        this->m_desc = this->m_fmt.extract_string();
        this->do_compose_message();
      }

    template<typename... ParamsT>
    explicit
    Compiler_Error(M_add_format, Compiler_Status xstat, const Source_Location& xsloc,
                   const char* templ, const ParamsT&... params)
      : m_stat(xstat), m_sloc(xsloc)
      {
        format(this->m_fmt, templ, params...);
        this->m_desc = this->m_fmt.extract_string();
        const char* stat_str = describe_compiler_status(xstat);
        size_t stat_len = ::std::strlen(stat_str);
        this->m_desc.insert(0, stat_str, stat_len + 1);
        this->m_desc.mut(stat_len) = '\n';
        this->do_compose_message();
      }

  private:
    void
    do_compose_message();

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Compiler_Error);

    const char*
    what() const noexcept override
      { return this->m_fmt.c_str();  }

    Compiler_Status
    status() const noexcept
      { return this->m_stat;  }

    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file() const noexcept
      { return this->m_sloc.file();  }

    int
    line() const noexcept
      { return this->m_sloc.line();  }

    int
    column() const noexcept
      { return this->m_sloc.column();  }
  };

inline
bool
operator==(const Compiler_Error& lhs, Compiler_Status rhs) noexcept
  {
    return lhs.status() == rhs;
  }

inline
bool
operator!=(const Compiler_Error& lhs, Compiler_Status rhs) noexcept
  {
    return lhs.status() != rhs;
  }

inline
bool
operator==(Compiler_Status lhs, const Compiler_Error& rhs) noexcept
  {
    return lhs == rhs.status();
  }

inline
bool
operator!=(Compiler_Status lhs, const Compiler_Error& rhs) noexcept
  {
    return lhs != rhs.status();
  }

}  // namespace asteria
#endif
