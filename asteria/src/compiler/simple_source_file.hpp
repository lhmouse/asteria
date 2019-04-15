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
    bool m_throw_on_failure;
    // We want type erasure.
    Cow_Vector<Instantiated_Function> m_inst;

  public:
    Simple_Source_File() noexcept
      : m_throw_on_failure(false)
      {
      }
    Simple_Source_File(std::streambuf& cbuf, const Cow_String& filename)
      : m_throw_on_failure(true)
      {
        this->reload(cbuf, filename);
      }
    Simple_Source_File(std::istream& cstrm, const Cow_String& filename)
      : m_throw_on_failure(true)
      {
        this->reload(cstrm, filename);
      }
    Simple_Source_File(const Cow_String& cstr, const Cow_String& filename)
      : m_throw_on_failure(true)
      {
        this->reload(cstr, filename);
      }
    explicit Simple_Source_File(const Cow_String& filename)
      : m_throw_on_failure(true)
      {
        this->open(filename);
      }

  private:
    inline Parser_Error do_reload_nothrow(std::streambuf& cbuf, const Cow_String& filename);
    inline Parser_Error do_throw_or_return(Parser_Error&& err);

  public:
    bool does_throw_on_failure() const noexcept
      {
        return this->m_throw_on_failure;
      }
    void set_throw_on_failure(bool throw_on_failure = true) noexcept
      {
        this->m_throw_on_failure = throw_on_failure;
      }

    Parser_Error reload(std::streambuf& cbuf, const Cow_String& filename);
    Parser_Error reload(std::istream& cstrm, const Cow_String& filename);
    Parser_Error reload(const Cow_String& cstr, const Cow_String& filename);
    Parser_Error open(const Cow_String& filename);
    void clear() noexcept;

    Reference execute(const Global_Context& global, Cow_Vector<Reference>&& args) const;
  };

}  // namespace Asteria

#endif
