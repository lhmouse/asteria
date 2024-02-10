// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "argument_reader.hpp"
#include "runtime_error.hpp"
#include "../utils.hpp"
namespace asteria {

Argument_Reader::
~Argument_Reader()
  {
  }

void
Argument_Reader::
do_prepare_parameter(const char* param)
  {
    // Ensure `end_overload()` has not been called for this overload.
    if(this->m_state.finish)
      throw Runtime_Error(xtc_format,
               "Current overload marked ended");

    // Append the parameter.
    this->m_state.params << param << ", ";
    this->m_state.nparams ++;
  }

void
Argument_Reader::
do_terminate_parameter_list()
  {
    // Ensure `end_overload()` has not been called for this overload.
    if(this->m_state.finish)
      throw Runtime_Error(xtc_format,
               "Current overload marked ended");

    // Mark this overload complete. The terminating ", " shall be dropped.
    this->m_state.finish = true;
    this->m_overloads.append(this->m_state.params, 0, this->m_state.params.size() - 2);
    this->m_overloads.push_back('|');
  }

void
Argument_Reader::
do_mark_match_failure() noexcept
  {
    // Set the current overload as a non-match.
    this->m_state.match = false;
  }

const Reference*
Argument_Reader::
do_peek_argument() const
  {
    // Try getting an argument for the next parameter. Before calling this
    // function, `do_prepare_parameter()` shall have been called.
    if(!this->m_state.match)
      return nullptr;

    uint32_t rindex = this->m_stack.size() - this->m_state.nparams;
    if(rindex >= this->m_stack.size())
      return nullptr;

    return &(this->m_stack.top(rindex));
  }

void
Argument_Reader::
load_state(uint32_t index)
  {
    this->m_state = this->m_saved_states.at(index);
  }

void
Argument_Reader::
save_state(uint32_t index)
  {
    // Reserve a slot.
    while(index >= this->m_saved_states.size())
      this->m_saved_states.emplace_back();

    this->m_saved_states.mut(index) = this->m_state;
  }

void
Argument_Reader::
start_overload() noexcept
  {
    this->m_state.params.clear();
    this->m_state.nparams = 0;
    this->m_state.finish = false;
    this->m_state.match = true;
  }

void
Argument_Reader::
optional(Reference& out)
  {
    out.clear();
    this->do_prepare_parameter("[reference]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Copy the argument reference as is.
    out = *qref;
  }

void
Argument_Reader::
optional(Value& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[value]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and copy the value as is.
    out = qref->dereference_readonly();
  }

void
Argument_Reader::
optional(optV_boolean& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[boolean]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_boolean())
      return this->do_mark_match_failure();

    out = val.as_boolean();
  }

void
Argument_Reader::
optional(optV_integer& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[integer]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_integer())
      return this->do_mark_match_failure();

    out = val.as_integer();
  }

void
Argument_Reader::
optional(optV_real& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[real]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_real())
      return this->do_mark_match_failure();

    out = val.as_real();
  }

void
Argument_Reader::
optional(optV_string& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[string]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_string())
      return this->do_mark_match_failure();

    out = val.as_string();
  }

void
Argument_Reader::
optional(optV_opaque& out)
  {
    out = nullptr;
    this->do_prepare_parameter("[opaque]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_opaque())
      return this->do_mark_match_failure();

    out = val.as_opaque();
  }

void
Argument_Reader::
optional(optV_function& out)
  {
    out = nullptr;
    this->do_prepare_parameter("[function]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_function())
      return this->do_mark_match_failure();

    out = val.as_function();
  }

void
Argument_Reader::
optional(optV_array& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[array]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_array())
      return this->do_mark_match_failure();

    out = val.as_array();
  }

void
Argument_Reader::
optional(optV_object& out)
  {
    out = nullopt;
    this->do_prepare_parameter("[object]");

    auto qref = this->do_peek_argument();
    if(!qref)
      return;

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(val.is_null())
      return;

    if(!val.is_object())
      return this->do_mark_match_failure();

    out = val.as_object();
  }

void
Argument_Reader::
required(V_boolean& out)
  {
    out = false;
    this->do_prepare_parameter("boolean");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_boolean())
      return this->do_mark_match_failure();

    out = val.as_boolean();
  }

void
Argument_Reader::
required(V_integer& out)
  {
    out = 0;
    this->do_prepare_parameter("integer");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_integer())
      return this->do_mark_match_failure();

    out = val.as_integer();
  }

void
Argument_Reader::
required(V_real& out)
  {
    out = 0.0;
    this->do_prepare_parameter("real");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_real())
      return this->do_mark_match_failure();

    out = val.as_real();
  }

