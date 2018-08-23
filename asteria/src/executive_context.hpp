// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_EXECUTIVE_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_context : public Abstract_context
  {
  private:
    const Executive_context *m_parent_opt;

  public:
    explicit Executive_context(const Executive_context *parent_opt = nullptr)
      : m_parent_opt(parent_opt)
      {
      }
    ~Executive_context();

    Executive_context(const Executive_context &)
      = delete;
    Executive_context & operator=(const Executive_context &)
      = delete;

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Executive_context * get_parent_opt() const noexcept override
      {
        return m_parent_opt;
      }
  };

extern void initialize_executive_function_context(Executive_context &ctx_out, const Vector<String> &params, const String &file, Unsigned line, Reference self, Vector<Reference> args);

}

#endif
