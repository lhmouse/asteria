// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

class Abstract_Context
  {
  private:
    struct Collection_Trigger
      {
        void operator()(Rcbase *base_opt) noexcept;
      };

  private:
    // Mind the order of destruction.
    Uptr<Rcbase, Collection_Trigger> m_tied_coll_opt;
    Reference_Dictionary m_named_refs;

  public:
    Abstract_Context() noexcept
      {
      }
    virtual ~Abstract_Context();

    Abstract_Context(const Abstract_Context &)
      = delete;
    Abstract_Context & operator=(const Abstract_Context &)
      = delete;

  protected:
    void do_tie_collector(Rcptr<Generational_Collector> tied_coll_opt) noexcept;

  public:
    const Reference * get_named_reference_opt(const PreHashed_String &name) const
      {
        return this->m_named_refs.get_opt(name);
      }
    Reference & open_named_reference(const PreHashed_String &name)
      {
        return this->m_named_refs.open(name);
      }
    void clear_named_references() noexcept
      {
        this->m_named_refs.clear();
      }

    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_Context * get_parent_opt() const noexcept = 0;
  };

}  // namespace Asteria

#endif
