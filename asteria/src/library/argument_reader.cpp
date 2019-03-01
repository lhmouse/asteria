// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Reference * Argument_Reader::do_peek_argument()
  {
    // Check for the end of operation.
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence no argument could be extracted any further.");
    }
    // Check for previous failures.
    if(!this->m_state.succeeded) {
      if(this->m_throw_on_failure) {
        ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");
      }
      this->m_state.succeeded = false;
      return nullptr;
    }
    // Check for the end of arguments.
    if(this->m_state.offset >= this->m_args.get().size()) {
      if(this->m_throw_on_failure) {
        ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", this->m_state.offset + 1, ").");
      }
      this->m_state.succeeded = false;
      return nullptr;
    }
    // Succeed.
    auto arg = this->m_args.get().data() + this->m_state.offset;
    ROCKET_ASSERT(arg);
    return arg;
  }

template<typename XvalueT> Argument_Reader & Argument_Reader::do_get_optional_value(XvalueT &value_out, const XvalueT &default_value)
  {
    // Record the type of this parameter.
    this->m_state.history.push_back(Value::Variant::index_of<XvalueT>::value);
    // Get the next argument.
    auto arg = this->do_peek_argument();
    if(!arg) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = arg->read();
    if(value.type() == type_null) {
      // If the value is `null`, set the default value.
      value_out = default_value;
    } else {
      // Not null...
      auto qvalue = value.opt<XvalueT>();
      if(!qvalue) {
        if(this->m_throw_on_failure) {
          ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                      "but `", Value::get_type_name<XvalueT>(), "` or `null` was expected.");
        }
        this->m_state.succeeded = false;
        return *this;
      }
      value_out = *qvalue;
    }
    // Succeed.
    this->m_state.offset++;
    return *this;
  }

template<typename XvalueT> Argument_Reader & Argument_Reader::do_get_required_value(XvalueT &value_out)
  {
    // Record the type of this parameter.
    this->m_state.history.push_back(Value::Variant::index_of<XvalueT>::value);
    // Get the next argument.
    auto arg = this->do_peek_argument();
    if(!arg) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = arg->read();
    // `null` is not an option.
    auto qvalue = value.opt<XvalueT>();
    if(!qvalue) {
      if(this->m_throw_on_failure) {
        ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                    "but `", Value::get_type_name<XvalueT>(), "` was expected.");
      }
      this->m_state.succeeded = false;
      return *this;
    }
    value_out = *qvalue;
    // Succeed.
    this->m_state.offset++;
    return *this;
  }

Argument_Reader & Argument_Reader::start() noexcept
  {
    this->m_state.history.clear();
    this->m_state.offset = 0;
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

Argument_Reader & Argument_Reader::opt(Reference &ref_out)
  {
    // Record a type-generic or output-only parameter.
    this->m_state.history.push_back(0xFF);
    // Get the next argument.
    auto arg = this->do_peek_argument();
    if(!arg) {
      return *this;
    }
    // Copy the reference as is.
    ref_out = *arg;
    // Succeed.
    this->m_state.offset++;
    return *this;
  }

Argument_Reader & Argument_Reader::opt(Value &value_out)
  {
    // Record a type-generic or output-only parameter.
    this->m_state.history.push_back(0xFF);
    // Get the next argument.
    auto arg = this->do_peek_argument();
    if(!arg) {
      return *this;
    }
    // Copy the value as is.
    value_out = arg->read();
    // Succeed.
    this->m_state.offset++;
    return *this;
  }

Argument_Reader & Argument_Reader::opt(D_boolean &value_out, D_boolean default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_integer &value_out, D_integer default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_real &value_out, D_real default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_string &value_out, const D_string &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_opaque &value_out, const D_opaque &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_function &value_out, const D_function &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_array &value_out, const D_array &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::opt(D_object &value_out, const D_object &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Reader & Argument_Reader::req(D_null &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_boolean &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_integer &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_real &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_string &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_opaque &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_function &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_array &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::req(D_object &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Reader & Argument_Reader::finish()
  {
    // Check for the end of operation.
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence cannot be finished a second time.");
    }
    // Record this overload, despite potential previous failures.
    unsigned nparams = static_cast<unsigned>(this->m_state.history.size());
    std::size_t offset = this->m_overloads.size();
    this->m_overloads.append(sizeof(nparams) + nparams);
    // 0) Append the number of parameters in native byte order.
    std::memcpy(this->m_overloads.mut_data() + offset, &nparams, sizeof(nparams));
    offset += sizeof(nparams);
    // 1) Append all parameters.
    std::memcpy(this->m_overloads.mut_data() + offset, this->m_state.history.data(), nparams);
    offset += nparams;
    ROCKET_ASSERT(offset == this->m_overloads.size());
    // Check for previous failures.
    if(!this->m_state.succeeded) {
      if(this->m_throw_on_failure) {
        ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");
      }
      this->m_state.succeeded = false;
      return *this;
    }
    // Check for the end of arguments.
    if(this->m_state.offset != this->m_args.get().size()) {
      if(this->m_throw_on_failure) {
        ASTERIA_THROW_RUNTIME_ERROR("Wrong number of arguments were provided (expecting exactly ", this->m_state.offset, ").");
      }
      this->m_state.succeeded = false;
      return *this;
    }
    // Succeed.
    this->m_state.finished = true;
    return *this;
  }

void Argument_Reader::throw_no_matching_function_call() const
  {
    const auto &name = this->m_name;
    const auto &args = this->m_args.get();
    // Create a message containing arguments.
    rocket::insertable_ostream mos;
    mos << "There was no matching overload for function call `" << name << "("
        << rocket::ostream_implode(args.data(), args.size(), ", ",
                                   [&](const Reference &arg) { return Value::get_type_name(arg.read().type());  })
        << ")`.";
    // If overload information is available, append the list of overloads.
    if(!this->m_overloads.empty()) {
      mos << "\n[list of overloads: ";
      // Decode overloads one by one.
      std::size_t offset = 0;
      unsigned nparams;
      for(;;) {
        // 0) Decode the number of parameters in native byte order.
        ROCKET_ASSERT(offset + sizeof(nparams) <= this->m_overloads.size());
        std::memcpy(&nparams, this->m_overloads.data() + offset, sizeof(nparams));
        offset += sizeof(nparams);
        // Append this overload.
        mos << "`" << name << "("
            << rocket::ostream_implode(this->m_overloads.data() + offset, nparams, ", ",
                                       [&](unsigned char param) { return (param == 0xFF) ? "<generic>"
                                                                                         : Value::get_type_name(static_cast<Value_Type>(param));  })
            << ")`";
        offset += nparams;
        // Break if there are no more data.
        if(offset == this->m_overloads.size()) {
          break;
        }
        // Read the next overload.
        mos << ", ";
      }
      mos << "]";
    }
    // Throw it now.
    throw_runtime_error(ROCKET_FUNCSIG, mos.extract_string());
  }

}
