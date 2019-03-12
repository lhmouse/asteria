// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ARGUMENT_READER_HPP_
#define ASTERIA_LIBRARY_ARGUMENT_READER_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
#include "../runtime/value.hpp"

namespace Asteria {

class Argument_Reader
  {
  public:
    struct State
      {
        Cow_Vector<unsigned char> history;
        std::uint32_t offset;
        bool finished;
        bool succeeded;
      };

  private:
    Cow_String m_name;
    std::reference_wrapper<const Cow_Vector<Reference>> m_args;
    bool m_throw_on_failure;

    // This string stores all overloads that have been tested so far.
    // Overloads are encoded in binary formats.
    Cow_Vector<unsigned char> m_overloads;
    // N.B. The contents of `m_state` can be copied elsewhere and back.
    // Any further operations will resume from that point.
    State m_state;

  public:
    Argument_Reader(Cow_String name, const Cow_Vector<Reference> &args) noexcept
      : m_name(rocket::move(name)), m_args(args), m_throw_on_failure(false),
        m_overloads(), m_state()
      {
      }

    Argument_Reader(const Argument_Reader &)
      = delete;
    Argument_Reader & operator=(const Argument_Reader &)
      = delete;

  private:
    inline const Reference * do_peek_argument(bool required);
    template<typename XvalueT> inline Argument_Reader & do_get_optional_value(XvalueT &value_out, const XvalueT &default_value);
    template<typename XvalueT> inline Argument_Reader & do_get_required_value(XvalueT &value_out);

  public:
    const Cow_String & get_name() const noexcept
      {
        return this->m_name;
      }
    std::size_t get_argument_count() const noexcept
      {
        return this->m_args.get().size();
      }
    const Reference & get_argument(std::size_t index) const
      {
        return this->m_args.get().at(index);
      }

    bool does_throw_on_failure() const noexcept
      {
        return this->m_throw_on_failure;
      }
    void set_throw_on_failure(bool throw_on_failure = true) noexcept
      {
        this->m_throw_on_failure = throw_on_failure;
      }
    const State & get_state() const noexcept
      {
        return this->m_state;
      }
    void set_state(const State &state) noexcept
      {
        this->m_state = state;
      }

    // Reader objects are allowed in `if` and `while` conditions, just like `std::istream`s.
    explicit operator bool () const noexcept
      {
        return this->m_state.succeeded;
      }
    // Start recording an overload.
    Argument_Reader & start() noexcept;
    // Get an OPTIONAL argument.
    // The argument must exist (and must be of the desired type or `null` for the overloads taking two arguments); otherwise this operation fails.
    // N.B. These functions provide STRONG exception safety guarantee.
    Argument_Reader & opt(Reference &ref_out);
    Argument_Reader & opt(Value &value_out);
    Argument_Reader & opt(D_boolean &value_out, D_boolean default_value = false);
    Argument_Reader & opt(D_integer &value_out, D_integer default_value = 0);
    Argument_Reader & opt(D_real &value_out, D_real default_value = 0);
    Argument_Reader & opt(D_string &value_out, const D_string &default_value = { });
    Argument_Reader & opt(D_opaque &value_out, const D_opaque &default_value /* no default value */);
    Argument_Reader & opt(D_function &value_out, const D_function &default_value /* no default value */);
    Argument_Reader & opt(D_array &value_out, const D_array &default_value = { });
    Argument_Reader & opt(D_object &value_out, const D_object &default_value = { });
    // Get a REQUIRED argument.
    // The argument must exist and must be of the desired type; otherwise this operation fails.
    // N.B. These functions provide STRONG exception safety guarantee.
    Argument_Reader & req(D_null &value_out);
    Argument_Reader & req(D_boolean &value_out);
    Argument_Reader & req(D_integer &value_out);
    Argument_Reader & req(D_real &value_out);
    Argument_Reader & req(D_string &value_out);
    Argument_Reader & req(D_opaque &value_out);
    Argument_Reader & req(D_function &value_out);
    Argument_Reader & req(D_array &value_out);
    Argument_Reader & req(D_object &value_out);
    // Terminate the argument list and finish this overload.
    // There will be no more argument; otherwise this operation fails.
    // N.B. This function provides STRONG exception safety guarantee.
    Argument_Reader & finish();

    // Throw an exception saying there are no viable overloads.
    [[noreturn]] void throw_no_matching_function_call() const;
  };

}  // namespace Asteria

#endif
