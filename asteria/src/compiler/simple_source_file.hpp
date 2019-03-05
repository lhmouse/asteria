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
    Block m_code;
    Cow_String m_file;

  public:
    Simple_Source_File() noexcept
      : m_code(), m_file()
      {
      }
    explicit Simple_Source_File(const Cow_String &filename)
      : Simple_Source_File()
      {
        auto err = this->load_file(filename);
        this->do_throw_on_error(err);
      }
    Simple_Source_File(std::istream &cstrm_io, const Cow_String &filename)
      : Simple_Source_File()
      {
        auto err = this->load_stream(cstrm_io, filename);
        this->do_throw_on_error(err);
      }

  private:
    void do_throw_on_error(const Parser_Error &err)
      {
        if(ROCKET_EXPECT(err == Parser_Error::code_success)) {
          return;
        }
        this->do_throw_error(err);
      }
    [[noreturn]] void do_throw_error(const Parser_Error &err);

  public:
    Parser_Error load_file(const Cow_String &filename);
    Parser_Error load_stream(std::istream &cstrm_io, const Cow_String &filename);
    void clear() noexcept;

    Reference execute(const Global_Context &global, Cow_Vector<Reference> &&args) const;
  };

}

#endif
