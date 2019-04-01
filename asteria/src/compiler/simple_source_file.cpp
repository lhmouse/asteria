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

Parser_Error Simple_Source_File::do_make_parser_error(Parser_Error::Code code)
  {
    return this->do_throw_or_return(Parser_Error(UINT32_MAX, SIZE_MAX, 0, code));
  }

Parser_Error Simple_Source_File::do_throw_or_return(Parser_Error&& err)
  {
    if((err != Parser_Error::code_success) && this->m_throw_on_failure) {
      Parser_Error::convert_to_runtime_error_and_throw(err);
    }
    return rocket::move(err);
  }

Parser_Error Simple_Source_File::reload(std::streambuf& cbuf, const Cow_String& filename)
  {
    // Use default options.
    static constexpr Parser_Options s_default_options = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(cbuf, filename, s_default_options)) {
      return this->do_throw_or_return(tstrm.get_parser_error());
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, s_default_options)) {
      return this->do_throw_or_return(parser.get_parser_error());
    }
    // Initialize parameters of the top scope.
    Source_Location sloc(filename, 1);
    // The file is considered to be a function taking variadic arguments.
    Cow_Vector<PreHashed_String> params;
    params.emplace_back(rocket::sref("..."));
    // Generate code.
    Cow_Vector<Air_Node> code;
    Analytic_Context ctx(nullptr);
    ctx.prepare_function_parameters(params);
    rocket::for_each(parser.get_statements(), [&](const Statement& stmt) { stmt.generate_code(code, nullptr, ctx);  });
    // Accept the code.
    this->m_inst.clear();
    this->m_inst.emplace_back(sloc, rocket::sref("<file scope"), params, rocket::move(code));
    return Parser_Error(0, 0, 0, Parser_Error::code_success);
  }

Parser_Error Simple_Source_File::reload(std::istream& cstrm, const Cow_String& filename)
  {
    std::istream::sentry cerb(cstrm);
    if(!cerb) {
      return this->do_make_parser_error(Parser_Error::code_istream_open_failure);
    }
    auto qbuf = cstrm.rdbuf();
    if(!qbuf) {
      cstrm.setstate(std::ios_base::badbit);
      return this->do_make_parser_error(Parser_Error::Parser_Error::code_istream_badbit_set);
    }
    // Extract characters from `*qbuf` directly.
    Opt<Parser_Error> qerr;
    auto state = std::ios_base::goodbit;
    try {
      qerr = this->reload(*qbuf, filename);
      // N.B. `reload()` will have consumed all data, so `eofbit` is always set.
      state |= std::ios_base::eofbit;
    } catch(...) {
      rocket::handle_ios_exception(cstrm, state);
    }
    if(state) {
      cstrm.setstate(state);
    }
    if(cstrm.bad()) {
      return this->do_make_parser_error(Parser_Error::Parser_Error::code_istream_badbit_set);
    }
    // `qerr` shall always have a value here.
    // If the exceptional path above was taken, `cstrm.bad()` would have been set.
    return this->do_throw_or_return(rocket::move(*qerr));
  }

Parser_Error Simple_Source_File::reload(const Cow_String& cstr, const Cow_String& filename)
  {
    // Create a `streambuf` from `cstr`.
    rocket::insertable_streambuf cbuf;
    cbuf.set_string(cstr);
    return this->reload(cbuf, filename);
  }

Parser_Error Simple_Source_File::open(const Cow_String& filename)
  {
    // Open the file designated by `filename`.
    std::filebuf cbuf;
    if(!cbuf.open(filename.c_str(), std::ios_base::in)) {
      return this->do_make_parser_error(Parser_Error::code_istream_open_failure);
    }
    return this->reload(cbuf, filename);
  }

void Simple_Source_File::clear() noexcept
  {
    // Destroy contents.
    this->m_inst.clear();
  }

Reference Simple_Source_File::execute(const Global_Context& global, Cow_Vector<Reference>&& args) const
  {
    Reference result;
    if(ROCKET_EXPECT(!this->m_inst.empty())) {
      this->m_inst.front().invoke(result, global, rocket::move(args));
    }
    return result;
  }

}  // namespace Asteria
