// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "utilities.hpp"

namespace Asteria {

namespace {

  [[noreturn]] void do_throw_parser_error(const Parser_error &err)
    {
      ASTERIA_THROW_RUNTIME_ERROR("There was an error in the source code provided:\n",
                                  "line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(), "\n",
                                  "code = ", err.code(), ": ", Parser_error::get_code_description(err.code()));
    }

}

Simple_source_file::Simple_source_file(std::istream &cstrm_io, String filename)
  : m_filename(std::move(filename))
  {
    ASTERIA_DEBUG_LOG("`Simple_source_file` constructor: ", static_cast<void *>(this));
    Token_stream tstrm;
    if(!tstrm.load(cstrm_io, this->m_filename)) {
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

const String & Simple_source_file::get_filename() const noexcept
  {
    return this->m_filename;
  }

Reference Simple_source_file::execute(Global_context &global, Vector<Reference> args) const
  {
    return this->m_code.execute_as_function(global, this->m_filename, 0, String::shallow("<top level>"), { }, { }, std::move(args));
  }

}
