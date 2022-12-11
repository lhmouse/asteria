// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "argument_reader.hpp"
#include "runtime_error.hpp"
#include "../utils.hpp"

namespace asteria {

Argument_Reader::
~Argument_Reader()
  {
  }

void
Argument_Reader::
do_prepare_parameter(const char* param)
  {
    // Ensure `end_overload()` has not been called for this overload.
    if(this->m_state.ended)
      ASTERIA_THROW_RUNTIME_ERROR(("Current overload marked ended"));

    // Append the parameter.
    // If it is not the first one, insert a comma before it.
    if(this->m_state.nparams)
      this->m_state.params += ", ";

    this->m_state.params += param;
    this->m_state.nparams += 1;
  }

void
Argument_Reader::
do_terminate_parameter_list()
  {
    // Ensure `end_overload()` has not been called for this overload.
    if(this->m_state.ended)
      ASTERIA_THROW_RUNTIME_ERROR(("Current overload marked ended"));

    // Mark this overload ended.
    this->m_state.ended = true;

    this->m_overloads.append(this->m_state.params.data(),
            this->m_state.params.size() + 1);  // null terminator included
  }

Argument_Reader&
Argument_Reader::
do_mark_match_failure() noexcept
  {
    // Set the current overload as unmatched.
    this->m_state.matched = false;
    return *this;
  }

const Reference*
Argument_Reader::
do_peek_argument() const
  {
    // Try getting an argument for the next parameter.
    // Prior to this function, `do_prepare_parameter()` shall have been called.
    if(!this->m_state.matched)
      return nullptr;

    size_t rindex = this->m_stack.size() - this->m_state.nparams;
    if(rindex >= this->m_stack.size())
      return nullptr;

    return &(this->m_stack.top(rindex));
  }

Argument_Reader&
Argument_Reader::load_state(size_t index)
  {
    this->m_state = this->m_saved_states.at(index);
    return *this;
  }

Argument_Reader&
Argument_Reader::
save_state(size_t index)
  {
    this->m_saved_states.append(subsat(index + 1, this->m_saved_states.size()));
    this->m_saved_states.mut(index) = this->m_state;
    return *this;
  }

Argument_Reader&
Argument_Reader::
start_overload() noexcept
  {
    this->m_state.params.clear();
    this->m_state.nparams = 0;
    this->m_state.ended = false;
    this->m_state.matched = true;
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(Reference& out)
  {
    out.set_invalid();
    this->do_prepare_parameter("[reference]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Copy the argument reference as is.
    out = *qref;
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(Value& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[value]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and copy the value as is.
    out = qref->dereference_readonly();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_boolean& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[boolean]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_boolean())
      return this->do_mark_match_failure();

    out = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_integer& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[integer]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_integer())
      return this->do_mark_match_failure();

    out = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_real& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[real]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_real())
      return this->do_mark_match_failure();

    out = val.as_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_string& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[string]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_string())
      return this->do_mark_match_failure();

    out = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_opaque& out)
  {
    out = nullptr;
    this->do_prepare_parameter("[opaque]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_opaque())
      return this->do_mark_match_failure();

    out = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_function& out)
  {
    out = nullptr;
    this->do_prepare_parameter("[function]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_function())
      return this->do_mark_match_failure();

    out = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_array& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[array]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_array())
      return this->do_mark_match_failure();

    out = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
optional(optV_object& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[object]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return *this;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return *this;

    if(!val.is_object())
      return this->do_mark_match_failure();

    out = val.as_object();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_boolean& out)
  {
    out = false;
    this->do_prepare_parameter("boolean");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_boolean())
      return this->do_mark_match_failure();

    out = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_integer& out)
  {
    out = 0;
    this->do_prepare_parameter("integer");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_integer())
      return this->do_mark_match_failure();

    out = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_real& out)
  {
    out = 0.0;
    this->do_prepare_parameter("real");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_real())
      return this->do_mark_match_failure();

    out = val.as_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_string& out)
  {
    out.clear();
    this->do_prepare_parameter("string");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_string())
      return this->do_mark_match_failure();

    out = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_opaque& out)
  {
    out = nullptr;
    this->do_prepare_parameter("opaque");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_opaque())
      return this->do_mark_match_failure();

    out = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_function& out)
  {
    out = nullptr;
    this->do_prepare_parameter("function");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_function())
      return this->do_mark_match_failure();

    out = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_array& out)
  {
    out.clear();
    this->do_prepare_parameter("array");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_array())
      return this->do_mark_match_failure();

    out = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
required(V_object& out)
  {
    out.clear();
    this->do_prepare_parameter("object");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_object())
      return this->do_mark_match_failure();

    out = val.as_object();
    return *this;
  }

bool
Argument_Reader::
end_overload()
  {
    this->do_terminate_parameter_list();

    if(!this->m_state.matched)
      return false;

    // Ensure no more arguments follow. Note there may be fewer.
    size_t nargs = this->m_state.nparams;
    nargs = subsat(this->m_stack.size(), nargs);
    if(nargs != 0) {
      this->do_mark_match_failure();
      return false;
    }

    // Accept all arguments.
    return true;
  }

