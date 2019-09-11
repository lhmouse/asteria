// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_script.hpp"
#include "air_node.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../utilities.hpp"
#include <fstream>

namespace Asteria {

Simple_Script& Simple_Script::reload(std::streambuf& sbuf, const cow_string& filename)
  {
    // Use default options.
    AIR_Node::S_define_function xnode = { };
    // Tokenize the character stream.
    Token_Stream tstrm(sbuf, filename, xnode.opts);
    // Parse tokens.
    Statement_Sequence stmseq(tstrm, xnode.opts);
    // Initialize arguments for the function object.
    xnode.sloc = std::make_pair(filename, 1);
    xnode.name = rocket::sref("<file scope>");
    xnode.params.emplace_back(rocket::sref("..."));
    xnode.body = stmseq.get_statements();
    // Construct an IR node so we can reuse its code somehow.
    AIR_Node node(rocket::move(xnode));
    ASTERIA_DEBUG_LOG("Instantiating file \'", filename, "\'...");
    auto qtarget = node.instantiate_function(nullptr);
    ASTERIA_DEBUG_LOG("Finished instantiating file \'", filename, "\' as `", *qtarget, "`.");
    // Accept it.
    this->m_cptr = rocket::move(qtarget);
    return *this;
  }

Simple_Script& Simple_Script::reload(std::istream& istrm, const cow_string& filename)
  {
    std::istream::sentry sentry(istrm, true);
    if(!sentry) {
      throw Parser_Error(parser_status_istream_input_failure);
    }
    // Extract characters from the stream buffer directly.
    std::ios_base::iostate state = { };
    try {
      this->reload(*(istrm.rdbuf()), filename);
      // `reload()` shall have consumed all data, so `eofbit` is always set.
      state |= std::ios_base::eofbit;
    }
    catch(...) {
      rocket::handle_ios_exception(istrm, state);
      // Don't swallow the exception.
      throw;
    }
    if(state) {
      istrm.setstate(state);
    }
    return *this;
  }

Simple_Script& Simple_Script::reload(const cow_string& cstr, const cow_string& filename)
  {
    // Use a `streambuf` in place of an `istream` to minimize overheads.
    cow_stringbuf sbuf;
    sbuf.set_string(cstr);
    return this->reload(sbuf, filename);
  }

Simple_Script& Simple_Script::reload_file(const cow_string& filename)
  {
    // Open the file designated by `filename`.
    std::filebuf sbuf;
    if(!sbuf.open(filename.c_str(), std::ios_base::in)) {
      throw Parser_Error(parser_status_ifstream_open_failure);
    }
    return this->reload(sbuf, filename);
  }

Reference Simple_Script::execute() const
  {
    // Invoke the function with no argument.
    cow_vector<Reference> args;
    return this->execute(rocket::move(args));
  }

Reference Simple_Script::execute(cow_vector<Reference>&& args) const
  {
    auto qtarget = rocket::dynamic_pointer_cast<Abstract_Function>(this->m_cptr);
    if(!qtarget) {
      ASTERIA_THROW_RUNTIME_ERROR("No code has been loaded so far.");
    }
    // Call the instantiated function.
    Reference self;
    qtarget->invoke(self, this->m_global, rocket::move(args));
    // Unpack TCO'd calls.
    self.finish_call(this->m_global);
    return self;
  }

Reference Simple_Script::execute(cow_vector<Value>&& vals) const
  {
    // Convert values to temporaries.
    cow_vector<Reference> args(vals.size());
    for(size_t i = 0; i != args.size(); ++i) {
      Reference_Root::S_temporary xref = { vals[i] };
      args.mut(i) = rocket::move(xref);
    }
    return this->execute(rocket::move(args));
  }

}  // namespace Asteria
