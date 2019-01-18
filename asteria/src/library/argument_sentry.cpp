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
                  ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence no argument could be extracted any further.");
                });
              return;
            }
            // Get an argument.
            if(state.offset >= parent.get_argument_count()) {
              do_fail(parent, state,
                [&]{
                  ASTERIA_THROW_RUNTIME_ERROR("No enough arguments were provided (expecting at least ", parent.get_argument_count(), ").");
                });
              return;
            }
            // Succeed.
            this->m_ref = std::addressof(parent.get_argument(state.offset));
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
    if(value.type() == type_null) {
      // If the value is `null`, set the default value.
      value_out = default_value;
    } else {
      // Not null...
      const auto qvalue = value.opt<XvalueT>();
      if(!qvalue) {
        do_fail(*this, this->m_state,
          [&]{
            ASTERIA_THROW_RUNTIME_ERROR("Argument ", this->m_state.offset + 1, " had type `", Value::get_type_name(value.type()), "`, "
                                        "but `", Value::get_type_name<XvalueT>(), "` or `null` was expected.");
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
                                      "but `", Value::get_type_name<XvalueT>(), "` was expected.");
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
          ASTERIA_THROW_RUNTIME_ERROR("This argument sentry had already been finished, hence could not be finished a second time.");
        });
      return *this;
    }
    // Check for the end of the argument list.
    if(state.offset != this->get_argument_count()) {
      do_fail(*this, state,
        [&]{
           ASTERIA_THROW_RUNTIME_ERROR("Wrong number of arguments were provided (expecting exactly ", this->get_argument_count(), ").");
        });
      return *this;
    }
    // Succeed.
    state.finished = true;
    return *this;
  }

    namespace {

    template<typename IteratorT, typename FilterT>
      class Type_Name_Imploder
      {
      private:
        IteratorT m_begin;
        std::size_t m_count;

        FilterT m_filter;

      public:
        constexpr Type_Name_Imploder(IteratorT xbegin, std::size_t xcount, FilterT xfilter)
          : m_begin(std::move(xbegin)), m_count(xcount),
            m_filter(std::move(xfilter))
          {
          }

      public:
        const IteratorT & begin() const noexcept
          {
            return this->m_begin;
          }
        std::size_t count() const noexcept
          {
            return this->m_count;
          }

        const char * filt(typename std::iterator_traits<IteratorT>::reference param) const
          {
            return this->m_filter(static_cast<typename std::iterator_traits<IteratorT>::reference>(param));
          }
      };

    template<typename IteratorT, typename FilterT>
      std::ostream & operator<<(std::ostream &os, const Type_Name_Imploder<IteratorT, FilterT> &imploder)
      {
        auto cur = imploder.begin();
        auto rem = imploder.count();
        // Deal with nasty commas.
        switch(rem) {
        default:
          while(--rem != 0) {
            os << imploder.filt(*cur) << ", ";
            ++cur;
          }
          // Fallthrough.
        case 1:
          os << imploder.filt(*cur);
          ++cur;
          // Fallthrough.
        case 0:
          break;
        }
        return os;
      }

    template<typename IteratorT, typename FilterT>
      constexpr Type_Name_Imploder<typename std::decay<IteratorT>::type, FilterT> do_implode(IteratorT &&begin, std::size_t count, FilterT &&filter)
      {
        return Type_Name_Imploder<typename std::decay<IteratorT>::type, FilterT>(std::forward<IteratorT>(begin), count, std::forward<FilterT>(filter));
      }

    }

[[noreturn]] void Argument_Sentry::throw_no_matching_function_call(const Overload_Parameter *overload_data, std::size_t overload_size) const
  {
    const auto &name = this->m_name;
    const auto &args = this->m_args.get();
    // Create a message containing arguments.
    rocket::insertable_ostream mos;
    mos << "There was no matching overload for function call `" << name << "("
        << do_implode(args.data(), args.size(), [&](const Reference &arg) { return Value::get_type_name(arg.read().type()); })
        << ")`.";
    // If an overload list is provided, append it.
    if(overload_size != 0) {
      auto ptr = overload_data;
      ROCKET_ASSERT(ptr);
      const auto end = ptr + overload_size;
      // Collect all overloads.
      mos << "\n[possible overloads: ";
      for(;;) {
        const auto nparams = static_cast<std::size_t>(ptr->nparams);
        ++ptr;
        ROCKET_ASSERT_MSG(nparams <= static_cast<std::size_t>(end - ptr), "The possible overload data were malformed.");
        mos << "`" << name << "("
            << do_implode(ptr, nparams, [&](const Overload_Parameter &param) { return (param.nparams == 0xFF) ? "<generic>" : Value::get_type_name(param.type); })
            << ")`";
        // Move to the next parameter.
        if((ptr += nparams) == end) {
          break;
        }
        mos << ", ";
      }
      mos << "]";
    }
    // Throw it now.
    throw_runtime_error(std::move(mos), ROCKET_FUNCSIG);
  }

}
