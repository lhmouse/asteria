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
        Cow_Vector<std::uint8_t> prototype;
        bool finished;
        bool succeeded;
      };

  private:
    Cow_String m_name;
    std::reference_wrapper<const Cow_Vector<Reference>> m_args;
    bool m_throw_on_failure;

    // This string stores all overloads that have been tested so far.
    // Overloads are encoded in binary formats.
    Cow_Vector<std::uint8_t> m_overloads;
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
    template<typename HandlerT> inline void do_fail(HandlerT &&handler);
    inline const Reference * do_peek_argument_optional_opt();
    template<typename XvalueT> inline Argument_Reader & do_read_typed_argument_optional(XvalueT &xvalue_out);
    inline const Reference * do_peek_argument_required_opt();
    template<typename XvalueT> inline Argument_Reader & do_read_typed_argument_required(XvalueT &xvalue_out);

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
    Argument_Reader & opt(Reference &ref_out);
    Argument_Reader & opt(Value &value_out);
    // The argument must exist and must be of the desired type or `null`; otherwise the operation fails.
    // If the argument is `null`, the operation succeeds without clobbering `xvalue_out`.
    Argument_Reader & opt(D_boolean &xvalue_out);
    Argument_Reader & opt(D_integer &xvalue_out);
    Argument_Reader & opt(D_real &xvalue_out);
    Argument_Reader & opt(D_string &xvalue_out);
    Argument_Reader & opt(D_opaque &xvalue_out);
    Argument_Reader & opt(D_function &xvalue_out);
    Argument_Reader & opt(D_array &xvalue_out);
    Argument_Reader & opt(D_object &xvalue_out);
    // Get a REQUIRED argument.
    // The argument must exist and must be of the desired type; otherwise the operation fails.
    // Particularly, if the argument is `null`, the operation fails.
    Argument_Reader & req(D_boolean &xvalue_out);
    Argument_Reader & req(D_integer &xvalue_out);
    Argument_Reader & req(D_real &xvalue_out);
    Argument_Reader & req(D_string &xvalue_out);
    Argument_Reader & req(D_opaque &xvalue_out);
    Argument_Reader & req(D_function &xvalue_out);
    Argument_Reader & req(D_array &xvalue_out);
    Argument_Reader & req(D_object &xvalue_out);
    // Terminate the argument list and finish this overload.
    // There shall be no more argument; otherwise this operation fails.
    Argument_Reader & finish();

    // Throw an exception saying there are no viable overloads.
    [[noreturn]] void throw_no_matching_function_call() const;
  };

}  // namespace Asteria

#endif
