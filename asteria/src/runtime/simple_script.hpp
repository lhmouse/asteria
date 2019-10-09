// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_
#define ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"

namespace Asteria {

class Simple_Script
  {
  private:
    Compiler_Options m_opts = { };
    rcptr<Rcbase> m_cptr;  // note type erasure

  public:
    Simple_Script()
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

    bool empty() const noexcept
      {
        return this->m_cptr == nullptr;
      }
    explicit operator bool () const noexcept
      {
        return this->m_cptr != nullptr;
      }
    Simple_Script& clear() noexcept
      {
        return this->m_cptr.reset(), *this;
      }

    Simple_Script& reload(tinybuf& cbuf, const cow_string& name);
    Simple_Script& reload_string(const cow_string& code, const cow_string& name);
    Simple_Script& reload_file(const cow_string& path);

    rcptr<Abstract_Function> copy_function_opt() const noexcept;

    Reference execute(const Global_Context& global, cow_vector<Reference>&& args) const;
    Reference execute(const Global_Context& global, cow_vector<Value>&& vals) const;
    Reference execute(const Global_Context& global) const;
  };

}  // namespace Asteria

#endif
