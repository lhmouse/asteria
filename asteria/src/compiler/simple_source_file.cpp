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

Parser_Error Simple_Source_File::do_reload_nothrow(std::streambuf& cbuf, const Cow_String& filename)
  {
    // Use default options.
    Parser_Options options = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(cbuf, filename, options)) {
      return tstrm.get_parser_error();
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, options)) {
      return parser.get_parser_error();
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
    this->m_inst.emplace_back(sloc, rocket::sref("<file scope>"), params, rocket::move(code));
    return Parser_Error(UINT32_MAX, SIZE_MAX, 0, Parser_Error::code_success);
  }

Parser_Error Simple_Source_File::do_throw_or_return(Parser_Error&& err)
  {
    if(this->m_throw_on_failure && (err != Parser_Error::code_success)) {
      err.convert_to_runtime_error_and_throw();
    }
    return rocket::move(err);
  }

Parser_Error Simple_Source_File::reload(std::streambuf& cbuf, const Cow_String& filename)
  {
    return this->do_throw_or_return(this->do_reload_nothrow(cbuf, filename));
  }

Parser_Error Simple_Source_File::reload(std::istream& cstrm, const Cow_String& filename)
  {
    std::istream::sentry sentry(cstrm, true);
    if(!sentry) {
      return this->do_throw_or_return(Parser_Error(UINT32_MAX, SIZE_MAX, 0, Parser_Error::code_istream_open_failure));
    }
    // Extract characters from the stream buffer directly.
    Opt<Parser_Error> qerr;
    std::ios_base::iostate state = std::ios_base::goodbit;
    try {
      qerr = this->do_reload_nothrow(*(cstrm.rdbuf()), filename);
      // N.B. `do_reload_nothrow()` shall have consumed all data, so `eofbit` is always set.
      state |= std::ios_base::eofbit;
      // If the source code contains errors, fail.
      if(*qerr != Parser_Error::code_success) {
        state |= std::ios_base::failbit;
      }
    }
    catch(...) {
      rocket::handle_ios_exception(cstrm, state);
    }
    // If `eofbit` or `failbit` would cause an exception, throw it here.
    if(state) {
      cstrm.setstate(state);
    }
    if(cstrm.bad()) {
      return this->do_throw_or_return(Parser_Error(UINT32_MAX, SIZE_MAX, 0, Parser_Error::code_istream_badbit_set));
    }
    // `qerr` shall always have a value here.
    // If the exceptional path above has been taken, `cstrm.bad()` will have been set.
    return this->do_throw_or_return(rocket::move(*qerr));
  }

Parser_Error Simple_Source_File::reload(const Cow_String& cstr, const Cow_String& filename)
  {
    // Use a `streambuf` in place of an `istream` to minimize overheads.
    Cow_stringbuf cbuf;
    cbuf.set_string(cstr);
    return this->reload(cbuf, filename);
  }

Parser_Error Simple_Source_File::open(const Cow_String& filename)
  {
    // Open the file designated by `filename`.
    std::filebuf cbuf;
    if(!cbuf.open(filename.c_str(), std::ios_base::in)) {
      return this->do_throw_or_return(Parser_Error(UINT32_MAX, SIZE_MAX, 0, Parser_Error::code_istream_open_failure));
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
