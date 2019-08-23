// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_ERROR_HPP_
#define ASTERIA_COMPILER_PARSER_ERROR_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Parser_Error
  {
  private:
    int32_t m_line;
    size_t m_offset;
    size_t m_length;
    Parser_Status m_status;

  public:
    constexpr Parser_Error(Parser_Status xstatus) noexcept
      : m_line(-1), m_offset(0), m_length(0), m_status(xstatus)
      {
      }
    constexpr Parser_Error(int32_t xline, size_t xoffset, size_t xlength, Parser_Status xstatus) noexcept
      : m_line(xline), m_offset(xoffset), m_length(xlength), m_status(xstatus)
      {
      }

  public:
    constexpr int32_t line() const noexcept
      {
        return this->m_line;
      }
    constexpr size_t offset() const noexcept
      {
        return this->m_offset;
      }
    constexpr size_t length() const noexcept
      {
        return this->m_length;
      }
    constexpr Parser_Status status() const noexcept
      {
        return this->m_status;
      }
    const char* what_status() const noexcept
      {
        return describe_parser_status(this->m_status);
      }

    std::ostream& print(std::ostream& ostrm) const;
    [[noreturn]] void convert_to_runtime_error_and_throw() const;
  };

constexpr bool operator==(const Parser_Error& lhs, Parser_Status rhs) noexcept
  {
    return lhs.status() == rhs;
  }
constexpr bool operator!=(const Parser_Error& lhs, Parser_Status rhs) noexcept
  {
    return lhs.status() != rhs;
  }

constexpr bool operator==(Parser_Status lhs, const Parser_Error& rhs) noexcept
  {
    return lhs == rhs.status();
  }
constexpr bool operator!=(Parser_Status lhs, const Parser_Error& rhs) noexcept
  {
    return lhs != rhs.status();
  }

inline std::ostream& operator<<(std::ostream& ostrm, const Parser_Error& error)
  {
    return error.print(ostrm);
  }

}  // namespace Asteria

#endif