bool
Argument_Reader::
end_overload(cow_vector<Reference>& vargs)
  {
    vargs.clear();
    this->do_prepare_parameter("...");
    this->do_terminate_parameter_list();

    if(!this->m_state.matched)
      return false;

    // Check for variadic arguments. Note the `...` is not a parameter.
    size_t nargs = this->m_state.nparams;
    nargs = subsat(this->m_stack.size(), nargs - 1);
    if(nargs != 0) {
      vargs.reserve(nargs);
      while(nargs != 0)
        vargs.emplace_back(this->m_stack.top(--nargs));
    }

    // Accept all arguments.
    return true;
  }

bool
Argument_Reader::
end_overload(cow_vector<Value>& vargs)
  {
    vargs.clear();
    this->do_prepare_parameter("...");
    this->do_terminate_parameter_list();

    if(!this->m_state.matched)
      return false;

    // Check for variadic arguments. Note the `...` is not a parameter.
    size_t nargs = this->m_state.nparams;
    nargs = subsat(this->m_stack.size(), nargs - 1);
    if(nargs != 0) {
      vargs.reserve(nargs);
      while(nargs != 0)
        vargs.emplace_back(this->m_stack.top(--nargs).dereference_readonly());
    }

    // Accept all arguments.
    return true;
  }

void
Argument_Reader::
throw_no_matching_function_call() const
  {
    // Compose the list of types of arguments.
    cow_string arguments;
    size_t index = this->m_stack.size();
    switch(index) {
        do {
          arguments += ", ";  // fallthrough
      default:
          arguments += describe_type(
                this->m_stack.top(--index).dereference_readonly().type());
        }
        while(index != 0);  // fallthrough

      case 0:
        break;
    }

    // Compose the list of overloads.
    cow_string overloads;
    index = 0;
    while(index != this->m_overloads.size()) {
      const char* ovr = this->m_overloads.c_str() + index;
      size_t len = ::std::strlen(ovr);
      index += len + 1;

      overloads += "  ";
      overloads += this->m_name;
      overloads += "(";
      overloads.append(ovr, len);
      overloads += ")\n";
    }

    // Throw the exception now.
    ASTERIA_THROW_RUNTIME_ERROR((
        "No matching function call for `$1($2)`",
        "[list of overloads:\n$3  -- end of list of overloads]"),
        this->m_name, arguments, overloads);
  }

// Binding factory operators
cow_function
details_argument_reader::
operator%(const Factory& fact, F_ref_global_self_reader func)
  {
    struct Thunk : Thunk_Base<F_ref_global_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            self = this->m_func(global, ::std::move(self), ::std::move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_ref_global_reader func)
  {
    struct Thunk : Thunk_Base<F_ref_global_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            self = this->m_func(global, ::std::move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_ref_self_reader func)
  {
    struct Thunk : Thunk_Base<F_ref_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            self = this->m_func(::std::move(self), ::std::move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_ref_reader func)
  {
    struct Thunk : Thunk_Base<F_ref_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            self = this->m_func(::std::move(reader));
            return self;
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_val_global_self_reader func)
  {
    struct Thunk : Thunk_Base<F_val_global_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            auto val = this->m_func(global, ::std::move(self), ::std::move(reader));
            return self.set_temporary(::std::move(val));
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_val_global_reader func)
  {
    struct Thunk : Thunk_Base<F_val_global_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            auto val = this->m_func(global, ::std::move(reader));
            return self.set_temporary(::std::move(val));
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_val_self_reader func)
  {
    struct Thunk : Thunk_Base<F_val_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            auto val = this->m_func(::std::move(self), ::std::move(reader));
            return self.set_temporary(::std::move(val));
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_val_reader func)
  {
    struct Thunk : Thunk_Base<F_val_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            auto val = this->m_func(::std::move(reader));
            return self.set_temporary(::std::move(val));
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_void_global_self_reader func)
  {
    struct Thunk : Thunk_Base<F_void_global_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            this->m_func(global, ::std::move(self), ::std::move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_void_global_reader func)
  {
    struct Thunk : Thunk_Base<F_void_global_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& global,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            this->m_func(global, ::std::move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_void_self_reader func)
  {
    struct Thunk : Thunk_Base<F_void_self_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            this->m_func(::std::move(self), ::std::move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

cow_function
details_argument_reader::
operator%(const Factory& fact, F_void_reader func)
  {
    struct Thunk : Thunk_Base<F_void_reader>
      {
        using Thunk_Base::Thunk_Base;

        Reference&
        invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                         Reference_Stack&& args) const override
          {
            Argument_Reader reader(this->m_name, ::std::move(args));
            this->m_func(::std::move(reader));
            return self.set_void();
          }
      };

    return ::rocket::make_refcnt<Thunk>(fact, func);
  }

}  // namespace asteria
