// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "../syntax/statement.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Parser_Error Simple_Source_File::do_reload_nothrow(std::streambuf& sbuf, const cow_string& filename)
  {
    // Use default options.
    Compiler_Options options = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(sbuf, filename, options)) {
      return tstrm.get_parser_error();
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, options)) {
      return parser.get_parser_error();
    }
    const auto& body = parser.get_statements();
    // Initialize parameters of the top scope.
    Source_Location sloc(filename, 1);
    // The file is considered to be a function taking variadic arguments.
    cow_vector<phsh_string> params;
    params.emplace_back(rocket::sref("..."));
    // Generate code.
    cow_vector<uptr<Air_Node>> code;
    Analytic_Context ctx(1, params);
    rocket::ranged_xfor(body.begin(), body.end(), [&](auto it) { it->generate_code(code, nullptr, ctx, options, false);  },
                                                  [&](auto it) { it->generate_code(code, nullptr, ctx, options, true);  });
    // Accept the code.
    this->m_cptr = rocket::make_refcnt<Instantiated_Function>(sloc, rocket::sref("<file scope>"), params, rocket::move(code));
    return Parser_Error(-1, SIZE_MAX, 0, Parser_Error::code_success);
  }

Parser_Error Simple_Source_File::do_throw_or_return(Parser_Error&& err)
  {
    if(this->m_fthr && (err != Parser_Error::code_success)) {
      err.convert_to_runtime_error_and_throw();
    }
    return rocket::move(err);
  }

Parser_Error Simple_Source_File::reload(std::streambuf& sbuf, const cow_string& filename)
  {
    return this->do_throw_or_return(this->do_reload_nothrow(sbuf, filename));
  }

Parser_Error Simple_Source_File::reload(std::istream& istrm, const cow_string& filename)
  {
    std::istream::sentry sentry(istrm, true);
    if(!sentry) {
      return this->do_throw_or_return(Parser_Error(-1, SIZE_MAX, 0, Parser_Error::code_istream_open_failure));
    }
    // Extract characters from the stream buffer directly.
    opt<Parser_Error> qerr;
    std::ios_base::iostate state = { };
    try {
      qerr = this->do_reload_nothrow(*(istrm.rdbuf()), filename);
      // If the source code contains errors, fail.
      if(*qerr != Parser_Error::code_success) {
        state |= std::ios_base::failbit;
      }
      // N.B. `do_reload_nothrow()` shall have consumed all data, so `eofbit` is always set.
      state |= std::ios_base::eofbit;
    }
    catch(...) {
      rocket::handle_ios_exception(istrm, state);
    }
    // If `eofbit` or `failbit` would cause an exception, throw it here.
    if(state) {
      istrm.setstate(state);
    }
    if(istrm.bad()) {
      return this->do_throw_or_return(Parser_Error(-1, SIZE_MAX, 0, Parser_Error::code_istream_badbit_set));
    }
    // `qerr` shall always have a value here.
    // If the exceptional path above has been taken, `istrm.bad()` will have been set.
    return this->do_throw_or_return(rocket::move(*qerr));
  }

Parser_Error Simple_Source_File::reload(const cow_string& cstr, const cow_string& filename)
  {
    // Use a `streambuf` in place of an `istream` to minimize overheads.
    cow_stringbuf sbuf;
    sbuf.set_string(cstr);
    return this->reload(sbuf, filename);
  }

Parser_Error Simple_Source_File::open(const cow_string& filename)
  {
    // Open the file designated by `filename`.
    std::filebuf sbuf;
    if(!sbuf.open(filename.c_str(), std::ios_base::in)) {
      return this->do_throw_or_return(Parser_Error(-1, SIZE_MAX, 0, Parser_Error::code_istream_open_failure));
    }
    return this->reload(sbuf, filename);
  }

void Simple_Source_File::clear() noexcept
  {
    this->m_cptr = nullptr;
  }

Reference Simple_Source_File::execute(const Global_Context& global, cow_vector<Reference>&& args) const
  {
    Reference self;
    if(!this->m_cptr) {
      ASTERIA_THROW_RUNTIME_ERROR("No code has been loaded so far.");
    }
    this->m_cptr->invoke(self, global, rocket::move(args));
    self.finish_call(global);
    return self;
  }

}  // namespace Asteria
