// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_BINDING_GENERATOR_
#define ASTERIA_RUNTIME_BINDING_GENERATOR_

#include "../fwd.hpp"
namespace asteria {

class Binding_Generator
  {
  public:
    using target_R_gsa  = Reference (Global_Context&, Reference&&, Argument_Reader&&);
    using target_R_ga   = Reference (Global_Context&, Argument_Reader&&);
    using target_R_sa   = Reference (Reference&&, Argument_Reader&&);
    using target_R_a    = Reference (Argument_Reader&&);

    using target_V_gsa  = Value (Global_Context&, Reference&&, Argument_Reader&&);
    using target_V_ga   = Value (Global_Context&, Argument_Reader&&);
    using target_V_sa   = Value (Reference&&, Argument_Reader&&);
    using target_V_a    = Value (Argument_Reader&&);

    using target_Z_gsa  = void (Global_Context&, Reference&&, Argument_Reader&&);
    using target_Z_ga   = void (Global_Context&, Argument_Reader&&);
    using target_Z_sa   = void (Reference&&, Argument_Reader&&);
    using target_Z_a    = void (Argument_Reader&&);

  private:
    cow_string::shallow_type m_name;
    const char* m_func;
    const char* m_file;
    int m_line;

  public:
    constexpr Binding_Generator(cow_string::shallow_type name, const char* func,
                                const char* file, int line) noexcept
      :
        m_name(name), m_func(func), m_file(file), m_line(line)
      { }

  public:
    // These functions are invoked by `ASTERIA_BINDING()`.
    cow_function
    operator->*(target_R_gsa& target) const;

    cow_function
    operator->*(target_R_ga& target) const;

    cow_function
    operator->*(target_R_sa& target) const;

    cow_function
    operator->*(target_R_a& target) const;

    cow_function
    operator->*(target_V_gsa& target) const;

    cow_function
    operator->*(target_V_ga& target) const;

    cow_function
    operator->*(target_V_sa& target) const;

    cow_function
    operator->*(target_V_a& target) const;

    cow_function
    operator->*(target_Z_gsa& target) const;

    cow_function
    operator->*(target_Z_ga& target) const;

    cow_function
    operator->*(target_Z_sa& target) const;

    cow_function
    operator->*(target_Z_a& target) const;
  };

// See 'library/string.cpp' for examples about how to use this macro.
#define ASTERIA_BINDING(name, params, ...)  \
    (::asteria::Binding_Generator(::rocket::sref("" name ""),  \
        ("" name "(" params ")"), __FILE__, __LINE__))->**[](__VA_ARGS__)

}  // namespace asteria
#endif
