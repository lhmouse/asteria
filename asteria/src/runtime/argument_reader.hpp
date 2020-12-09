// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ARGUMENT_READER_HPP_
#define ASTERIA_RUNTIME_ARGUMENT_READER_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "runtime_error.hpp"
#include "../llds/reference_stack.hpp"
#include "../details/argument_reader.ipp"

namespace asteria {

class Argument_Reader
  {
  private:
    cow_string m_name;
    Reference_Stack m_stack;

    details_argument_reader::State m_state;
    cow_vector<details_argument_reader::State> m_saved_states;
    cow_string m_overloads;

  public:
    explicit
    Argument_Reader(const cow_string& name, Reference_Stack&& stack)
      noexcept
      : m_name(name), m_stack(::std::move(stack))
      { }

  private:
    inline
    void
    do_prepare_parameter(const char* param);

    inline
    void
    do_terminate_parameter_list();

    inline
    Argument_Reader&
    do_mark_match_failure()
      noexcept;

    inline
    const Reference*
    do_peek_argument()
      const;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Argument_Reader);

    const cow_string&
    name()
      const noexcept
      { return this->m_name;  }

    // These functions access `m_saved_states`.
    // Under a number of circumstances, function overloads share a common
    // initial parameter sequence. We allow saving and loading parser
    // states to eliminate the overhead of re-parsing this sequence. The
    // `index` argument is a subscript of `m_saved_states`, which is
    // resized by `save_state()` as necessary.
    Argument_Reader&
    load_state(size_t index);

    Argument_Reader&
    save_state(size_t index);

    // Start an overload. Effectively, this function clears `m_state`.
    Argument_Reader&
    start_overload()
      noexcept;

    // Gets an optional argument. The argument may be of the desired type
    // or null.
    Argument_Reader&
    optional(Reference& out);

    Argument_Reader&
    optional(Value& out);

    Argument_Reader&
    optional(Opt_boolean& out);

    Argument_Reader&
    optional(Opt_integer& out);

    Argument_Reader&
    optional(Opt_real& out);

    Argument_Reader&
    optional(Opt_string& out);

    Argument_Reader&
    optional(Opt_opaque& out);

    Argument_Reader&
    optional(Opt_function& out);

    Argument_Reader&
    optional(Opt_array& out);

    Argument_Reader&
    optional(Opt_object& out);

    // Gets a required argument. The argument must be of the desired type.
    Argument_Reader&
    required(V_boolean& out);

    Argument_Reader&
    required(V_integer& out);

    Argument_Reader&
    required(V_real& out);

    Argument_Reader&
    required(V_string& out);

    Argument_Reader&
    required(V_opaque& out);

    Argument_Reader&
    required(V_function& out);

    Argument_Reader&
    required(V_array& out);

    Argument_Reader&
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
    throw_no_matching_function_call()
      const;
  };

#define ASTERIA_BINDING_BEGIN(name, self, global, reader)  \
    (::asteria::V_function(  \
      "`" name "(...)` at '" ASTERIA_NATIVE_SOURCE_LOCATION "'",  \
      *[](::asteria::Reference& self,  \
          ::asteria::Global_Context& global,  \
          ::asteria::Reference_Stack&& tfiXopzY)  \
        -> Reference& \
        {  \
          (void)global;  \
          ::asteria::Argument_Reader reader(  \
                 ::rocket::sref("" name), ::std::move(tfiXopzY));  \
          try  \
          // Add function body here.

#define ASTERIA_BINDING_END  \
          ASTERIA_RUNTIME_CONVERT_EXCEPTION_AND_RETHROW  \
          reader.throw_no_matching_function_call();  \
        }  \
    ))

#define ASTERIA_BINDING_RETURN(self, func, ...)  \
    do {  \
      return ::asteria::details_argument_reader::  \
        apply_and_set_result(self,  \
          [](auto&&... jZrUeTNf) -> decltype(auto)  \
            { return func(  \
                static_cast<decltype(jZrUeTNf)&&>(  \
                    jZrUeTNf)...);  },  \
          ::std::forward_as_tuple(__VA_ARGS__));  \
    }  \
    while(false)

#define ASTERIA_BINDING_RETURN_MOVE(self, func, ...)  \
    do {  \
      return ::asteria::details_argument_reader::  \
        apply_and_set_result(self,  \
          [](auto&&... jZrUeTNf) -> decltype(auto)  \
            { return func(  \
                ::asteria::details_argument_reader::move(  \
                    jZrUeTNf)...);  },  \
          ::std::forward_as_tuple(__VA_ARGS__));  \
    }  \
    while(false)

}  // namespace asteria

#endif
