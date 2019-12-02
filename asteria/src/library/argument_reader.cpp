// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

template<typename HandlerT> void Argument_Reader::do_fail(HandlerT&& handler)
  {
    if(this->m_throw_on_failure) {
      rocket::forward<HandlerT>(handler)();
    }
    this->m_state.succeeded = false;
  }

void Argument_Reader::do_record_parameter_required(Gtype gtype)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", " << describe_gtype(gtype);
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_optional(Gtype gtype)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", [" << describe_gtype(gtype) << ']';
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_generic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    // Record a parameter and increment the number of parameters in total.
    this->m_state.history << ", <generic>";
    this->m_state.nparams++;
  }

void Argument_Reader::do_record_parameter_variadic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    // Terminate the parameter list.
    this->m_state.history << ", ...";
  }

void Argument_Reader::do_record_parameter_finish()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW("This argument sentry had already been finished; it cannot be finished a second time.");
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
      return rocket::clear;
    }
    // Before calling this function, the current overload must have been finished.
    auto index = this->m_state.nparams;
    // Return the beginning of variadic arguments.
    return rocket::min(index, this->m_args->size());
  }

Argument_Reader& Argument_Reader::start() noexcept
  {
    // Clear internal states.
    this->m_state.history.clear();
    this->m_state.nparams = 0;
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

Argument_Reader& Argument_Reader::g(Reference& ref)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      ref = Reference_Root::S_null();
      return *this;
    }
    // Copy the reference as is.
    ref = *karg;
    return *this;
  }

Argument_Reader& Argument_Reader::g(Value& value)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      value = G_null();
      return *this;
    }
    // Copy the value as is.
    value = karg->read();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_boolean>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_boolean);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_boolean()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `boolean` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_integer>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_integer);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_integer()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `integer` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_real>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_real);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_convertible_to_real()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `integer`, `real` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_string>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_string);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_string()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `string` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_opaque>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_opaque);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_opaque()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `opaque` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_function>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_function);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_function()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `function` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_array>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_array);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_array()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `array` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(opt<G_object>& qxvalue)
  {
    this->do_record_parameter_optional(gtype_object);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      qxvalue.reset();
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(value.is_null()) {
      // Accept a `null` argument.
      qxvalue.reset();
      return *this;
    }
    if(!value.is_object()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `object` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_object();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_boolean& xvalue)
  {
    this->do_record_parameter_required(gtype_boolean);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_boolean()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `boolean` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_integer& xvalue)
  {
    this->do_record_parameter_required(gtype_integer);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_integer()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `integer` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_real& xvalue)
  {
    this->do_record_parameter_required(gtype_real);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_convertible_to_real()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `integer` or `real` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_string& xvalue)
  {
    this->do_record_parameter_required(gtype_string);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_string()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `string` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_opaque& xvalue)
  {
    this->do_record_parameter_required(gtype_opaque);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_opaque()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `opaque` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_function& xvalue)
  {
    this->do_record_parameter_required(gtype_function);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_function()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `function` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_array& xvalue)
  {
    this->do_record_parameter_required(gtype_array);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_array()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `array` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_object& xvalue)
  {
    this->do_record_parameter_required(gtype_object);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_object()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW("Argument ", karg - this->m_args->data() + 1, " had type `", value.what_gtype(), "`, "
                                                     "but `object` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_object();
    return *this;
  }

bool Argument_Reader::finish(cow_vector<Reference>& vargs)
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
    if(*qvoff < this->m_args->size()) {
      rocket::ranged_for(*qvoff, this->m_args->size(), [&](size_t i) { vargs.emplace_back(this->m_args.get()[i]);  });
    }
    return true;
  }

bool Argument_Reader::finish(cow_vector<Value>& vargs)
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
    if(*qvoff < this->m_args->size()) {
      rocket::ranged_for(*qvoff, this->m_args->size(), [&](size_t i) { vargs.emplace_back(this->m_args.get()[i].read());  });
    }
    return true;
  }

bool Argument_Reader::finish()
  {
    this->do_record_parameter_finish();
    // Get the number of named parameters.
    auto qvoff = this->do_check_finish_opt();
    if(!qvoff) {
      return false;
    }
    // There shall be no more arguments than parameters.
    if(*qvoff < this->m_args->size()) {
      this->do_fail([&]{ ASTERIA_THROW("Too many arguments were provided (got `", this->m_args->size(), "`, "
                                                     "but expecting no more than `", *qvoff, "`).");  });
      return false;
    }
    return true;
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
        size_t n = std::strlen(s);
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
    rocket::sprintf_and_throw<std::invalid_argument>("no matching function call for `%s(%s)`%s",
                                                     this->m_name.c_str(), args.c_str(), ovlds.c_str());
  }

}  // namespace Asteria
