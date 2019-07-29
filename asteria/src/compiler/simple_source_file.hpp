// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"

namespace Asteria {

class Simple_Source_File
  {
  private:
    bool m_fthr;  // throw on failure
    cow_vector<Instantiated_Function> m_code;  // for type erasure

  public:
    Simple_Source_File() noexcept
      : m_fthr(false)
      {
      }
    Simple_Source_File(std::streambuf& cbuf, const cow_string& filename)
      : m_fthr(true)
      {
        this->reload(cbuf, filename);
      }
    Simple_Source_File(std::istream& cstrm, const cow_string& filename)
      : m_fthr(true)
      {
        this->reload(cstrm, filename);
      }
    Simple_Source_File(const cow_string& cstr, const cow_string& filename)
      : m_fthr(true)
      {
        this->reload(cstr, filename);
      }
    explicit Simple_Source_File(const cow_string& filename)
      : m_fthr(true)
      {
        this->open(filename);
      }

  private:
    inline Parser_Error do_reload_nothrow(std::streambuf& cbuf, const cow_string& filename);
    inline Parser_Error do_throw_or_return(Parser_Error&& err);

  public:
    bool does_throw_on_failure() const noexcept
      {
        return this->m_fthr;
      }
    void set_throw_on_failure(bool fthr = true) noexcept
      {
        this->m_fthr = fthr;
      }

    Parser_Error reload(std::streambuf& cbuf, const cow_string& filename);
    Parser_Error reload(std::istream& cstrm, const cow_string& filename);
    Parser_Error reload(const cow_string& cstr, const cow_string& filename);
    Parser_Error open(const cow_string& filename);
    void clear() noexcept;

    Reference execute(const Global_Context& global, cow_vector<Reference>&& args) const;
  };

inline std::istream& operator>>(std::istream& is, Simple_Source_File& file)
  {
    return file.reload(is, rocket::sref("<anonymous>")), is;
  }

}  // namespace Asteria

#endif
