// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ARGUMENT_READER_
#define ASTERIA_ARGUMENT_READER_

#include "fwd.hpp"
#include "runtime/reference.hpp"
#include "runtime/runtime_error.hpp"
#include "llds/reference_stack.hpp"
namespace asteria {

class Argument_Reader
  {
  private:
    cow_string m_name;
    Reference_Stack m_stack;

    struct State
      {
        cow_string params;
        uint32_t nparams = 0;
        bool ended = false;
        bool matched = false;
      };

    State m_state;
    cow_vector<State> m_saved_states;
    cow_string m_overloads;

  public:
    explicit
    Argument_Reader(stringR name, Reference_Stack&& stack) noexcept
      :
        m_name(name), m_stack(::std::move(stack))
      {
      }

  private:
    inline
    void
    do_prepare_parameter(const char* param);

    inline
    void
    do_terminate_parameter_list();

    inline
    void
    do_mark_match_failure() noexcept;

    inline
    const Reference*
    do_peek_argument() const;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Argument_Reader);

    const cow_string&
    name() const noexcept
      { return this->m_name;  }

    // These functions access `m_saved_states`.
    // Under a number of circumstances, function overloads share a common
    // initial parameter sequence. We allow saving and loading parser
    // states to eliminate the overhead of re-parsing this sequence. The
    // `index` argument is a subscript of `m_saved_states`, which is
    // resized by `save_state()` as necessary.
    void
    load_state(size_t index);

    void
    save_state(size_t index);

    // Start an overload. Effectively, this function clears `m_state`.
    void
    start_overload() noexcept;

    // Gets an optional argument. The argument may be of the desired type
    // or null.
    void
    optional(Reference& out);

    void
    optional(Value& out);

    void
    optional(optV_boolean& out);

    void
    optional(optV_integer& out);

    void
    optional(optV_real& out);

    void
    optional(optV_string& out);

    void
    optional(optV_opaque& out);

    void
    optional(optV_function& out);

    void
    optional(optV_array& out);

    void
    optional(optV_object& out);

    // Gets a required argument. The argument must be of the desired type.
    void
    required(V_boolean& out);

    void
    required(V_integer& out);

    void
    required(V_real& out);

    void
    required(V_string& out);

    void
    required(V_opaque& out);

    void
    required(V_function& out);

    void
    required(V_array& out);

    void
    required(V_object& out);

    // Terminate the current overload. The return value indicates whether
    // the overload has been accepted. The second and third functions
    // accept variadic arguments.
    bool
    end_overload();

    bool
    end_overload(cow_vector<Reference>& vargs);

    bool
    end_overload(cow_vector<Value>& vargs);

    // This function throws an exception containing a message composed
    // from all overloads that have been tested so far.
    [[noreturn]]
    void
    throw_no_matching_function_call() const;
  };

}  // namespace asteria
#endif
