// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

Argument_Sentry::~Argument_Sentry()
  {
  }

    namespace {

    template<typename ThrowerT>
      void do_fail(const Argument_Sentry &parent, Argument_Sentry::State &state, ThrowerT &&thrower)
      {
        // If exceptions are preferred, throw an exception. Do not set `state.succeeded` in this case.
        if(parent.does_throw_on_failure()) {
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
                [&]{
                  ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");
                });
              return;
            }
            if(state.finished) {
              do_fail(parent, state,
                [&]{
                  ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had been finished hence no argument could be extracted any further.");
                });
              return;
            }
            // Get an argument.
            if(state.offset >= state.args->size()) {
              do_fail(parent, state,
                [&]{
                  ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", state.args->size(), ").");
                });
              return;
            }
            // Succeed.
            this->m_ref = state.args->data() + state.offset;
            this->m_committable = true;
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_Sentry)
          {
            Argument_Sentry::State &state = this->m_state;
            // If anything went wrong, don't do anything.
            if(!state.succeeded) {
              return;
            }
            // If an argument has been read consumed, bump up the index.
            state.offset += !this->m_committable;
          }

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

template<typename XvalueT>
  Argument_Sentry & Argument_Sentry::do_get_optional_value(XvalueT &value_out, const XvalueT &default_value)
  {
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = sentry.ref().read();
    if(value.type() == Value::type_null) {
      // If the value is `null`, set the default value.
      value_out = default_value;
    } else {
      // Not null...
      const auto qvalue = value.opt<XvalueT>();
      if(!qvalue) {
        do_fail(*this, this->m_state,
          [&]{
            ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                        "but `", Value::get_type_name(Value::Type(Value::Variant::index_of<XvalueT>::value)), "` or `null` was expected.");
          });
        return *this;
      }
      value_out = *qvalue;
    }
    // Succeed.
    sentry.commit();
    return *this;
  }

template<typename XvalueT>
  Argument_Sentry & Argument_Sentry::do_get_required_value(XvalueT &value_out)
  {
    Reference_Sentry sentry(*this, this->m_state);
    if(!sentry) {
      return *this;
    }
    // Check whether the value has the desired type.
    const auto &value = sentry.ref().read();
    const auto qvalue = value.opt<XvalueT>();
    if(!qvalue) {
      do_fail(*this, this->m_state,
        [&]{
          ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                      "but `", Value::get_type_name(Value::Type(Value::Variant::index_of<XvalueT>::value)), "` was expected.");
        });
      return *this;
    }
    value_out = *qvalue;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_Sentry & Argument_Sentry::opt(Reference &ref_out)
  {
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

Argument_Sentry & Argument_Sentry::cut()
  {
    auto &state = this->m_state;
    // Check for general conditions.
    if(!state.succeeded) {
      do_fail(*this, state,
        [&]{
          ASTERIA_THROW_RUNTIME_ERROR("A previous operation had failed.");
        });
      return *this;
    }
    if(state.finished) {
      do_fail(*this, state,
        [&]{
          ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had been finished hence no argument could be extracted any further.");
        });
      return *this;
    }
    // Check for the end of the argument list.
    if(state.offset != state.args->size()) {
      do_fail(*this, state,
        [&]{
           ASTERIA_THROW_RUNTIME_ERROR("Wrong number of arguments were provided (expecting exactly ", state.args->size(), ").");
        });
      return *this;
    }
    // Succeed.
    state.finished = true;
    return *this;
  }

[[noreturn]] void Argument_Sentry::throw_no_matching_function_call(const Overload_Parameter *overload_data, std::size_t overload_size) const
  {
    const auto args = this->m_state.args;
    if(!args) {
      // Hmmm you have to call `.reset()` first.
      ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had not been initialized yet.");
    }
    // Create a message containing arguments.
    Formatter msg;
    ASTERIA_FORMAT(msg, "There was no matching overload for function call `", this->m_name, "(");
    // Append the types of all arguments.
    auto size = args->size();
    if(size != 0) {
      // Deal with the nasty commas.
      auto ptr = args->data();
      Value value;
      do {
        value = ptr->read();
        ASTERIA_FORMAT(msg, Value::get_type_name(value.type()));
        ++ptr;
        if(--size == 0) {
          break;
        }
        ASTERIA_FORMAT(msg, ", ");
      } while(true);
    }
    ASTERIA_FORMAT(msg, ")`.");
    // If an overload list is provided, append it.
    size = overload_size;
    if(size != 0) {
      ASTERIA_FORMAT(msg, "\n(possible overload(s): ");
      // Collect all overloads.
      auto ptr = overload_data;
      for(;;) {
        auto nparams = static_cast<std::size_t>(ptr->nparams);
        ++ptr;
        --size;
        ROCKET_ASSERT_MSG(nparams <= size, "Overload list data were malformed.");
        // Assemble the function prototype.
        ASTERIA_FORMAT(msg, "`", this->m_name, "(");
        if(nparams != 0) {
          // Yay, nasty commas again.
          size -= nparams;
          do {
            ASTERIA_FORMAT(msg, (ptr->nparams == 0xFF) ? "<any>" : Value::get_type_name(ptr->type));
            ++ptr;
            if(--nparams == 0) {
              break;
            }
            ASTERIA_FORMAT(msg, ", ");
          } while(true);
        }
        ASTERIA_FORMAT(msg, ")`");
        // Finish the parameter list of this overload.
        if(size == 0) {
          break;
        }
        ASTERIA_FORMAT(msg, ", ");
      }
      ASTERIA_FORMAT(msg, ")");
    }
    throw_runtime_error(ROCKET_FUNCSIG, std::move(msg));
  }

}
