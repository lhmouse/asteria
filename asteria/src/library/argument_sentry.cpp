// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

Argument_sentry::~Argument_sentry()
  {
  }

    namespace {

    template<typename ThrowerT>
      void do_fail(const Argument_sentry &parent, Argument_sentry::State &state, ThrowerT &&thrower)
      {
        // If exceptions are preferred, throw an exception. Do not set `state.succeeded` in this case.
        if(parent.does_throw_on_failure()) {
          std::forward<ThrowerT>(thrower)();
        }
        // Error codes are preferred.
        state.succeeded = false;
      }

    class Reference_sentry
      {
      private:
        std::reference_wrapper<const Argument_sentry> m_parent;
        std::reference_wrapper<Argument_sentry::State> m_state;

        bool m_committable;
        const Reference *m_ref_opt;

      public:
        Reference_sentry(const Argument_sentry &parent, Argument_sentry::State &state, bool cut) noexcept
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
                  ASTERIA_THROW_RUNTIME_ERROR("This `Argument_sentry` had been finished hence no argument could be extracted any further.");
                });
              return;
            }
            if(cut) {
              // The argument list should stop here.
              if(state.offset != state.args->size()) {
                do_fail(parent, state,
                  [&]{
                    ASTERIA_THROW_RUNTIME_ERROR("Wrong number of arguments were provided (expecting ", state.args->size(), ").");
                  });
                return;
              }
              // Succeed.
              this->m_ref_opt = nullptr;
            } else {
              // Get an argument.
              if(state.offset >= state.args->size()) {
                do_fail(parent, state,
                  [&]{
                    ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", state.args->size(), ").");
                  });
                return;
              }
              // Succeed.
              this->m_ref_opt = state.args->data() + state.offset;
            }
            this->m_committable = true;
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_sentry)
          {
            Argument_sentry::State &state = this->m_state;
            // If anything went wrong, don't do anything.
            if(!state.succeeded) {
              return;
            }
            // If an argument has been read consumed, bump up the index.
            state.offset += (!this->m_committable && this->m_ref_opt);
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
        const Reference * ref_opt() const noexcept
          {
            return this->m_ref_opt;
          }
      };

    }

template<typename XvalueT>
  Argument_sentry & Argument_sentry::do_get_optional_value(XvalueT &value_out, const XvalueT &default_value)
  {
    Reference_sentry sentry(*this, this->m_state, false);
    if(!sentry) {
      return *this;
    }
    const auto ref = sentry.ref_opt();
    ROCKET_ASSERT(ref);
    const auto &value = ref->read();
    if(value.type() == Value::type_null) {
      // If the value is `null`, set the default value.
      value_out = default_value;
    } else {
      // Check whether the value has the desired type.
      const auto opt = value.opt<XvalueT>();
      if(!opt) {
        do_fail(*this, this->m_state,
          [&]{
            ASTERIA_THROW_RUNTIME_ERROR("The optional argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                        "but `", Value::get_type_name(Value::Type(Value::Variant::index_of<XvalueT>::value)), "` was expected.");
          });
        return *this;
      }
      value_out = *opt;
    }
    // Succeed.
    sentry.commit();
    return *this;
  }

template<typename XvalueT>
  Argument_sentry & Argument_sentry::do_get_required_value(XvalueT &value_out)
  {
    Reference_sentry sentry(*this, this->m_state, false);
    if(!sentry) {
      return *this;
    }
    const auto ref = sentry.ref_opt();
    ROCKET_ASSERT(ref);
    const auto &value = ref->read();
    // Check whether the value has the desired type.
    const auto opt = value.opt<XvalueT>();
    if(!opt) {
      do_fail(*this, this->m_state,
        [&]{
          ASTERIA_THROW_RUNTIME_ERROR("The required argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                      "but `", Value::get_type_name(Value::Type(Value::Variant::index_of<XvalueT>::value)), "` was expected.");
        });
      return *this;
    }
    value_out = *opt;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_sentry & Argument_sentry::opt(Reference &ref_out)
  {
    Reference_sentry sentry(*this, this->m_state, false);
    if(!sentry) {
      return *this;
    }
    const auto ref = sentry.ref_opt();
    ROCKET_ASSERT(ref);
    // Copy the reference as is.
    ref_out = *ref;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_sentry & Argument_sentry::opt(Value &value_out)
  {
    Reference_sentry sentry(*this, this->m_state, false);
    if(!sentry) {
      return *this;
    }
    const auto ref = sentry.ref_opt();
    ROCKET_ASSERT(ref);
    const auto &value = ref->read();
    // Copy the value as is.
    value_out = value;
    // Succeed.
    sentry.commit();
    return *this;
  }

Argument_sentry & Argument_sentry::opt(D_boolean &value_out, D_boolean default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_integer &value_out, D_integer default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_real &value_out, D_real default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_string &value_out, const D_string &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_opaque &value_out, const D_opaque &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_function &value_out, const D_function &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_array &value_out, const D_array &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::opt(D_object &value_out, const D_object &default_value)
  {
    return this->do_get_optional_value(value_out, default_value);
  }

Argument_sentry & Argument_sentry::req(D_null &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_boolean &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_integer &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_real &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_string &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_opaque &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_function &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_array &value_out)
  {
    return this->do_get_required_value(value_out);
  }

Argument_sentry & Argument_sentry::req(D_object &value_out)
  {
    return this->do_get_required_value(value_out);
  }


Argument_sentry & Argument_sentry::cut()
  {
    Reference_sentry sentry(*this, this->m_state, true);
    if(!sentry) {
      return *this;
    }
    // Succeed.
    sentry.commit();
    return *this;
  }

}
