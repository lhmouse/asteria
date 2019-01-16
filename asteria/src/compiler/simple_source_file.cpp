// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "../syntax/source_location.hpp"
#include "../runtime/reference.hpp"
#include "../runtime/variadic_arguer.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Simple_Source_File::~Simple_Source_File()
  {
  }

void Simple_Source_File::do_throw_error(const Parser_Error &err)
  {
    ASTERIA_THROW_RUNTIME_ERROR("An error was encountered while parsing source data:\n",
                                "line = ", err.get_line(), ", offset = ", err.get_offset(), ", length = ", err.get_length(), "\n",
                                "code = ", err.get_code(), ": ", Parser_Error::get_code_description(err.get_code()));
  }

Parser_Error Simple_Source_File::load_file(const rocket::cow_string &filename)
  {
    std::ifstream ifs(filename.c_str());
    return this->load_stream(ifs, filename);
  }

Parser_Error Simple_Source_File::load_stream(std::istream &cstrm_io, const rocket::cow_string &filename)
  {
    Token_Stream tstrm;
    if(!tstrm.load(cstrm_io, filename)) {
      return tstrm.get_parser_error();
    }
    Parser parser;
    if(!parser.load(tstrm)) {
      return parser.get_parser_error();
    }
    this->m_code = parser.extract_document();
    this->m_file = filename;
    return Parser_Error(0, 0, 0, Parser_Error::code_success);
  }

void Simple_Source_File::clear() noexcept
  {
    this->m_code = rocket::cow_vector<Statement>();
    this->m_file = std::ref("");
  }

Reference Simple_Source_File::execute(Global_Context &global, rocket::cow_vector<Reference> &&args) const
  {
    // Initialize parameters.
    const auto loc = Source_Location(this->m_file, 0);
    const auto name = rocket::cow_string(std::ref("<file scope>"));
    const auto zvarg = rocket::refcounted_object<Variadic_Arguer>(loc, name);
    const auto params = rocket::cow_vector<rocket::prehashed_string>();
    // Execute the code as a function.
    Reference self;
    this->m_code.execute_as_function(self, global, zvarg, params, std::move(args));
    return self;
  }

}
