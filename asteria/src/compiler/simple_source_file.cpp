// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "../syntax/statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Parser_Error Simple_Source_File::do_reload_file(bool throw_on_failure, const Cow_String& filename)
  {
    std::ifstream ifstrm(filename.c_str());
    auto err = this->do_reload_stream(throw_on_failure, ifstrm, filename);
    return err;
  }

Parser_Error Simple_Source_File::do_reload_stream(bool throw_on_failure, std::istream& cstrm, const Cow_String& filename)
  {
    // Use default options.
    static constexpr Parser_Options options = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(cstrm, filename, options)) {
      auto err = tstrm.get_parser_error();
      if(throw_on_failure) {
        Parser_Error::convert_to_runtime_error_and_throw(err);
      }
      return err;
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, options)) {
      auto err = parser.get_parser_error();
      if(throw_on_failure) {
        Parser_Error::convert_to_runtime_error_and_throw(err);
      }
      return err;
    }
    // Initialize parameters of the top scope.
    Source_Location sloc(filename, 0);
    // The file is considered to be a function taking variadic arguments.
    Cow_Vector<PreHashed_String> params;
    params.emplace_back(rocket::sref("..."));
    // Generate code.
    Cow_Vector<Air_Node> code_func;
    Analytic_Context ctx_func(nullptr);
    ctx_func.prepare_function_parameters(params);
    rocket::for_each(parser.get_statements(), [&](const Statement& stmt) { stmt.generate_code(code_func, nullptr, ctx_func);  });
    // Accept the code.
    this->m_codev.clear();
    this->m_codev.emplace_back(sloc, rocket::sref("<file scope>"), params, rocket::move(code_func));
    return Parser_Error(0, 0, 0, Parser_Error::code_success);
  }

Reference Simple_Source_File::execute(const Global_Context& global, Cow_Vector<Reference>&& args) const
  {
    // This is initialized to `null`.
    Reference result;
    if(ROCKET_EXPECT(!this->m_codev.empty())) {
      // Execute the instantiated function.
      this->m_codev.front().invoke(result, global, rocket::move(args));
    }
    return result;
  }

}  // namespace Asteria
