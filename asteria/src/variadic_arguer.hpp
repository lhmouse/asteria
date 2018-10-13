// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIADIC_ARGUER_HPP_
#define ASTERIA_VARIADIC_ARGUER_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "reference.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Variadic_arguer : public Abstract_function
  {
  private:
    String m_file;
    Uint32 m_line;
    Vector<Reference> m_vargs;

  public:
    Variadic_arguer(String file, Uint32 line, Vector<Reference> vargs) noexcept
      : m_file(std::move(file)), m_line(line), m_vargs(std::move(vargs))
      {
      }
    ~Variadic_arguer();

  public:
    String describe() const override;
    void collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const override;

    Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const override;
  };

}

#endif
