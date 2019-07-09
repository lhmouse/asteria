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
    Gtype gtype;

    void print(std::ostream& os) const
      {
        switch(this->tag) {
        case tag_optional:
          {
            os << '[' << Value::get_gtype_name(this->gtype) << ']';
            return;
          }
        case tag_required:
          {
            os << Value::get_gtype_name(this->gtype);
            return;
          }
        case tag_generic:
          {
            os << "<generic>";
            return;
          }
        case tag_variadic:
          {
            os << "...";
            return;
          }
        case tag_finish:
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

void Argument_Reader::do_record_parameter(Gtype gtype, bool required)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    Mparam pinfo = { };
    pinfo.tag = required ? Mparam::tag_required : Mparam::tag_optional;
    pinfo.gtype = gtype;
    this->m_state.prototype.emplace_back(pinfo);
  }

void Argument_Reader::do_record_parameter_generic()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; no argument could be extracted any further.");
    }
    Mparam pinfo = { };
    pinfo.tag = Mparam::tag_generic;
    this->m_state.prototype.emplace_back(pinfo);
  }

void Argument_Reader::do_record_parameter_finish(bool variadic)
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished; it cannot be finished a second time.");
    }
    Mparam pinfo = { };
    if(variadic) {
      pinfo.tag = Mparam::tag_variadic;
      this->m_state.prototype.emplace_back(pinfo);
    }
    pinfo.tag = Mparam::tag_finish;
    this->m_state.prototype.emplace_back(pinfo);
    // Append this prototype.
    this->m_overloads.append(this->m_state.prototype.begin(), this->m_state.prototype.end());
  }

const Reference* Argument_Reader::do_peek_argument_opt() const
  {
    if(!this->m_state.succeeded) {
      return nullptr;
    }
    // Before calling this function, the parameter information must have been recorded in `m_state.prototype`.
    auto nparams = this->m_state.prototype.size();
    if(this->m_args.get().size() < nparams) {
      return nullptr;
    }
    // Return a pointer to this argument.
    return &(this->m_args.get().at(nparams - 1));
  }

Opt<std::ptrdiff_t> Argument_Reader::do_check_finish_opt(bool variadic) const
  {
    if(!this->m_state.succeeded) {
      return rocket::nullopt;
    }
    // Before calling this function, a finish tag must have been recorded in `m_state.prototype`.
    auto nparams = this->m_state.prototype.size() - 1;
    return static_cast<std::ptrdiff_t>(nparams - variadic);
  }

Argument_Reader& Argument_Reader::start() noexcept
  {
    this->m_state.prototype.clear();
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

Argument_Reader& Argument_Reader::g(Opt<G_boolean>& qxvalue)
  {
    this->do_record_parameter(gtype_boolean, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `boolean` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_integer>& qxvalue)
  {
    this->do_record_parameter(gtype_integer, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `integer` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_real>& qxvalue)
  {
    this->do_record_parameter(gtype_real, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `integer`, `real` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_string>& qxvalue)
  {
    this->do_record_parameter(gtype_string, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `string` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_opaque>& qxvalue)
  {
    this->do_record_parameter(gtype_opaque, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `opaque` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_function>& qxvalue)
  {
    this->do_record_parameter(gtype_function, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `function` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_array>& qxvalue)
  {
    this->do_record_parameter(gtype_array, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `array` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(Opt<G_object>& qxvalue)
  {
    this->do_record_parameter(gtype_object, false);
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
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `object` or `null` was expected.");  });
      return *this;
    }
    // Copy the value.
    qxvalue = value.as_object();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_boolean& xvalue)
  {
    this->do_record_parameter(gtype_boolean, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_boolean()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `boolean` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_boolean();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_integer& xvalue)
  {
    this->do_record_parameter(gtype_integer, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_integer()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `integer` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_integer();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_real& xvalue)
  {
    this->do_record_parameter(gtype_real, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_convertible_to_real()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `integer` or `real` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.convert_to_real();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_string& xvalue)
  {
    this->do_record_parameter(gtype_string, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_string()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `string` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_string();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_opaque& xvalue)
  {
    this->do_record_parameter(gtype_opaque, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_opaque()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `opaque` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_opaque();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_function& xvalue)
  {
    this->do_record_parameter(gtype_function, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_function()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `function` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_function();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_array& xvalue)
  {
    this->do_record_parameter(gtype_array, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_array()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `array` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_array();
    return *this;
  }

Argument_Reader& Argument_Reader::g(G_object& xvalue)
  {
    this->do_record_parameter(gtype_object, true);
    // Get the next argument.
    auto karg = this->do_peek_argument_opt();
    if(!karg) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No argument is available.");  });
      return *this;
    }
    // Read a value from the argument.
    const auto& value = karg->read();
    if(!value.is_object()) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", karg - this->m_args.get().data() + 1, " had type `", value.gtype_name(), "`, "
                                                     "but `object` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.as_object();
    return *this;
  }

bool Argument_Reader::finish()
  {
    this->do_record_parameter_finish(false);
    // Get the number of named parameters.
    auto qoff = this->do_check_finish_opt(false);
    if(!qoff) {
      return false;
    }
    // There shall be no more arguments than parameters.
    if(*qoff < this->m_args.get().ssize()) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Too many arguments were provided (expecting no more than `", *qoff, "`, "
                                                     "but got `", this->m_args.get().size(), "`).");  });
      return false;
    }
    return true;
  }

bool Argument_Reader::finish(Cow_Vector<Reference>& vargs)
  {
    this->do_record_parameter_finish(true);
    // Get the number of named parameters.
    auto qoff = this->do_check_finish_opt(true);
    if(!qoff) {
      return false;
    }
    // Copy variadic arguments as is.
    vargs.clear();
    if(*qoff < this->m_args.get().ssize()) {
      std::for_each(this->m_args.get().begin() + *qoff, this->m_args.get().end(), [&](const Reference& arg) { vargs.emplace_back(arg);  });
    }
    return true;
  }

bool Argument_Reader::finish(Cow_Vector<Value>& vargs)
  {
    this->do_record_parameter_finish(true);
    // Get the number of named parameters.
    auto qoff = this->do_check_finish_opt(true);
    if(!qoff) {
      return false;
    }
    // Copy variadic arguments as is.
    vargs.clear();
    if(*qoff < this->m_args.get().ssize()) {
      std::for_each(this->m_args.get().begin() + *qoff, this->m_args.get().end(), [&](const Reference& arg) { vargs.emplace_back(arg.read());  });
    }
    return true;
  }

void Argument_Reader::throw_no_matching_function_call() const
  {
    const auto& name = this->m_name;
    const auto& args = this->m_args.get();
    // Create a message containing arguments.
    Cow_osstream mos;
    mos << "There was no matching overload for function call `" << name << "(";
    if(!args.empty()) {
      mos << args.front().read().gtype_name();
      std::for_each(args.begin() + 1, args.end(), [&](const Reference& arg) { mos <<", " << arg.read().gtype_name();  });
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
          bpos->print(mos);
          std::for_each(bpos + 1, epos, [&](const Mparam& pinfo) { pinfo.print(mos << ", ");  });
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
