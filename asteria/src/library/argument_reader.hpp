// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ARGUMENT_READER_HPP_
#define ASTERIA_LIBRARY_ARGUMENT_READER_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
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
    cow_string m_name;
    ref_to<const cow_vector<Reference>> m_args;
    bool m_throw_on_failure;

    // `m_ovlds` contains all overloads that have been tested so far.
    cow_string m_ovlds;
    // `m_state` can be copied elsewhere and back; any further operations will resume from that point.
    State m_state;

  public:
    Argument_Reader(const cow_string& name, ref_to<const cow_vector<Reference>> args) noexcept
      :
        m_name(name), m_args(args), m_throw_on_failure(false),
        m_ovlds(), m_state()
      {
      }

    Argument_Reader(const Argument_Reader&)
      = delete;
    Argument_Reader& operator=(const Argument_Reader&)
      = delete;

  private:
    template<typename HandlerT> inline void do_fail(HandlerT&& handler);

    inline void do_record_parameter_optional(Gtype gtype);
    inline void do_record_parameter_required(Gtype gtype);
    inline void do_record_parameter_generic();
    inline void do_record_parameter_variadic();
    inline void do_record_parameter_finish();

    inline const Reference* do_peek_argument_opt() const;
    inline opt<size_t> do_check_finish_opt() const;

  public:
    const cow_string& get_name() const noexcept
      {
        return this->m_name;
      }
    size_t count_arguments() const noexcept
      {
        return this->m_args->size();
      }
    const Reference& get_argument(size_t index) const
      {
        return this->m_args->at(index);
      }

    bool does_throw_on_failure() const noexcept
      {
        return this->m_throw_on_failure;
      }
    Argument_Reader& set_throw_on_failure(bool throw_on_failure = true) noexcept
      {
        return this->m_throw_on_failure = throw_on_failure, *this;
      }

    const Argument_Reader& save(State& state) const noexcept
      {
        state = this->m_state;
        return *this;
      }
    Argument_Reader& save(State& state) noexcept
      {
        state = this->m_state;
        return *this;
      }
    Argument_Reader& load(const State& state) noexcept
      {
        this->m_state = state;
        return *this;
      }

    // Start recording an overload.
    Argument_Reader& start() noexcept;
    // Get an OPTIONAL argument.
    // The argument must exist and must be of the desired type or `null`; otherwise the operation fails.
    Argument_Reader& g(Reference& ref);
    Argument_Reader& g(Value& value);
    Argument_Reader& g(opt<G_boolean>& qxvalue);
    Argument_Reader& g(opt<G_integer>& qxvalue);
    Argument_Reader& g(opt<G_real>& qxvalue);  // This function converts `integer`s to `real`s implicitly.
    Argument_Reader& g(opt<G_string>& qxvalue);
    Argument_Reader& g(opt<G_opaque>& qxvalue);
    Argument_Reader& g(opt<G_function>& qxvalue);
    Argument_Reader& g(opt<G_array>& qxvalue);
    Argument_Reader& g(opt<G_object>& qxvalue);
    // Get a REQUIRED argument.
    // The argument must exist and must be of the desired type; otherwise the operation fails.
    Argument_Reader& g(G_boolean& xvalue);
    Argument_Reader& g(G_integer& xvalue);
    Argument_Reader& g(G_real& xvalue);  // This function converts `integer`s to `real`s implicitly.
    Argument_Reader& g(G_string& xvalue);
    Argument_Reader& g(G_opaque& xvalue);
    Argument_Reader& g(G_function& xvalue);
    Argument_Reader& g(G_array& xvalue);
    Argument_Reader& g(G_object& xvalue);
    // Terminate the argument list and finish this overload.
    // For the overload taking no argument, if there are excess arguments, the operation fails.
    // For the other overloads, excess arguments are copied into `vargs`.
    bool finish(cow_vector<Reference>& vargs);
    bool finish(cow_vector<Value>& vargs);
    bool finish();

    // Throw an exception saying there are no viable overloads.
    [[noreturn]] void throw_no_matching_function_call() const;
  };

}  // namespace Asteria

#endif
