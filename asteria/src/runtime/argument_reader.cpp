// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

void
Argument_Reader::
do_record_parameter_required(Vtype vtype)
  {
    if(this->m_state.finished)
      ASTERIA_THROW("argument reader finished and disposed");

    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", " << describe_vtype(vtype);
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_optional(Vtype vtype)
  {
    if(this->m_state.finished)
      ASTERIA_THROW("argument reader finished and disposed");

    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", [" << describe_vtype(vtype) << ']';
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_generic()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("argument reader finished and disposed");

    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", <generic>";
    this->m_state.nparams++;
  }

void
Argument_Reader::
do_record_parameter_variadic()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("argument reader finished and disposed");

    // Terminate the parameter list.
    this->m_state.history << ", ...";
  }

void
Argument_Reader::
do_record_parameter_finish()
  {
    if(this->m_state.finished)
      ASTERIA_THROW("argument reader finished and disposed");

    // Terminate this overload.
    this->m_state.history.push_back('\0');
    // Append it to the overload list as a single operation.
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
    auto index = this->m_state.nparams - 1;
    // Return a pointer to the argument at `index`.
    return this->m_args->get_ptr(index);
  }

opt<size_t>
Argument_Reader::
do_check_finish_opt()
const
  {
    if(!this->m_state.succeeded)
      return nullopt;

    // Before calling this function, the current overload must have been finished.
    auto index = this->m_state.nparams;
    // Return the beginning of variadic arguments.
    return ::rocket::min(index, this->m_args->size());
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
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      vargs.reserve(nargs - *qvoff);
      ::std::for_each(this->m_args->begin() + static_cast<ptrdiff_t>(*qvoff), this->m_args->end(),
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
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      vargs.reserve(nargs - *qvoff);
      ::std::for_each(this->m_args->begin() + static_cast<ptrdiff_t>(*qvoff), this->m_args->end(),
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
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      this->m_state.succeeded = false;
      return false;
    }
    return true;
  }

Argument_Reader&
Argument_Reader::
v(Bval& xval)
  {
    this->do_record_parameter_required(vtype_boolean);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_boolean()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Ival& xval)
  {
    this->do_record_parameter_required(vtype_integer);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_integer()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Rval& xval)
  {
    this->do_record_parameter_required(vtype_real);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_convertible_to_real()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.convert_to_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Sval& xval)
  {
    this->do_record_parameter_required(vtype_string);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_string()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Pval& xval)
  {
    this->do_record_parameter_required(vtype_opaque);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_opaque()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Fval& xval)
  {
    this->do_record_parameter_required(vtype_function);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_function()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Aval& xval)
  {
    this->do_record_parameter_required(vtype_array);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_array()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
v(Oval& xval)
  {
    this->do_record_parameter_required(vtype_object);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->m_state.succeeded = false;
      return *this;
    }
    const auto& val = karg->read();

    // If the value doesn't have the desired type, fail.
    if(!val.is_object()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xval = val.as_object();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Reference& ref)
  {
    this->do_record_parameter_generic();

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      ref = Reference_root::S_constant();
      return *this;
    }
    ref = *karg;
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Value& val)
  {
    this->do_record_parameter_generic();

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      val = nullptr;
      return *this;
    }
    val = karg->read();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Bopt& xopt)
  {
    this->do_record_parameter_optional(vtype_boolean);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_boolean()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_boolean();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Iopt& xopt)
  {
    this->do_record_parameter_optional(vtype_integer);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_integer()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_integer();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Ropt& xopt)
  {
    this->do_record_parameter_optional(vtype_real);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_convertible_to_real()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.convert_to_real();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Sopt& xopt)
  {
    this->do_record_parameter_optional(vtype_string);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_string()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_string();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Popt& xopt)
  {
    this->do_record_parameter_optional(vtype_opaque);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_opaque()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_opaque();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Fopt& xopt)
  {
    this->do_record_parameter_optional(vtype_function);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_function()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_function();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Aopt& xopt)
  {
    this->do_record_parameter_optional(vtype_array);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_array()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_array();
    return *this;
  }

Argument_Reader&
Argument_Reader::
o(Oopt& xopt)
  {
    this->do_record_parameter_optional(vtype_object);

    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    const auto& val = karg->read();
    if(val.is_null()) {
      xopt.reset();
      return *this;
    }

    // If the value doesn't have the desired type, fail.
    if(!val.is_object()) {
      this->m_state.succeeded = false;
      return *this;
    }
    xopt = val.as_object();
    return *this;
  }

void
Argument_Reader::
throw_no_matching_function_call()
const
  {
    // Create a message containing all arguments.
    cow_string args_str;
    const auto& args = this->m_args.get();
    if(!args.empty()) {
      size_t k = 0;
      for(;;) {
        args_str << args[k].read().what_vtype();
        // Seek to the next argument.
        if(++k == args.size())
          break;
        args_str << ", ";
      }
    }

    // Append the list of overloads.
    cow_string ovlds_str;
    const auto& ovlds = this->m_ovlds;
    if(!ovlds.empty()) {
      ovlds_str << "\n[list of overloads:\n  ";
      size_t k = 0;
      for(;;) {
        ovlds_str << '`' << this->m_name << '(';
        // Get the current parameter list.
        const char* sparams = ovlds.data() + k;
        size_t nchars = ::std::strlen(sparams);
        // If the parameter list is not empty, it always start with a ", " so skip it.
        if(nchars != 0)
          ovlds_str.append(sparams + 2, nchars - 2);
        k += nchars;
        ovlds_str << ')' << '`';
        // Seek to the next overload.
        if(++k == ovlds.size())
          break;
        ovlds_str << "\n  ";
      }
      ovlds_str << "\n  -- end of list of overloads]";
    }

    // Throw the exception now.
    ASTERIA_THROW("no matching function call for `$1($2)`$3", this->m_name, args_str, ovlds_str);
  }

}  // namespace Asteria