void
Argument_Reader::
required(V_string& out)
  {
    out.clear();
    this->do_prepare_parameter("string");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_string())
      return this->do_mark_match_failure();

    out = val.as_string();
  }

void
Argument_Reader::
required(V_opaque& out)
  {
    out = nullptr;
    this->do_prepare_parameter("opaque");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_opaque())
      return this->do_mark_match_failure();

    out = val.as_opaque();
  }

void
Argument_Reader::
required(V_function& out)
  {
    out = nullptr;
    this->do_prepare_parameter("function");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_function())
      return this->do_mark_match_failure();

    out = val.as_function();
  }

void
Argument_Reader::
required(V_array& out)
  {
    out.clear();
    this->do_prepare_parameter("array");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_array())
      return this->do_mark_match_failure();

    out = val.as_array();
  }

void
Argument_Reader::
required(V_object& out)
  {
    out.clear();
    this->do_prepare_parameter("object");

    auto qref = this->do_peek_argument();
    if(!qref)
      return this->do_mark_match_failure();

    // Dereference the argument and check its type.
    const auto& val = qref->dereference_readonly();
    if(!val.is_object())
      return this->do_mark_match_failure();

    out = val.as_object();
  }

bool
Argument_Reader::
end_overload()
  {
    this->do_terminate_parameter_list();

    if(!this->m_state.match)
      return false;

    // Ensure no more arguments follow. Note there may be fewer.
    uint32_t nparams = this->m_state.nparams;
    if(this->m_stack.size() > nparams) {
      this->do_mark_match_failure();
      return false;
    }

    // Accept all arguments.
    return true;
  }

bool
Argument_Reader::
end_overload(cow_vector<Reference>& vargs)
  {
    vargs.clear();
    this->do_prepare_parameter("...");
    this->do_terminate_parameter_list();

    if(!this->m_state.match)
      return false;

    // Check for variadic arguments. Note the `...` is not a parameter.
    uint32_t nparams = this->m_state.nparams - 1;
    if(this->m_stack.size() > nparams) {
      uint32_t nvargs = this->m_stack.size() - nparams;
      vargs.reserve(nvargs);
      while(nvargs != 0)
        vargs.emplace_back(this->m_stack.top(--nvargs));
    }

    // Accept all arguments.
    return true;
  }

bool
Argument_Reader::
end_overload(cow_vector<Value>& vargs)
  {
    vargs.clear();
    this->do_prepare_parameter("...");
    this->do_terminate_parameter_list();

    if(!this->m_state.match)
      return false;

    // Check for variadic arguments. Note the `...` is not a parameter.
    uint32_t nparams = this->m_state.nparams - 1;
    if(this->m_stack.size() > nparams) {
      uint32_t nvargs = this->m_stack.size() - nparams;
      vargs.reserve(nvargs);
      while(nvargs != 0)
        vargs.emplace_back(this->m_stack.top(--nvargs).dereference_readonly());
    }

    // Accept all arguments.
    return true;
  }

void
Argument_Reader::
throw_no_matching_function_call() const
  {
    // Compose the list of arguments.
    cow_string caller;
    caller << this->m_name << "(";
    uint32_t offset = this->m_stack.size() - 1;
    switch(this->m_stack.size())
      {
        do {
          caller << ", ";  // fallthrough
      default:
          const auto& arg = this->m_stack.top(offset);
          caller << describe_type(arg.dereference_readonly().type());
        } while(-- offset != UINT32_MAX);  // fallthrough
      case 0:
        break;
    }
    caller << ")";

    // Get the width of the overload number column.
    ::rocket::ascii_numput nump;
    uint32_t overload_count = (uint32_t) ::rocket::count(this->m_overloads, '|');
    nump.put_DU(overload_count);
    static_vector<char, 24> sbuf(nump.size(), ' ');
    sbuf.emplace_back();

    // Compose the list of overloads.
    cow_string overloads;
    overloads << "[list of overloads:";
    offset = 0;
    for(size_t k = 0;  k < overload_count;  ++k) {
      nump.put_DU(k + 1);
      ::std::copy_backward(nump.begin(), nump.end(), sbuf.mut_end() - 1);
      overloads << "\n  " << sbuf.data() << ") `" << this->m_name << "(";

      size_t epos = this->m_overloads.find(offset, '|');
      ROCKET_ASSERT(epos != cow_string::npos);
      overloads.append(this->m_overloads, offset, epos - offset);

      overloads << ")`";
      offset = (uint32_t) epos + 1;
    }
    overloads << "\n  -- end of list of overloads]";

    // Compose the message and throw it.
    throw Runtime_Error(xtc_format,
             "No matching function call for `$1`\n$2", caller, overloads);
  }

}  // namespace asteria
