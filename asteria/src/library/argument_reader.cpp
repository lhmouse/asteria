// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

template<typename HandlerT> void Argument_Reader::do_fail(HandlerT&& handler)
  {
    if(this->m_throw_on_failure) {
      ::rocket::forward<HandlerT>(handler)();
    }
    this->m_state.succeeded = false;
  }

void Argument_Reader::do_record_parameter_required(Gtype gtype)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("argument reader finished and disposed");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", " << describe_gtype(gtype);
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_optional(Gtype gtype)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("argument reader finished and disposed");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", [" << describe_gtype(gtype) << ']';
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_generic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("argument reader finished and disposed");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", <generic>";
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_variadic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("argument reader finished and disposed");
    }
    // Terminate the parameter list.
    this->m_state.history << ", ...";
  }

void Argument_Reader::do_record_parameter_finish()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("argument reader finished and disposed");
    }
    // Terminate this overload.
    this->m_state.history.push_back('\0');
    // Append it to the overload list as a single operation.
    this->m_ovlds.append(this->m_state.history);
  }

const Reference* Argument_Reader::do_peek_argument_opt() const
  {
    if(!this->m_state.succeeded) {
      return nullptr;
    }
    // Before calling this function, the parameter information must have been recorded.
    auto index = this->m_state.nparams - 1;
    // Return a pointer to the argument at `index`.
    return this->m_args->get_ptr(index);
  }

opt<size_t> Argument_Reader::do_check_finish_opt() const
  {
    if(!this->m_state.succeeded) {
      return clear;
    }
    // Before calling this function, the current overload must have been finished.
    auto index = this->m_state.nparams;
    // Return the beginning of variadic arguments.
    return ::rocket::min(index, this->m_args->size());
  }

Argument_Reader& Argument_Reader::I() noexcept
  {
    // Clear internal states.
    this->m_state.history.clear();
    this->m_state.nparams = 0;
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

bool Argument_Reader::F(cow_vector<Reference>& vargs)
  {
    this->do_record_parameter_variadic();
    this->do_record_parameter_finish();
    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff) {
      return false;
    }
    // Initialize the argument vector.
    vargs.clear();
    // Copy variadic arguments, if any.
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      ::rocket::ranged_for(*qvoff, nargs, [&](size_t i) { vargs.emplace_back(this->m_args->data()[i]);  });
    }
    return true;
  }

bool Argument_Reader::F(cow_vector<Value>& vargs)
  {
    this->do_record_parameter_variadic();
    this->do_record_parameter_finish();
    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff) {
      return false;
    }
    // Initialize the argument vector.
    vargs.clear();
    // Copy variadic arguments, if any.
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      ::rocket::ranged_for(*qvoff, nargs, [&](size_t i) { vargs.emplace_back(this->m_args->data()[i].read());  });
    }
    return true;
  }

bool Argument_Reader::F()
  {
    this->do_record_parameter_finish();
    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff) {
      return false;
    }
    // There shall be no more arguments than parameters.
    auto nargs = this->m_args->size();
    if(nargs > *qvoff) {
      this->do_fail([&]{ ASTERIA_THROW("too many arguments (`$1` > `$2`)", nargs, *qvoff);  });
      return false;
    }
    return true;
  }

Argument_Reader& Argument_Reader::g(Bval& xval)
  {
    this->do_record_parameter_required(gtype_boolean);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_boolean()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `boolean`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Ival& xval)
  {
    this->do_record_parameter_required(gtype_integer);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_integer()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `integer`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Rval& xval)
  {
    this->do_record_parameter_required(gtype_real);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_convertible_to_real()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `integer` or `real`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Sval& xval)
  {
    this->do_record_parameter_required(gtype_string);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_string()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `string`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Pval& xval)
  {
    this->do_record_parameter_required(gtype_opaque);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_opaque()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `opaque`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Fval& xval)
  {
    this->do_record_parameter_required(gtype_function);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_function()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `function`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Aval& xval)
  {
    this->do_record_parameter_required(gtype_array);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_array()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `array`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Oval& xval)
  {
    this->do_record_parameter_required(gtype_object);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("no more arguments");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(!val.is_object()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `object`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xval = val.as_object();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Reference& ref)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      ref = Reference_Root::S_void();
      return *this;
    }
    // Copy the reference as is.
    ref = *karg;
    return *this;
  }

Argument_Reader& Argument_Reader::g(Value& val)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      val = nullptr;
      return *this;
    }
    // Copy the val as is.
    val = karg->read();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Bopt& xopt)
  {
    this->do_record_parameter_optional(gtype_boolean);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_boolean()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `boolean` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Iopt& xopt)
  {
    this->do_record_parameter_optional(gtype_integer);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_integer()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `integer` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Ropt& xopt)
  {
    this->do_record_parameter_optional(gtype_real);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_convertible_to_real()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `integer`, `real` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Sopt& xopt)
  {
    this->do_record_parameter_optional(gtype_string);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_string()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `string` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Popt& xopt)
  {
    this->do_record_parameter_optional(gtype_opaque);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_opaque()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `opaque` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Fopt& xopt)
  {
    this->do_record_parameter_optional(gtype_function);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_function()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `function` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Aopt& xopt)
  {
    this->do_record_parameter_optional(gtype_array);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_array()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `array` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Oopt& xopt)
  {
    this->do_record_parameter_optional(gtype_object);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      xopt.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& val = karg->read();
    if(val.is_null()) {
      // Accept a `null` argument.
      xopt.reset();
      return *this;
    }
    if(!val.is_object()) {
      // If the val doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("unexpected type of argument `$1` (expecting `object` or `null`, got `$2`)",
                                       karg - this->m_args->data() + 1, val.what_gtype());  });
      return *this;
    }
    // Copy the value.
    xopt = val.as_object();
    return *this;
  }

void Argument_Reader::throw_no_matching_function_call() const
  {
    // Create a message containing all arguments.
    cow_string args;
    if(!this->m_args->empty()) {
      size_t k = 0;
      for(;;) {
        args << this->m_args.get()[k].read().what_gtype();
        // Seek to the next argument.
        if(++k == this->m_args->size())
          break;
        args << ", ";
      }
    }
    // Append the list of overloads.
    cow_string ovlds;
    if(!this->m_ovlds.empty()) {
      ovlds << "\n[list of overloads:\n  ";
      size_t k = 0;
      for(;;) {
        ovlds << '`' << this->m_name << '(';
        // Get the current parameter list.
        const char* s = this->m_ovlds.data() + k;
        size_t n = ::std::strlen(s);
        // If the parameter list is not empty, it always start with a ", ".
        if(n != 0) {
          ovlds.append(s + 2, n - 2);
          k += n;
        }
        ovlds << ')' << '`';
        // Seek to the next overload.
        if(++k == this->m_ovlds.size())
          break;
        ovlds << ",\n  ";
      }
      ovlds << ']';
    }
    // Throw the exception now.
    ASTERIA_THROW("no matching function call for `$1($2)`$3", this->m_name, args, ovlds);
  }

}  // namespace Asteria
