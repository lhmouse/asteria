// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_
#define ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Simple_Script
  {
  private:
    cow_vector<phsh_string> m_params;  // constant
    cow_function m_func;  // note type erasure

  public:
    Simple_Script() noexcept
      {
      }
    Simple_Script(tinybuf& cbuf, const cow_string& name, const Compiler_Options& opts = { })
      {
        this->reload(cbuf, name, opts);
      }

  public:
    explicit operator bool () const noexcept
      {
        return bool(this->m_func);
      }
    Simple_Script& clear() noexcept
      {
        return this->m_func.reset(), *this;
      }

    Simple_Script& reload(tinybuf& cbuf, const cow_string& name, const Compiler_Options& opts = { });

    Simple_Script& reload_string(const cow_string& code, const cow_string& name, const Compiler_Options& opts = { });
    Simple_Script& reload_file(const cow_string& path, const Compiler_Options& opts = { });
    Simple_Script& reload_stdin(const Compiler_Options& opts = { });

    Reference execute(Global_Context& global, cow_vector<Reference>&& args = { }) const;
    Reference execute(Global_Context& global, cow_vector<Value>&& vals) const;
  };

}  // namespace Asteria

#endif
