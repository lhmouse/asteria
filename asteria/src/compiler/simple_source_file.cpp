// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_source_file.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "../runtime/air_node.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Parser_Error Simple_Source_File::do_reload_nothrow(std::streambuf& sbuf, const cow_string& filename)
  {
    // Use default opts.
    AIR_Node::S_define_function xnode = { };
    // Tokenize the character stream.
    Token_Stream tstrm;
    if(!tstrm.load(sbuf, filename, xnode.opts)) {
      return tstrm.get_parser_error();
    }
    // Parse tokens.
    Parser parser;
    if(!parser.load(tstrm, xnode.opts)) {
      return parser.get_parser_error();
    }
    // Initialize arguments for the function object.
    xnode.sloc = std::make_pair(filename, 1);
    xnode.name = rocket::sref("<file scope>");
    xnode.params.emplace_back(rocket::sref("..."));
    xnode.body = parser.get_statements();
    // Construct an IR node so we can reuse its code somehow.
    AIR_Node node(rocket::move(xnode));
    ASTERIA_DEBUG_LOG("Instantiating file \'", filename, "\'...");
    auto qtarget = node.instantiate_function(nullptr);
    ASTERIA_DEBUG_LOG("Finished instantiating file \'", filename, "\' as `", *qtarget, "`.");
    // Accept it.
    this->m_cptr = rocket::move(qtarget);
    return parser_status_success;
  }

Parser_Error Simple_Source_File::do_throw_or_return(Parser_Error&& err)
  {
    if(this->m_fthr && (err != parser_status_success)) {
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
      return this->do_throw_or_return(parser_status_istream_open_failure);
    }
    // Extract characters from the stream buffer directly.
    opt<Parser_Error> qerr;
    std::ios_base::iostate state = { };
    try {
      qerr = this->do_reload_nothrow(*(istrm.rdbuf()), filename);
      // If the source code contains errors, fail.
      if(*qerr != parser_status_success) {
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
      return this->do_throw_or_return(parser_status_istream_badbit_set);
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
      return this->do_throw_or_return(parser_status_istream_open_failure);
    }
    return this->reload(sbuf, filename);
  }

Reference Simple_Source_File::execute(const Global_Context& global, cow_vector<Reference>&& args) const
  {
    auto qtarget = rocket::dynamic_pointer_cast<Abstract_Function>(this->m_cptr);
    if(!qtarget) {
      ASTERIA_THROW_RUNTIME_ERROR("No code has been loaded so far.");
    }
    Reference self;
    qtarget->invoke(self, global, rocket::move(args));
    self.finish_call(global);
    return self;
  }

}  // namespace Asteria
