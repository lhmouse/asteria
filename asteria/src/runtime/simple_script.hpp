// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_
#define ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_

#include "../fwd.hpp"
#include "../abstract_function.hpp"

namespace Asteria {

class Simple_Script
  {
  private:
    cow_vector<phsh_string> m_params;  // constant
    Compiler_Options m_opts = { };
    rcptr<Abstract_Function> m_cptr;  // note type erasure

  public:
    Simple_Script() noexcept
      {
      }
    Simple_Script(tinybuf& cbuf, const cow_string& name)
      {
        this->reload(cbuf, name);
      }

  public:
    const Compiler_Options& get_options() const noexcept
      {
        return this->m_opts;
      }
    Compiler_Options& open_options() noexcept
      {
        return this->m_opts;
      }
    Simple_Script& set_options(const Compiler_Options& opts) noexcept
      {
        return this->m_opts = opts, *this;
      }

    explicit operator bool () const noexcept
      {
        return bool(this->m_cptr);
      }
    Simple_Script& clear() noexcept
      {
        return this->m_cptr.reset(), *this;
      }

    Simple_Script& reload(tinybuf& cbuf, const cow_string& name = ::rocket::sref("<unnamed>"));
    Simple_Script& reload_string(const cow_string& code, const cow_string& name);
    Simple_Script& reload_file(const cow_string& path);
    Simple_Script& reload_stdin();

    Reference execute(Global_Context& global, cow_vector<Reference>&& args = nullopt) const;
    Reference execute(Global_Context& global, cow_vector<Value>&& vals) const;
  };

}  // namespace Asteria

#endif
