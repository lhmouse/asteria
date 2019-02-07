// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  private:
    // This is the global garbage collector.
    RefCnt_Ptr<RefCnt_Base> m_collector;
    // This is the variable holding an object referenced as `std` in this context.
    RefCnt_Ptr<RefCnt_Base> m_std_var;

  public:
    // A global context does not have a parent context.
    Global_Context()
      : Abstract_Context()
      {
        this->do_initialize_runtime();
      }
    ~Global_Context() override;

  private:
    void do_initialize_runtime();

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Abstract_Context * get_parent_opt() const noexcept override
      {
        return nullptr;
      }

    // These are interfaces of the global garbage collector/
    RefCnt_Ptr<Variable> create_variable() const;
    bool collect_variables(unsigned gen_limit = 0x7F) const;

    // These are interfaces of the standard library.
    const Value & get_std_member(const PreHashed_String &name) const;
    Value & open_std_member(const PreHashed_String &name);
    template<typename XvalueT, ROCKET_ENABLE_IF(std::is_convertible<XvalueT, Value>::value)> void set_std_member(const PreHashed_String &name, XvalueT &&xvalue)
      {
        this->open_std_member(name) = std::forward<XvalueT>(xvalue);
      }
    bool unset_std_member(const PreHashed_String &name);
  };

}

#endif
