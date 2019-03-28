// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

struct Argument_Reader::Mparam
  {
    enum Tag : std::uint8_t
      {
        tag_optional  = 0,  // optional, statically typed
        tag_required  = 1,  // required, statically typed
        tag_generic   = 2,  // optional, dynamically typed
        tag_variadic  = 3,  // variadic argument placeholder
        tag_finish    = 4,  // end of parameter list
      };

    Tag tag;
    Dtype dtype;

    void print(std::ostream& os) const
      {
        switch(this->tag) {
        case tag_optional:
          {
            os << '[' << Value::get_type_name(this->dtype) << ']';
            return;
          }
        case Mparam::tag_required:
          {
            os << Value::get_type_name(this->dtype);
            return;
          }
        case Mparam::tag_generic:
          {
            os << "<generic>";
            return;
          }
        case Mparam::tag_variadic:
          {
            os << "...";
            return;
          }
        case Mparam::tag_finish:
          {
            ROCKET_ASSERT(false);
          }
        default:
          ROCKET_ASSERT(false);
        }
      }
  };

template<typename HandlerT> void Argument_Reader::do_fail(HandlerT&& handler)
  {
    if(this->m_throw_on_failure) {
      rocket::forward<HandlerT>(handler)();
    }
    this->m_state.succeeded = false;
  }

void Argument_Reader::do_record_parameter(Dtype dtype, bool required)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    Mparam pinfo;
    if(required) {
      pinfo.tag = Mparam::tag_required;
    } else {
      pinfo.tag = Mparam::tag_optional;
    }
    pinfo.dtype = dtype;
    this->m_state.prototype.emplace_back(pinfo);
  }

void Argument_Reader::do_record_parameter_generic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    Mparam pinfo;
    pinfo.tag = Mparam::tag_generic;
    this->m_state.prototype.emplace_back(pinfo);
  }

void Argument_Reader::do_record_parameter_finish(bool variadic)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; it cannot be finished a second time.");
    }
    Mparam pinfo;
    if(variadic) {
      pinfo.tag = Mparam::tag_variadic;
      this->m_state.prototype.emplace_back(pinfo);
    }
    pinfo.tag = Mparam::tag_finish;
    this->m_state.prototype.emplace_back(pinfo);
    // Append this prototype.
    this->m_overloads.append(this->m_state.prototype.begin(), this->m_state.prototype.end());
  }

const Reference* Argument_Reader::do_peek_argument_opt(bool required)
  {
    if(!this->m_state.succeeded) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
      return nullptr;
    }
    // Before calling this function, the parameter information must have been recorded in `m_state.prototype`.
    auto nparams = this->m_state.prototype.size();
    if(this->m_args.get().size() < nparams) {
      if(!required) {
        return nullptr;
      }
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", nparams, ").");  });
      return nullptr;
    }
    // Return a pointer to this argument.
    auto karg = this->m_args.get().data() + (nparams - 1);
    ROCKET_ASSERT(karg);
    return karg;
  }

Optional<std::size_t> Argument_Reader::do_check_finish_opt(bool variadic)
  {
    if(!this->m_state.succeeded) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
      return rocket::nullopt;
    }
    // Before calling this function, the parameter information must have been recorded in `m_state.prototype`.
    auto nparams = this->m_state.prototype.size() - 1;
    if(variadic) {
      nparams -= 1;
    }
    return nparams;
  }

