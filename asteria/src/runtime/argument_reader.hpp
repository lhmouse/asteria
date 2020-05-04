// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ARGUMENT_READER_HPP_
#define ASTERIA_RUNTIME_ARGUMENT_READER_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../value.hpp"

namespace Asteria {

class Argument_Reader
  {
  public:
    struct State
      {
        cow_string history;
        uint32_t nparams;
        bool finished;
        bool succeeded;
      };

  private:
    refp<const cow_vector<Reference>> m_args;
    cow_string m_name;

    // `m_ovlds` contains all overloads that have been tested so far.
    cow_string m_ovlds;
    // `m_state` can be copied elsewhere and back; any further operations will resume
    // from that point.
    State m_state = { };

  public:
    Argument_Reader(refp<const cow_vector<Reference>> args, const cow_string& name)
    noexcept
      : m_args(args), m_name(name)
      { }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Argument_Reader);

  private:
    inline
    void
    do_record_parameter_optional(Vtype vtype);

    inline
    void
    do_record_parameter_required(Vtype vtype);

    inline
    void
    do_record_parameter_generic();

    inline
    void
    do_record_parameter_variadic();

    inline
    void
    do_record_parameter_finish();

    inline
    const Reference*
    do_peek_argument_opt()
    const;

    inline
    opt<size_t>
    do_check_finish_opt()
    const;

  public:
    size_t
    count_arguments()
    const noexcept
      { return this->m_args->size();  }

    const Reference&
    get_argument(size_t index)
    const
      { return this->m_args->at(index);  }

    const cow_string&
    get_name()
    const noexcept
      { return this->m_name;  }

    // `S` stands for `save` or `store`.
    const Argument_Reader&
    S(State& state)
    const noexcept
      {
        state = this->m_state;
        return *this;
      }

    Argument_Reader&
    S(State& state)
    noexcept
      {
        state = this->m_state;
        return *this;
      }

    // `L` stands for `load`.
    Argument_Reader&
    L(const State& state)
    noexcept
      {
        this->m_state = state;
        return *this;
      }

    // Start recording an overload.
    // `I` stands for `initiate` or `initialize`.
    Argument_Reader&
    I()
    noexcept;

    // Terminate the argument list and finish this overload.
    // For the overload taking no argument, if there are excess arguments, the operation
    // fails.
    // For the other overloads, excess arguments are copied into `vargs`.
    // `F` stands for `finish` or `finalize`.
    bool
    F(cow_vector<Reference>& vargs);

    bool
    F(cow_vector<Value>& vargs);

    bool
    F();

    // Get a REQUIRED argument.
    // The argument must exist and must be of the desired type; otherwise the operation
    // fails.
    Argument_Reader&
    v(V_boolean& xval);

    Argument_Reader&
    v(V_integer& xval);

    Argument_Reader&
    v(V_real& xval);  // performs integer-to-real conversions as necessary

    Argument_Reader&
    v(V_string& xval);

    Argument_Reader&
    v(V_opaque& xval);

    Argument_Reader&
    v(V_function& xval);

    Argument_Reader&
    v(V_array& xval);

    Argument_Reader&
    v(V_object& xval);

    // Get an OPTIONAL argument.
    // The argument must exist and must be of the desired type or `null`; otherwise the
    // operation fails.
    // `g` stands for `get` or `go`.
    Argument_Reader&
    o(Reference& ref);

    Argument_Reader&
    o(Value& val);

    Argument_Reader&
    o(optV_boolean& xopt);

    Argument_Reader&
    o(optV_integer& xopt);

    Argument_Reader&
    o(optV_real& xopt);  // performs integer-to-real conversions as necessary

    Argument_Reader&
    o(optV_string& xopt);

    Argument_Reader&
    o(optV_opaque& xopt);

    Argument_Reader&
    o(optV_function& xopt);

    Argument_Reader&
    o(optV_array& xopt);

    Argument_Reader&
    o(optV_object& xopt);

    // Throw an exception saying there are no viable overloads.
    [[noreturn]]
    void
    throw_no_matching_function_call()
    const;
  };

}  // namespace Asteria

#endif
