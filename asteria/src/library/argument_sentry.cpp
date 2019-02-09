// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

Argument_Sentry & Argument_Sentry::start() noexcept
  {
    this->m_state.history.clear();
    this->m_state.offset = 0;
    this->m_state.succeeded = true;
    this->m_state.finished = false;
    return *this;
  }

    namespace {

    template<typename ThrowerT> inline void do_fail(const Argument_Sentry &parent, Argument_Sentry::State &state, ThrowerT &&thrower)
      {
        if(parent.does_throw_on_failure()) {
          // If exceptions are preferred, throw an exception. Do not set `state.succeeded` in this case.
          std::forward<ThrowerT>(thrower)();
        }
        // Error codes are preferred.
        state.succeeded = false;
      }

    class Reference_Sentry
      {
      private:
        std::reference_wrapper<const Argument_Sentry> m_parent;
        std::reference_wrapper<Argument_Sentry::State> m_state;

        bool m_committable;
        const Reference *m_ref;

      public:
        Reference_Sentry(const Argument_Sentry &parent, Argument_Sentry::State &state) noexcept
          : m_parent(std::ref(parent)), m_state(std::ref(state)),
            m_committable(false)
          {
            // Check for general conditions.
            if(!state.succeeded) {
              do_fail(parent, state,
                [&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
              return;
            }
            if(state.finished) {
              do_fail(parent, state,
                [&]{ ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence no argument could be extracted any further.");  });
              return;
            }
            // Get an argument.
            if(state.offset >= parent.get_argument_count()) {
              do_fail(parent, state,
                [&]{ ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", parent.get_argument_count(), ").");  });
              return;
            }
            // Succeed.
            this->m_ref = std::addressof(parent.get_argument(state.offset));
            this->m_committable = true;
          }
        ~Reference_Sentry()
          {
            Argument_Sentry::State &state = this->m_state;
            // If anything went wrong, don't do anything.
            if(!state.succeeded) {
              return;
            }
            // If the argument was not consumed, don't do anything.
            if(this->m_committable) {
              return;
            }
            // Bump up the index.
            ROCKET_ASSERT(state.offset < 0xFE);
            state.offset++;
          }

        Reference_Sentry(const Reference_Sentry &)
          = delete;
        Reference_Sentry & operator=(const Reference_Sentry &)
          = delete;

      public:
        explicit operator bool () const noexcept
          {
            return this->m_committable;
          }
        void commit() noexcept
          {
            ROCKET_ASSERT(this->m_committable);
            this->m_committable = false;
          }
        const Reference & ref() const noexcept
          {
            ROCKET_ASSERT(this->m_committable);
            return *(this->m_ref);
          }
      };

    }

template<typename XvalueT> Argument_Sentry & Argument_Sentry::do_get_optional_value(XvalueT &value_out, const XvalueT &default_value)
  {
    // Record the type of this parameter.
    this->m_state.history.push_back(Value::Variant::index_of<XvalueT>::value);
    // Get the next argument.
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = sentry.ref().read();
    if(value.type() == type_null) {
      // If the value is `null`, set the default value.
      value_out = default_value;
    } else {
      // Not null...
      const auto qvalue = value.opt<XvalueT>();
      if(!qvalue) {
        do_fail(*this, this->m_state,
          [&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                           "but `", Value::get_type_name<XvalueT>(), "` or `null` was expected.");  });
        return *this;
      }
      value_out = *qvalue;
    }
    // Succeed.
    sentry.commit();
    return *this;
  }

template<typename XvalueT> Argument_Sentry & Argument_Sentry::do_get_required_value(XvalueT &value_out)
  {
    // Record the type of this parameter.
    this->m_state.history.push_back(Value::Variant::index_of<XvalueT>::value);
    // Get the next argument.
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = sentry.ref().read();
    const auto qvalue = value.opt<XvalueT>();
    if(!qvalue) {
      do_fail(*this, this->m_state,
        [&]{ ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                         "but `", Value::get_type_name<XvalueT>(), "` was expected.");  });
      return *this;
    }
    value_out = *qvalue;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_Sentry & Argument_Sentry::opt(Reference &ref_out)
  {
    // Record a type-generic or output-only parameter.
    this->m_state.history.push_back(-1);
    // Get the next argument.
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Copy the reference as is.
    ref_out = sentry.ref();
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_Sentry & Argument_Sentry::opt(Value &value_out)
  {
    // Record a type-generic or output-only parameter.
    this->m_state.history.push_back(-1);
    // Get the next argument.
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Copy the value as is.
    const auto &value = sentry.ref().read();
    value_out = value;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_Sentry & Argument_Sentry::opt(D_boolean &value_out, D_boolean default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_integer &value_out, D_integer default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_real &value_out, D_real default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_string &value_out, const D_string &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_opaque &value_out, const D_opaque &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_function &value_out, const D_function &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_array &value_out, const D_array &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::opt(D_object &value_out, const D_object &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_Sentry & Argument_Sentry::req(D_null &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_boolean &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_integer &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_real &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_string &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_opaque &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_function &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_array &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::req(D_object &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_Sentry & Argument_Sentry::finish()
  {
    // Record this overload.
    // 0) Append the number of parameters in native byte order.
    unsigned nparams;
    nparams = static_cast<unsigned>(this->m_state.history.size());
    this->m_overloads.reserve(this->m_overloads.size() + sizeof(nparams) + nparams);
    this->m_overloads.append(reinterpret_cast<const std::int8_t *>(&nparams), sizeof(nparams));
    // 1) Append all parameters.
    this->m_overloads.append(this->m_state.history.data(), nparams);
    // Check for general conditions.
    if(!this->m_state.succeeded) {
      do_fail(*this, this->m_state,
        [&]{ ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");  });
      return *this;
    }
    if(this->m_state.finished) {
      do_fail(*this, this->m_state,
        [&]{ ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence could not be finished a second time.");  });
      return *this;
    }
    // Check for the end of the argument list.
    if(this->m_state.offset != this->get_argument_count()) {
      do_fail(*this, this->m_state,
        [&]{ ASTERIA_THROW_RUNTIME_ERROR("Wrong number of arguments were provided (expecting exactly ", this->get_argument_count(), ").");  });
      return *this;
    }
    // Succeed.
    this->m_state.finished = true;
    return *this;
  }

void Argument_Sentry::throw_no_matching_function_call() const
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
      mos << "\n[possible overloads: ";
      // Decode overloads one by one.
      unsigned nparams;
      auto read = this->m_overloads.copy(reinterpret_cast<std::int8_t *>(&nparams), sizeof(nparams));
      auto offset = read;
      for(;;) {
        ROCKET_ASSERT(read == sizeof(nparams));
        ROCKET_ASSERT(offset + nparams <= this->m_overloads.size());
        // Append this overload.
        mos << "`" << name << "("
            << rocket::ostream_implode(this->m_overloads.data() + offset, nparams, ", ",
                                       [&](std::int8_t param) { return (param >= 0) ? Value::get_type_name(static_cast<Value_Type>(param))
                                                                                    : "<generic>";  })
            << ")`";
        offset += nparams;
        read = this->m_overloads.copy(reinterpret_cast<std::int8_t *>(&nparams), sizeof(nparams), offset);
        if(read == 0) {
          break;
        }
        offset += read;
        // Read the next overload.
        mos << ", ";
      }
      mos << "]";
    }
    // Throw it now.
    throw_runtime_error(std::move(mos), ROCKET_FUNCSIG);
  }

}
