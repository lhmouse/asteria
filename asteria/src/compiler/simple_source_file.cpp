// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "../syntax/statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/function_analytic_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Parser_Error Simple_Source_File::do_reload_file(bool throw_on_failure, const Cow_String &filename)
  {
    std::ifstream ifstrm(filename.c_str());
    return this->do_reload_stream(throw_on_failure, ifstrm, filename);
  }

    namespace {

    Parser_Error do_handle_parser_error(bool throw_on_failure, const Parser_Error &error)
      {
        if(ROCKET_EXPECT(!throw_on_failure)) {
          return error;
        }
        rocket::insertable_ostream mos;
        mos << "An error was encountered while parsing source data: " << Parser_Error::get_code_description(error.code());
        // Append error location information.
        mos << "\n[parser error " << error.code() << " encountered at ";
        if(error.line() == 0) {
          mos << "the end of stream";
        } else {
          mos << "line " << error.line() << ", offset " << error.offset() << ", length " << error.length();
        }
        mos << "]";
        // Throw it now.
        throw_runtime_error(ROCKET_FUNCSIG, mos.extract_string());
      }

    }

Parser_Error Simple_Source_File::do_reload_stream(bool throw_on_failure, std::istream &cstrm_io, const Cow_String &filename)
  {
    // Use default options.
    static constexpr Parser_Options options = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(cstrm_io, filename, options)) {
      return do_handle_parser_error(throw_on_failure, tstrm.get_parser_error());
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, options)) {
      return do_handle_parser_error(throw_on_failure, parser.get_parser_error());
    }
    // Initialize parameters of the top scope.
    const Source_Location sloc(filename, 0);
    const Cow_Vector<PreHashed_String> params;
    // Generate code.
    Cow_Vector<RefCnt_Object<Air_Node>> code;
    Function_Analytic_Context ctx(nullptr, params);
    for(const auto &stmt : parser.get_statements()) {
      stmt.generate_code(code, ctx);
    }
    // Accept the code.
    this->m_codev.clear();
    this->m_codev.emplace_back(sloc, rocket::sref("<file scope>"), params, std::move(code));
    return Parser_Error(0, 0, 0, Parser_Error::code_success);
  }

Reference Simple_Source_File::execute(const Global_Context &global, Cow_Vector<Reference> &&args) const
  {
    // This is initialized to `null`.
    Reference result;
    if(ROCKET_EXPECT(!this->m_codev.empty())) {
      // Execute the instantiated function.
      this->m_codev.front().invoke(result, global, std::move(args));
    }
    return result;
  }

}
