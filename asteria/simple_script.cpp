// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "simple_script.hpp"
#include "compiler/token_stream.hpp"
#include "compiler/statement_sequence.hpp"
#include "compiler/expression_unit.hpp"
#include "runtime/air_optimizer.hpp"
#include "runtime/variable.hpp"
#include "runtime/garbage_collector.hpp"
#include "llds/reference_stack.hpp"
#include "utils.hpp"
namespace asteria {

refcnt_ptr<Variable>
Simple_Script::
get_global_variable_opt(phsh_stringR name) const noexcept
  {
    auto gref = this->m_global.get_named_reference_opt(name);
    if(!gref)
      return nullptr;

    // Errors are ignored.
    return gref->unphase_variable_opt();
  }

refcnt_ptr<Variable>
Simple_Script::
open_global_variable(phsh_stringR name)
  {
    auto& ref = this->m_global.insert_named_reference(name);
    auto var = ref.unphase_variable_opt();
    if(var)
      return var;

    // Allocate a new variable and take it over.
    var = this->m_global.garbage_collector()->create_variable();
    ref.set_variable(var);
    return var;
  }

bool
Simple_Script::
erase_global_variable(phsh_stringR name) noexcept
  {
    return this->m_global.erase_named_reference(name, nullptr);
  }

void
Simple_Script::
reload(cow_stringR name, Statement_Sequence&& stmtq)
  {
    // Instantiate the function.
    cow_vector<phsh_string> script_params;
    script_params.emplace_back(sref("..."));

    AIR_Optimizer optmz(this->m_opts);
    optmz.reload(nullptr, script_params, this->m_global, stmtq.get_statements());

    Source_Location script_sloc(name, 0, 0);
    this->m_func = optmz.create_function(script_sloc, sref("[file scope]"));
  }

void
Simple_Script::
reload(cow_stringR name, Token_Stream&& tstrm)
  {
    Statement_Sequence stmtq(this->m_opts);
    stmtq.reload(move(tstrm));
    this->reload(name, move(stmtq));
  }

void
Simple_Script::
reload(cow_stringR name, int line, tinybuf&& cbuf)
  {
    Token_Stream tstrm(this->m_opts);
    tstrm.reload(name, line, move(cbuf));
    this->reload(name, move(tstrm));
  }

void
Simple_Script::
reload_string(cow_stringR name, int line, cow_stringR code)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    this->reload(name, line, move(cbuf));
  }

void
Simple_Script::
reload_string(cow_stringR name, cow_stringR code)
  {
    this->reload_string(name, 1, code);
  }

void
Simple_Script::
reload_stdin(int line)
  {
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    this->reload(sref("[stdin]"), line, move(cbuf));
  }

void
Simple_Script::
reload_stdin()
  {
    this->reload_stdin(1);
  }

void
Simple_Script::
reload_file(const char* path)
  {
    unique_ptr<char, void (void*)> abspath(::free);
    if(!abspath.reset(::realpath(path, nullptr)))
      ASTERIA_THROW((
          "Could not open script file '$1'",
          "[`realpath()` failed: ${errno:full}]"),
          path);

    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath, tinybuf::open_read);
    this->reload(cow_string(abspath), 1, move(cbuf));
  }

void
Simple_Script::
reload_file(cow_stringR path)
  {
    this->reload_file(path.safe_c_str());
  }

Reference
Simple_Script::
execute(Reference_Stack&& stack)
  {
    Reference self;
    this->m_global.set_recursion_base(&self);
    auto func = this->m_func;
    func.invoke(self, this->m_global, move(stack));
    ::fflush(nullptr);
    return self;
  }

Reference
Simple_Script::
execute(cow_vector<Value>&& args)
  {
    Reference_Stack stack;
    for(auto it = args.mut_begin();  it != args.end(); ++it)
      stack.push().set_temporary(move(*it));
    return this->execute(move(stack));
  }

Reference
Simple_Script::
execute()
  {
    Reference_Stack stack;
    return this->execute(move(stack));
  }

}  // namespace asteria
