// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIADIC_ARGUER_HPP_
#define ASTERIA_VARIADIC_ARGUER_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "reference.hpp"

namespace Asteria {

class Variadic_arguer
  : public Abstract_function
  {
  private:
    String m_file;
    Unsigned m_line;
    Vector<Reference> m_vargs;

  public:
    Variadic_arguer(String file, Unsigned line, Vector<Reference> vargs)
      : m_file(std::move(file)), m_line(line), m_vargs(std::move(vargs))
      {
      }
    ~Variadic_arguer();

    Variadic_arguer(const Variadic_arguer &)
      = delete;
    Variadic_arguer & operator=(const Variadic_arguer &)
      = delete;

  public:
    String describe() const override;
    Reference invoke(Reference self, Vector<Reference> args) const override;
  };

}

#endif
