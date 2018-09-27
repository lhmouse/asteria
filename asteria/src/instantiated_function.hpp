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
    Uint64 m_line;
    Block m_body;

  public:
    Instantiated_function(Vector<String> params, String file, Uint64 line, Block body) noexcept;
    ~Instantiated_function();

  public:
    String describe() const override;
    Reference invoke(Reference self, Vector<Reference> args) const override;
  };

}

#endif
