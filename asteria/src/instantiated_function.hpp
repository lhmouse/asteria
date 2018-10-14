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
    String m_func;
    Vector<String> m_params;
    Block m_body_bnd;

  public:
    Instantiated_function(const String &file, Uint32 line, const String &func, const Vector<String> &params, Block body_bnd)
      : m_file(file), m_line(line), m_func(func), m_params(params), m_body_bnd(std::move(body_bnd))
      {
      }
    ~Instantiated_function();

  public:
    String describe() const override;
    void collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const override;

    Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const override;
  };

}

#endif
