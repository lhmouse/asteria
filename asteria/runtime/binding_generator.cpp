// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "binding_generator.hpp"
#include "argument_reader.hpp"
#include "reference.hpp"
#include "../utils.hpp"
namespace asteria {

cow_function
Binding_Generator::
operator->*(target_R_gsa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_R_gsa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_R_gsa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            self = this->m_target(global, move(self), move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_R_ga& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_R_ga* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_R_ga* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            self = this->m_target(global, move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_R_sa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_R_sa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_R_sa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            self = this->m_target(move(self), move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_R_a& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_R_a* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_R_a* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            self = this->m_target(move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_V_gsa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_V_gsa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_V_gsa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            auto result = this->m_target(global, move(self), move(reader));
            return self.set_temporary(move(result));
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_V_ga& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_V_ga* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_V_ga* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            auto result = this->m_target(global, move(reader));
            return self.set_temporary(move(result));
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_V_sa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_V_sa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_V_sa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            auto result = this->m_target(move(self), move(reader));
            return self.set_temporary(move(result));
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_V_a& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_V_a* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_V_a* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            auto result = this->m_target(move(reader));
            return self.set_temporary(move(result));
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_Z_gsa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_Z_gsa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_Z_gsa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            this->m_target(global, move(self), move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_Z_ga& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_Z_ga* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_Z_ga* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            this->m_target(global, move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_Z_sa& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_Z_sa* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_Z_sa* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            this->m_target(move(self), move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

cow_function
Binding_Generator::
operator->*(target_Z_a& target) const
  {
    struct Thunk : Abstract_Function
      {
        cow_string::shallow_type m_name;
        const char* m_func;
        const char* m_file;
        int m_line;
        target_Z_a* m_target;

        explicit
        Thunk(const Binding_Generator* gen, target_Z_a* target)
          :
            m_name(gen->m_name), m_func(gen->m_func), m_file(gen->m_file),
            m_line(gen->m_line), m_target(target)
          {
          }

        tinyfmt&
        describe(tinyfmt& fmt) const override
          {
            return format(fmt, "`$1` at '$2:$3'", this->m_func, this->m_file, this->m_line);
          }

        void
        collect_variables(Variable_HashMap&, Variable_HashMap&) const override
          {
          }

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack) const override
          {
            Argument_Reader reader(this->m_name, move(stack));
            this->m_target(move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(this, target);
  }

}  // namespace asteria
