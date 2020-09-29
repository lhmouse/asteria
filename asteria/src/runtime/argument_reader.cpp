// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../util.hpp"

namespace asteria {

Argument_Reader::
~Argument_Reader()
  {
  }

void
Argument_Reader::
do_record_parameter_required(Type type)
  {
    if(this->m_state.finished)
      ASTERIA_THROW("Argument reader finished and disposed");

    if(this->m_state.nparams)
      this->m_state.history << ", ";

    // Record a parameter.
    this->m_state.history << describe_type(type);
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_optional(Type type)
  {
    if(this->m_state.finished)
      ASTERIA_THROW("Argument reader finished and disposed");

    if(this->m_state.nparams)
      this->m_state.history << ", ";

    // Record a parameter.
    this->m_state.history << '[' << describe_type(type) << ']';
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_generic()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("Argument reader finished and disposed");

    if(this->m_state.nparams)
      this->m_state.history << ", ";

    // Record a parameter.
    this->m_state.history << "[generic]";
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_variadic()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("Argument reader finished and disposed");

    if(this->m_state.nparams)
      this->m_state.history << ", ";

    // Record the end of parameters.
    this->m_state.history << "...";
  }

void
Argument_Reader::
do_record_parameter_finish()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("Argument reader finished and disposed");

    // Terminate this overload.
    this->m_state.history.push_back('\0');
    this->m_ovlds.append(this->m_state.history);
  }

const Reference*
Argument_Reader::
do_peek_argument_opt()
const
  {
    if(!this->m_state.succeeded)
      return nullptr;

    // Before calling this function, the parameter information must have been recorded.
    size_t index = this->m_state.nparams - 1;
    return this->m_args.get().ptr(index);
  }

opt<size_t>
Argument_Reader::
do_check_finish_opt()
const
  {
    if(!this->m_state.succeeded)
      return nullopt;

    // Before calling this function, the current overload must have been finished.
    size_t index = this->m_state.nparams;
    return ::rocket::min(index, this->m_args.get().size());
  }

Argument_Reader&
Argument_Reader::
I()
noexcept
  {
    // Clear internal states.
    this->m_state.history.clear();
    this->m_state.nparams = 0;
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

bool
Argument_Reader::
F(cow_vector<Reference>& vargs)
  {
    this->do_record_parameter_variadic();
    this->do_record_parameter_finish();

    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff)
      return false;

    // Copy variadic arguments, if any.
    vargs.clear();
    auto nargs = this->m_args.get().size();
    if(nargs > *qvoff) {
      vargs.reserve(nargs - *qvoff);
      ::std::for_each(this->m_args.get().begin() + static_cast<ptrdiff_t>(*qvoff),
                      this->m_args.get().end(),
                      [&](const Reference& arg) {vargs.emplace_back(arg);  });
    }
    return true;
  }

bool
Argument_Reader::
F(cow_vector<Value>& vargs)
  {
    this->do_record_parameter_variadic();
    this->do_record_parameter_finish();

    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff)
      return false;

    // Copy variadic arguments, if any.
    vargs.clear();
    auto nargs = this->m_args.get().size();
    if(nargs > *qvoff) {
      vargs.reserve(nargs - *qvoff);
      ::std::for_each(this->m_args.get().begin() + static_cast<ptrdiff_t>(*qvoff),
                      this->m_args.get().end(),
                      [&](const Reference& arg) {vargs.emplace_back(arg.read());  });
    }
    return true;
  }

bool
Argument_Reader::
F()
  {
    this->do_record_parameter_finish();

    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff)
      return false;

    // There shall be no more arguments than parameters.
    auto nargs = this->m_args.get().size();
    if(nargs > *qvoff) {
      this->do_fail();
      return false;
    }
    return true;
  }

Argument_Reader&
Argument_Reader::
v(V_boolean& xval)
  {
    xval = false;
    this->do_record_parameter_required(type_boolean);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_boolean())
      return this->do_fail();

    xval = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_integer& xval)
  {
    xval = 0;
    this->do_record_parameter_required(type_integer);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_integer())
      return this->do_fail();

    xval = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_real& xval)
  {
    xval = 0;
    this->do_record_parameter_required(type_real);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_convertible_to_real())
      return this->do_fail();

    xval = val.convert_to_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_string& xval)
  {
    xval.clear();
    this->do_record_parameter_required(type_string);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_string())
      return this->do_fail();

    xval = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_opaque& xval)
  {
    xval.reset();
    this->do_record_parameter_required(type_opaque);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_opaque())
      return this->do_fail();

    xval = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_function& xval)
  {
    xval.reset();
    this->do_record_parameter_required(type_function);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_function())
      return this->do_fail();

    xval = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_array& xval)
  {
    xval.clear();
    this->do_record_parameter_required(type_array);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_array())
      return this->do_fail();

    xval = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(V_object& xval)
  {
    xval.clear();
    this->do_record_parameter_required(type_object);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return this->do_fail();

    // If the value doesn't have the desired type, fail.
    const auto& val = karg->read();
    if(!val.is_object())
      return this->do_fail();

    xval = val.as_object();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Reference& ref)
  {
    ref = Reference::S_uninit();
    this->do_record_parameter_generic();

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    ref = *karg;
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Value& val)
  {
    val = V_null();
    this->do_record_parameter_generic();

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    val = karg->read();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_boolean& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_boolean);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_boolean())
      return this->do_fail();

    xopt = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_integer& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_integer);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_integer())
      return this->do_fail();

    xopt = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_real& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_real);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_convertible_to_real())
      return this->do_fail();

    xopt = val.convert_to_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_string& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_string);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_string())
      return this->do_fail();

    xopt = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_opaque& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_opaque);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_opaque())
      return this->do_fail();

    xopt = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_function& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_function);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_function())
      return this->do_fail();

    xopt = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_array& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_array);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_array())
      return this->do_fail();

    xopt = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Opt_object& xopt)
  {
    xopt.reset();
    this->do_record_parameter_optional(type_object);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg)
      return *this;

    const auto& val = karg->read();
    if(val.is_null())
      return *this;

    // If the value doesn't have the desired type, fail.
    if(!val.is_object())
      return this->do_fail();

    xopt = val.as_object();
    return *this;
  }

void
Argument_Reader::
throw_no_matching_function_call()
const
  {
    // Compose an argument list.
    cow_string arguments;
    if(this->m_args.get().size()) {
      arguments << this->m_args.get()[0].read().what_type();
      for(size_t k = 1;  k != this->m_args.get().size();  ++k)
        arguments << ", " << this->m_args.get()[k].read().what_type();
    }

    // Append the list of overloads.
    cow_string overloads;
    for(size_t k = 0;  k != this->m_ovlds.size();  ++k) {
      auto sh = ::rocket::sref(this->m_ovlds.c_str() + k);
      overloads << "  " << this->m_name << '(' << sh << ")\n";
      k += sh.length();
    }

    // Throw the exception now.
    ASTERIA_THROW("No matching function call for `$1($2)`\n"
                  "[list of overloads:\n"
                  "$3  -- end of list of overloads]",
                  this->m_name, arguments, overloads);
  }

}  // namespace asteria
