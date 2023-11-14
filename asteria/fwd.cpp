// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
#include "fwd.hpp"
#include "runtime/reference.hpp"
#include "runtime/runtime_error.hpp"
#include "llds/reference_stack.hpp"
#include "utils.hpp"
namespace asteria {

void
Rcbase::
vtable_key_function_sLBHstEX() noexcept
  {
  }

Abstract_Opaque::
~Abstract_Opaque()
  {
  }

Abstract_Function::
~Abstract_Function()
  {
  }

tinyfmt&
cow_opaque::
describe(tinyfmt& fmt) const
  {
    if(auto ptr = this->m_sptr.get())
      return ptr->describe(fmt);

    return fmt << "[null opaque pointer]";
  }

tinyfmt&
cow_function::
describe(tinyfmt& fmt) const
  {
    if(this->m_fptr)
      return fmt << "native function " << this->m_desc;  // static

    if(auto ptr = this->m_sptr.get())
      return ptr->describe(fmt);  // dynamic

    return fmt << "[null function pointer]";
  }

Reference&
cow_function::
invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const
  {
    try {
      stack.clear_red_zone();

      if(auto fptr = this->m_fptr)
        return fptr(self, global, ::std::move(stack));  // static

      if(auto ptr = this->m_sptr.get())
        return ptr->invoke_ptc_aware(self, global, ::std::move(stack));  // dynamic
    }
    catch(Runtime_Error& except)
      { throw;  }  // forward
    catch(exception& stdex)
      { throw Runtime_Error(Runtime_Error::M_format(), "$1", stdex);  }  // replace

    throw Runtime_Error(Runtime_Error::M_format(),
              "cow_function: attempt to call a null function");
  }

Reference&
cow_function::
invoke(Reference& self, Global_Context& global, Reference_Stack&& stack) const
  {
    auto& result = this->invoke_ptc_aware(self, global, ::std::move(stack));
    result.check_function_result(global);
    return result;
  }

const char*
describe_type(Type type) noexcept
  {
    switch(type) {
      case type_null:
        return "null";

      case type_boolean:
        return "boolean";

      case type_integer:
        return "integer";

      case type_real:
        return "real";

      case type_string:
        return "string";

      case type_opaque:
        return "opaque";

      case type_function:
        return "function";

      case type_array:
        return "array";

      case type_object:
        return "object";

      default:
        return "[unknown data type]";
    }
  }

const char*
describe_xref(Xref xref) noexcept
  {
    switch(xref) {
      case xref_invalid:
        return "uninitialized reference";

      case xref_void:
        return "void";

      case xref_temporary:
        return "temporary value";

      case xref_variable:
        return "variable";

      case xref_ptc:
        return "pending proper tail call";

      default:
        return "[unknown reference type]";
    }
  }

const char*
describe_frame_type(Frame_Type type) noexcept
  {
    switch(type) {
      case frame_type_native:
        return "native code";

      case frame_type_throw:
        return "throw statement";

      case frame_type_catch:
        return "  catch clause";

      case frame_type_plain:
        return "  expression";

      case frame_type_func:
        return "  function";

      case frame_type_defer:
        return "  defer statement";

      case frame_type_assert:
        return "assertion failure";

      case frame_type_try:
        return "  try clause";

      default:
        return "[unknown frame type]";
    }
  }

const char*
describe_compiler_status(Compiler_Status status) noexcept
  {
    switch(status) {
      case compiler_status_success:
        return "operation succeeded";

      case compiler_status_utf8_sequence_invalid:
        return "UTF-8 sequence invalid";

      case compiler_status_utf8_sequence_incomplete:
        return "UTF-8 sequence incomplete";

      case compiler_status_utf_code_point_invalid:
        return "UTF code point invalid";

      case compiler_status_null_character_disallowed:
        return "null character disallowed";

      case compiler_status_conflict_marker_detected:
        return "merge conflict marker detected";

      case compiler_status_token_character_unrecognized:
        return "character not recognizable";

      case compiler_status_string_literal_unclosed:
        return "string literal unclosed";

      case compiler_status_escape_sequence_unknown:
        return "escape sequence invalid";

      case compiler_status_escape_sequence_incomplete:
        return "escape sequence incomplete";

      case compiler_status_escape_sequence_invalid_hex:
        return "hexadecimal digit expected";

      case compiler_status_escape_utf_code_point_invalid:
        return "UTF code point value invalid";

      case compiler_status_reserved_1012:
        return "";

      case compiler_status_integer_literal_overflow:
        return "integer literal too large";

      case compiler_status_integer_literal_inexact:
        return "integer literal yielding a fraction";

      case compiler_status_real_literal_overflow:
        return "real literal too large";

      case compiler_status_real_literal_underflow:
        return "real literal truncated to zero";

      case compiler_status_numeric_literal_suffix_invalid:
        return "numeric literal suffix invalid";

      case compiler_status_block_comment_unclosed:
        return "block comment unclosed";

      case compiler_status_digit_separator_following_nondigit:
        return "digit separator not following a digit";

      case compiler_status_duplicate_key_in_object:
        return "duplicate key in unnamed object";

      case compiler_status_identifier_expected:
        return "identifier expected";

      case compiler_status_semicolon_expected:
        return "`;` expected";

      case compiler_status_string_literal_expected:
        return "string literal expected";

      case compiler_status_statement_expected:
        return "statement expected";

      case compiler_status_equals_sign_expected:
        return "`=` expected";

      case compiler_status_expression_expected:
        return "expression expected";

      case compiler_status_open_brace_expected:
        return "`{` expected";

      case compiler_status_closing_brace_or_statement_expected:
        return "`}` or statement expected";

      case compiler_status_open_parenthesis_expected:
        return "`(` expected";

      case compiler_status_closing_parenthesis_or_comma_expected:
        return "`)` or `,` expected";

      case compiler_status_closing_parenthesis_expected:
        return "`)` expected";

      case compiler_status_colon_expected:
        return "`:` expected";

      case compiler_status_closing_brace_or_switch_clause_expected:
        return "`}`, `case` or `default` expected";

      case compiler_status_keyword_while_expected:
        return "`while` expected";

      case compiler_status_keyword_catch_expected:
        return "`catch` expected";

      case compiler_status_comma_expected:
        return "`,` expected";

      case compiler_status_for_statement_initializer_expected:
        return "`each`, `;`, variable definition or expression statement expected";

      case compiler_status_semicolon_or_expression_expected:
        return "`;` or expression expected";

      case compiler_status_closing_brace_expected:
        return "`}` expected";

      case compiler_status_too_many_elements:
        return "max number of elements exceeded";

      case compiler_status_closing_bracket_expected:
        return "`]` expected";

      case compiler_status_open_brace_or_initializer_expected:
        return "`{`, `=` or `->` expected";

      case compiler_status_equals_sign_or_colon_expected:
        return "`=` or `:` expected";

      case compiler_status_closing_bracket_or_comma_expected:
        return "`]` or `,` expected";

      case compiler_status_closing_brace_or_comma_expected:
        return "`}` or `,` expected";

      case compiler_status_closing_bracket_or_expression_expected:
        return "`]` or expression expected";

      case compiler_status_closing_brace_or_json5_key_expected:
        return "`}`, identifier or string literal expected";

      case compiler_status_argument_expected:
        return "argument expected";

      case compiler_status_closing_parenthesis_or_argument_expected:
        return "`)` or argument expected";

      case compiler_status_arrow_expected:
        return "`->` expected";

      case compiler_status_reserved_identifier_not_declarable:
        return "reserved identifier not declarable";

      case compiler_status_break_no_matching_scope:
        return "no matching scope found for `break`";

      case compiler_status_continue_no_matching_scope:
        return "no matching scope found for `continue`";

      case compiler_status_closing_bracket_or_identifier_expected:
        return "`]` or identifier expected";

      case compiler_status_closing_brace_or_identifier_expected:
        return "`}` or identifier expected";

      case compiler_status_invalid_expression:
        return "invalid expression";

      case compiler_status_undeclared_identifier:
        return "undeclared identifier";

      case compiler_status_multiple_default:
        return "multiple `default` clauses not allowed";

      case compiler_status_duplicate_name_in_structured_binding:
        return "duplicate name in structured binding";

      case compiler_status_duplicate_name_in_parameter_list:
        return "duplicate name in parameter list";

      case compiler_status_nondeclaration_statement_expected:
        return "non-declaration statement expected";

      default:
        return "[unknown compiler status]";
    }
  }

// We assume that a all-bit-zero struct represents the `null` value.
// This is effectively undefined behavior. Don't play with this at home!
alignas(Value) const unsigned char null_value_storage[sizeof(Value)] = { };

}  // namespace asteria
