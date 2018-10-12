// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_INSTANTIATED_FUNCTION_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "block.hpp"

namespace Asteria {

class Instantiated_function : public Abstract_function
  {
  private:
    String m_file;
    Uint32 m_line;
    String m_name;
    Vector<String> m_params;
    Block m_body;

  public:
    Instantiated_function(String file, Uint32 line, String name, Vector<String> params, Block body) noexcept
      : m_file(std::move(file)), m_line(line), m_name(std::move(name)), m_params(std::move(params)), m_body(std::move(body))
      {
      }
    ~Instantiated_function();

  public:
    String describe() const override;
    void collect_variables(bool (*callback)(void *, const Rcptr<Variable> &), void *param) const override;

    Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const override;
  };

}

#endif
