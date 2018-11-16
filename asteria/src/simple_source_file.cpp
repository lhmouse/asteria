// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "function_header.hpp"
#include "utilities.hpp"

namespace Asteria {

    namespace {

    [[noreturn]] void do_throw_parser_error(const Parser_error &err)
      {
        ASTERIA_THROW_RUNTIME_ERROR("There was an error in the source code provided:\n",
                                    "line = ", err.get_line(), ", offset = ", err.get_offset(), ", length = ", err.get_length(), "\n",
                                    "code = ", err.get_code(), ": ", Parser_error::get_code_description(err.get_code()));
      }

    }

Simple_source_file::Simple_source_file(std::istream &cstrm_io, const rocket::cow_string &file)
  {
    ASTERIA_DEBUG_LOG("`Simple_source_file` constructor: ", static_cast<void *>(this));
    // Tokenize.
    Token_stream tstrm;
    if(!tstrm.load(cstrm_io, file)) {
      do_throw_parser_error(tstrm.get_parser_error());
    }
    // Parse.
    Parser parser;
    if(!parser.load(tstrm)) {
      do_throw_parser_error(parser.get_parser_error());
    }
    // TODO: Optimize?
    // Finish.
    this->m_file = file;
    this->m_code = parser.extract_document();
  }

Simple_source_file::~Simple_source_file()
  {
    ASTERIA_DEBUG_LOG("`Simple_source_file` destructor: ", static_cast<void *>(this));
  }

Reference Simple_source_file::execute(Global_context &global, rocket::cow_vector<Reference> &&args) const
  {
    Reference result;
    Function_header head(this->m_file, 0, rocket::cow_string::shallow("<file scope>"), { });
    this->m_code.execute_as_function(result, global, std::move(head), nullptr, { }, std::move(args));
    return result;
  }

}
