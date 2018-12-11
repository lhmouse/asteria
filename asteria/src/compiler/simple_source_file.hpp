// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"
#include "../syntax/block.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Simple_source_file
  {
  private:
    Block m_code;
    rocket::cow_string m_file;

  public:
    Simple_source_file() noexcept
      {
      }
    explicit Simple_source_file(const rocket::cow_string &filename)
      {
        const auto err = this->load_file(filename);
        this->do_throw_on_error(err);
      }
    Simple_source_file(std::istream &cstrm_io, const rocket::cow_string &filename)
      {
        const auto err = this->load_stream(cstrm_io, filename);
        this->do_throw_on_error(err);
      }
    ROCKET_COPYABLE_DESTRUCTOR(Simple_source_file);

  private:
    void do_throw_on_error(const Parser_error &err)
      {
        if(err == Parser_error::code_success) {
          return;
        }
        this->do_throw_error(err);
      }
    [[noreturn]] void do_throw_error(const Parser_error &err);

  public:
    Parser_error load_file(const rocket::cow_string &filename);
    Parser_error load_stream(std::istream &cstrm_io, const rocket::cow_string &filename);
    void clear() noexcept;

    Reference execute(Global_context &global, rocket::cow_vector<Reference> &&args) const;
  };

}

#endif
