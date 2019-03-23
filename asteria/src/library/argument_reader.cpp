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

    namespace {

    /* Bits in a byte are described as follows:
     * ------------------------------------------
     * type enum  -------------+
     * generic    --------+    |
     * required   ------+ |  __^__
     *                  | | /     \
     * bit index    7 6 5 4 3 2 1 0
     */

    constexpr std::uint8_t do_encode_typed_param(Value_Type type, bool required) noexcept
      {
        return static_cast<std::uint8_t>((static_cast<int>(type) & 0x0F) | (required << 5));
      }
    constexpr std::uint8_t do_encode_generic_param(bool required) noexcept
      {
        return static_cast<std::uint8_t>((1 << 4) | (required << 5));
      }

    struct Decoded_Param
      {
        Value_Type type;
        bool generic;
        bool required;

        explicit constexpr Decoded_Param(std::uint8_t byte) noexcept
          : type(static_cast<Value_Type>(byte & 0x0F)),
            generic((byte >> 4) & 1), required((byte >> 5) & 1)
          {
          }
      };

    inline std::ostream& operator<<(std::ostream& os, const Decoded_Param& p)
      {
        if(p.generic) {
          // type-generic parameter (and optional always)
          return os << "<generic>";
        }
        if(p.required) {
          // required parameter
          return os << Value::get_type_name(p.type);
        }
        // optional parameter
        return os << '[' << Value::get_type_name(p.type) << ']';
      }

    }

const Reference* Argument_Reader::do_peek_argument_optional_opt()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence no argument could be extracted any further.");
    }
    // Check for previous failures.
    if(!this->m_state.succeeded) {
      return nullptr;
    }
    // Before calling this function, the parameter information must have been recorded in `m_state.prototype`.
    auto index = this->m_state.prototype.size() - 1;
    if(index >= this->m_args.get().size()) {
      return nullptr;
    }
    auto qarg = this->m_args.get().data() + index;
    ROCKET_ASSERT(qarg);
    return qarg;
  }

template<typename XvalueT> Argument_Reader& Argument_Reader::do_read_typed_argument_optional(XvalueT& xvalue)
  {
    // Record a parameter.
    constexpr auto xtype = static_cast<Value_Type>(Value::Xvariant::index_of<XvalueT>::value);
    this->m_state.prototype.emplace_back(do_encode_typed_param(xtype, false));
    // Get the next argument.
    auto qarg = this->do_peek_argument_optional_opt();
    if(!qarg) {
      return *this;
    }
    // Read a value from the argument.
    const auto& value = qarg->read();
    if(value.type() != type_null) {
      if(value.type() != xtype) {
        // If the value doesn't have the desired type, fail.
        this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.prototype.size(), " had type `", Value::get_type_name(value.type()), "`, "
                                                       "but `", Value::get_type_name(xtype), "` or `null` was expected.");  });
        return *this;
      }
      // Copy the value.
      xvalue = value.check<XvalueT>();
    }
    return *this;
  }

const Reference* Argument_Reader::do_peek_argument_required_opt()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence no argument could be extracted any further.");
    }
    // Check for previous failures.
    if(!this->m_state.succeeded) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
      return nullptr;
    }
    // Before calling this function, the parameter information must have been recorded in `m_state.prototype`.
    auto index = this->m_state.prototype.size() - 1;
    if(index >= this->m_args.get().size()) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", index + 1, ").");  });
      return nullptr;
    }
    auto qarg = this->m_args.get().data() + index;
    ROCKET_ASSERT(qarg);
    return qarg;
  }

template<typename XvalueT> Argument_Reader& Argument_Reader::do_read_typed_argument_required(XvalueT& xvalue)
  {
    // Record a parameter.
    constexpr auto xtype = static_cast<Value_Type>(Value::Xvariant::index_of<XvalueT>::value);
    this->m_state.prototype.emplace_back(do_encode_typed_param(xtype, true));
    // Get the next argument.
    auto qarg = this->do_peek_argument_required_opt();
    if(!qarg) {
      return *this;
    }
    // Read a value from the argument.
    const auto& value = qarg->read();
    if(value.type() != xtype) {
      // If the value doesn't have the desired type, fail.
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.prototype.size(), " had type `", Value::get_type_name(value.type()), "`, "
                                                     "but `", Value::get_type_name(xtype), "` was expected.");  });
      return *this;
    }
    // Copy the value.
    xvalue = value.check<XvalueT>();
    return *this;
  }

