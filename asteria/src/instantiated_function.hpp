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
    Vector<String> m_params;
    String m_file;
    Unsigned m_line;
    Block m_body;

  public:
    Instantiated_function(Vector<String> params, String file, Unsigned line, Block body) noexcept
      : m_params(std::move(params)), m_file(std::move(file)), m_line(line), m_body(std::move(body))
      {
      }
    ~Instantiated_function();

  public:
    String describe() const override;
    Reference invoke(Reference self, Vector<Reference> args) const override;
  };

}

#endif
