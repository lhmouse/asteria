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
    rocket::insertable_ostream mos;
    mos << "An error was encountered while parsing source data: " << Parser_Error::get_code_description(err.code());
    // Append error location information.
    mos << "\n[parser error " << err.code() << " encountered at ";
    if(err.line() == 0) {
      mos << "the end of source data";
    } else {
      mos << "line " << err.line() << ", offset " << err.offset();
    }
    mos << "]";
    // Throw it now.
    throw_runtime_error(std::move(mos), ROCKET_FUNCSIG);
  }

Parser_Error Simple_Source_File::load_file(const Cow_String &filename)
  {
    std::ifstream ifs(filename.c_str());
    return this->load_stream(ifs, filename);
  }

Parser_Error Simple_Source_File::load_stream(std::istream &cstrm_io, const Cow_String &filename)
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
    this->m_code = Cow_Vector<Statement>();
    this->m_file = rocket::sref("");
  }

Reference Simple_Source_File::execute(Global_Context &global, Cow_Vector<Reference> &&args) const
  {
    Reference self;
    Variadic_Arguer zvarg(Source_Location(this->m_file, 0), rocket::sref("<file scope>"));
    this->m_code.execute_as_function(self, global, std::ref(zvarg), { }, std::move(args));
    return self;
  }

}
