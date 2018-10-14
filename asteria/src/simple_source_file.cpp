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

Simple_source_file::Simple_source_file(std::istream &cstrm_io, const String &file)
  : m_file(file)
  {
    ASTERIA_DEBUG_LOG("`Simple_source_file` constructor: ", static_cast<void *>(this));
    Token_stream tstrm;
    if(!tstrm.load(cstrm_io, this->m_file)) {
      do_throw_parser_error(tstrm.get_parser_error());
    }
    Parser parser;
    if(!parser.load(tstrm)) {
      do_throw_parser_error(parser.get_parser_error());
    }
    this->m_code = parser.extract_document();
  }

Simple_source_file::~Simple_source_file()
  {
    ASTERIA_DEBUG_LOG("`Simple_source_file` destructor: ", static_cast<void *>(this));
  }

Reference Simple_source_file::execute(Global_context &global, Vector<Reference> args) const
  {
    return this->m_code.execute_as_function(global, Function_header(this->m_file, 0, String::shallow("<file scope>"), { }), nullptr, { }, std::move(args));
  }

}
