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
    rcptr<Abstract_Function> m_cptr;  // note type erasure

  public:
    Simple_Source_File() noexcept
      : m_fthr(false)
      {
      }
    Simple_Source_File(std::streambuf& sbuf, const cow_string& filename)
      : m_fthr(true)
      {
        this->reload(sbuf, filename);
      }
    Simple_Source_File(std::istream& istrm, const cow_string& filename)
      : m_fthr(true)
      {
        this->reload(istrm, filename);
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
    inline Parser_Error do_reload_nothrow(std::streambuf& sbuf, const cow_string& filename);
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

    Parser_Error reload(std::streambuf& sbuf, const cow_string& filename);
    Parser_Error reload(std::istream& istrm, const cow_string& filename);
    Parser_Error reload(const cow_string& cstr, const cow_string& filename);
    Parser_Error open(const cow_string& filename);
    void clear() noexcept;

    Reference execute(const Global_Context& global, cow_vector<Reference>&& args) const;
  };

inline std::istream& operator>>(std::istream& istrm, Simple_Source_File& file)
  {
    return file.reload(istrm, rocket::sref("<anonymous>")), istrm;
  }

}  // namespace Asteria

#endif