Argument_Reader& Argument_Reader::start() noexcept
  {
    // Clear any internal states.
    this->m_state.prototype.clear();
    this->m_state.finished = false;
    this->m_state.succeeded = true;
    return *this;
  }

Argument_Reader& Argument_Reader::opt(Reference& ref)
  {
    // Record a parameter.
    this->m_state.prototype.emplace_back(do_encode_generic_param(false));
    // Get the next argument.
    auto qarg = this->do_peek_argument_optional_opt();
    if(!qarg) {
      return *this;
    }
    // Copy the reference as is.
    ref = *qarg;
    return *this;
  }

Argument_Reader& Argument_Reader::opt(Value& value)
  {
    // Record a parameter.
    this->m_state.prototype.emplace_back(do_encode_generic_param(false));
    // Get the next argument.
    auto qarg = this->do_peek_argument_optional_opt();
    if(!qarg) {
      return *this;
    }
    // Read a value from the argument and copy it as is.
    value = qarg->read();
    return *this;
  }

Argument_Reader& Argument_Reader::opt(D_boolean& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_integer& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_real& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_string& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_opaque& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_function& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_array& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::opt(D_object& xvalue)
  {
    return this->do_read_typed_argument_optional(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_boolean& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_integer& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_real& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_string& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_opaque& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_function& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_array& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::req(D_object& xvalue)
  {
    return this->do_read_typed_argument_required(xvalue);
  }

Argument_Reader& Argument_Reader::finish()
  {
    if(this->m_state.finished) {
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence cannot be finished a second time.");
    }
    // Record this overload.
    std::size_t offset = this->m_overloads.size();
    std::size_t nparams = this->m_state.prototype.size();
    this->m_overloads.append(sizeof(nparams) + nparams);
    // Append the number of parameters in native byte order.
    std::memcpy(this->m_overloads.mut_data() + offset, &nparams, sizeof(nparams));
    offset += sizeof(nparams);
    if(nparams != 0) {
      // Append all parameters.
      std::memcpy(this->m_overloads.mut_data() + offset, this->m_state.prototype.data(), nparams);
      offset += nparams;
    }
    ROCKET_ASSERT(offset == this->m_overloads.size());
    // Check for previous failures.
    if(!this->m_state.succeeded) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
      return *this;
    }
    // There shall be no more arguments than parameters.
    if(nparams < this->m_args.get().size()) {
      this->do_fail([&]{ ASTERIA_THROW_RUNTIME_ERROR("Too many arguments were provided for this overload (expecting no more than `", nparams, "`, "
                                                     "but got `", this->m_args.get().size(), "`).");  });
      return *this;
    }
    // Accept the end of argument list.
    return *this;
  }

void Argument_Reader::throw_no_matching_function_call() const
  {
    const auto& name = this->m_name;
    const auto& args = this->m_args.get();
    // Create a message containing arguments.
    rocket::insertable_ostream mos;
    mos << "There was no matching overload for function call `" << name << "("
        << rocket::ostream_implode(args.data(), args.size(), ", ",
                                   [](const Reference& arg) { return Value::get_type_name(arg.read().type());  })
        << ")`.";
    // If overload information is available, append the list of overloads.
    if(!this->m_overloads.empty()) {
      mos << "\n[list of overloads: ";
      // Decode overloads one by one.
      std::size_t offset = 0;
      std::size_t nparams;
      for(;;) {
        // 0) Decode the number of parameters in native byte order.
        ROCKET_ASSERT(offset + sizeof(nparams) <= this->m_overloads.size());
        std::memcpy(&nparams, this->m_overloads.data() + offset, sizeof(nparams));
        offset += sizeof(nparams);
        // Append this overload.
        mos << "`" << name << "("
            << rocket::ostream_implode(this->m_overloads.data() + offset, nparams, ", ",
                                       [](std::uint8_t byte) { return Decoded_Param(byte);  })
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
    throw_runtime_error(__func__, mos.extract_string());
  }

}  // namespace Asteria