template<Dtype dtypeT> Argument_Reader& Argument_Reader::do_read_typed_argument_optional(typename Value::Xvariant::type_at<dtypeT>::type& xvalue)
  {
    this->do_record_parameter(dtypeT, false);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt(false);
    if(!karg) {
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    auto qrvalue = value.opt<typename Value::Xvariant::type_at<dtypeT>::type>();
    if(!qrvalue) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.prototype.size(), " had type `", Value::get_type_name(value.type()), "`, "
                                                     "but `", Value::get_type_name(dtypeT), "` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = *qrvalue;
    return *this;
  }

template<Dtype dtypeT> Argument_Reader& Argument_Reader::do_read_typed_argument_required(typename Value::Xvariant::type_at<dtypeT>::type& xvalue)
  {
    // Record this parameter.
    this->do_record_parameter(dtypeT, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt(true);
    if(!karg) {
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    auto qrvalue = value.opt<typename Value::Xvariant::type_at<dtypeT>::type>();
    if(!qrvalue) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.prototype.size(), " had type `", Value::get_type_name(value.type()), "`, "
                                                     "but `", Value::get_type_name(dtypeT), "` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = *qrvalue;
    return *this;
  }

Argument_Reader& Argument_Reader::start() noexcept
  {
    this->m_state.prototype.clear();
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

Argument_Reader& Argument_Reader::opt(Reference& ref)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt(false);
    if(!karg) {
      return *this;
    }
    // Copy the reference as is.
    ref = *karg;
    return *this;
  }

Argument_Reader& Argument_Reader::opt(Value& value)
  {
    this->do_record_parameter_generic();
    // Get the next argument.
    auto karg = this->do_peek_argument_opt(false);
    if(!karg) {
      return *this;
    }
    // Copy the value as is.
    value = karg->read();
    return *this;
  }

Argument_Reader& Argument_Reader::opt(D_boolean& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_boolean>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_integer& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_integer>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_real& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_real>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_string& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_string>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_opaque& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_opaque>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_function& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_function>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_array& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_array>(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_object& xvalue)
  {
    return this->do_read_typed_argument_optional<dtype_object>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_boolean& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_boolean>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_integer& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_integer>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_real& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_real>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_string& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_string>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_opaque& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_opaque>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_function& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_function>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_array& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_array>(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_object& xvalue)
  {
    return this->do_read_typed_argument_required<dtype_object>(xvalue);
  }

Argument_Reader& Argument_Reader::finish()
  {
    this->do_record_parameter_finish(false);
    // Get the number of named parameters.
    auto knparams = this->do_check_finish_opt(false);
    if(!knparams) {
      return *this;
    }
    // There shall be no more arguments than parameters.
    if(*knparams < this->m_args.get().size()) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Too many arguments were provided (expecting no more than `", *knparams, "`, "
                                                     "but got `", this->m_args.get().size(), "`).");  });
      return *this;
    }
    // Accept the end of arguments.
    return *this;
  }

Argument_Reader& Argument_Reader::finish(Cow_Vector<Reference>& vargs)
  {
    this->do_record_parameter_finish(true);
    // Get the number of named parameters.
    auto knparams = this->do_check_finish_opt(true);
    if(!knparams) {
      return *this;
    }
    // Copy variadic arguments as is.
    vargs.clear();
    if(*knparams < this->m_args.get().size()) {
      std::for_each(this->m_args.get().begin() + static_cast<std::ptrdiff_t>(*knparams), this->m_args.get().end(),
                    [&](const Reference& arg) { vargs.emplace_back(arg);  });
    }
    // Accept the end of arguments.
    return *this;
  }

Argument_Reader& Argument_Reader::finish(Cow_Vector<Value>& vargs)
  {
    this->do_record_parameter_finish(true);
    // Get the number of named parameters.
    auto knparams = this->do_check_finish_opt(true);
    if(!knparams) {
      return *this;
    }
    // Copy variadic arguments as is.
    vargs.clear();
    if(*knparams < this->m_args.get().size()) {
      std::for_each(this->m_args.get().begin() + static_cast<std::ptrdiff_t>(*knparams), this->m_args.get().end(),
                    [&](const Reference& arg) { vargs.emplace_back(arg.read());  });
    }
    // Accept the end of arguments.
    return *this;
  }

void Argument_Reader::throw_no_matching_function_call() const
  {
    const auto& name = this->m_name;
    const auto& args = this->m_args.get();
    // Create a message containing arguments.
    rocket::insertable_ostream mos;
    mos << "There was no matching overload for function call `" << name << "(";
    if(!args.empty()) {
      std::for_each(args.begin(), args.end() - 1, [&](const Reference& arg) { mos << Value::get_type_name(arg.read().type()) <<", ";  });
      mos << Value::get_type_name(args.back().read().type());
    }
    mos << ")`.";
    // If overload information is available, append the list of overloads.
    if(!this->m_overloads.empty()) {
      mos << "\n[list of overloads: ";
      // Decode overloads one by one.
      auto bpos = this->m_overloads.begin();
      for(;;) {
        auto epos = std::find_if(bpos, this->m_overloads.end(), [&](const Mparam& pinfo) { return pinfo.tag == Mparam::tag_finish;  });
        // Append this overload.
        mos << "`" << name << "(";
        if(bpos != epos) {
          std::for_each(bpos, epos - 1, [&](const Mparam& pinfo) { pinfo.print(mos);  });
          epos[-1].print(mos);
        }
        mos << ")`";
        // Are there more overloads?
        if(this->m_overloads.end() - epos <= 1) {
          break;
        }
        // Read the next overload.
        bpos = epos + 1;
        mos << ", ";
      }
      mos << "]";
    }
    // Throw it now.
    throw_runtime_error(__func__, mos.extract_string());
  }

}  // namespace Asteria
