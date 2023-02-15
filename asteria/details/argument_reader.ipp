// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ARGUMENT_READER_
#  error Please include <asteria/runtime/argument_reader.hpp> instead.
#endif
namespace asteria {
namespace details_argument_reader {

// This struct records all parameters of an overload.
// It can be copied elsewhere and back; any further operations will
// resume from that point.
struct State
  {
    cow_string params;
    uint32_t nparams = 0;
    bool ended = false;
    bool matched = false;
  };

// This is a tag type that helps pulling in factory operators.
struct Factory
  {
    cow_string::shallow_type name;
    cow_string::shallow_type desc;
  };

template<typename FunctionT>
class Thunk_Base
  : public Abstract_Function
  {
  protected:
    cow_string::shallow_type m_name;
    cow_string::shallow_type m_desc;
    FunctionT* m_func;

  public:
    explicit
    Thunk_Base(const Factory& fact, FunctionT func)
      : m_name(fact.name), m_desc(fact.desc), m_func(func)  { }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      { return fmt << this->m_desc.c_str();  }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }
  };

using F_ref_global_self_reader   = Reference (Global_Context&, Reference&&, Argument_Reader&&);
using F_ref_global_reader        = Reference (Global_Context&, Argument_Reader&&);
using F_ref_self_reader          = Reference (Reference&&, Argument_Reader&&);
using F_ref_reader               = Reference (Argument_Reader&&);

using F_val_global_self_reader   = Value (Global_Context&, Reference&&, Argument_Reader&&);
using F_val_global_reader        = Value (Global_Context&, Argument_Reader&&);
using F_val_self_reader          = Value (Reference&&, Argument_Reader&&);
using F_val_reader               = Value (Argument_Reader&&);

using F_void_global_self_reader  = void (Global_Context&, Reference&&, Argument_Reader&&);
using F_void_global_reader       = void (Global_Context&, Argument_Reader&&);
using F_void_self_reader         = void (Reference&&, Argument_Reader&&);
using F_void_reader              = void (Argument_Reader&&);

cow_function
operator%(const Factory& fact, F_ref_global_self_reader func);

cow_function
operator%(const Factory& fact, F_ref_global_reader func);

cow_function
operator%(const Factory& fact, F_ref_self_reader func);

cow_function
operator%(const Factory& fact, F_ref_reader func);

cow_function
operator%(const Factory& fact, F_val_global_self_reader func);

cow_function
operator%(const Factory& fact, F_val_global_reader func);

cow_function
operator%(const Factory& fact, F_val_self_reader func);

cow_function
operator%(const Factory& fact, F_val_reader func);

cow_function
operator%(const Factory& fact, F_void_global_self_reader func);

cow_function
operator%(const Factory& fact, F_void_global_reader func);

cow_function
operator%(const Factory& fact, F_void_self_reader func);

cow_function
operator%(const Factory& fact, F_void_reader func);

}  // namespace details_argument_reader
}  // namespace asteria
