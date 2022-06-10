// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "statement_sequence.hpp"
#include "compiler_error.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "expression_unit.hpp"
#include "statement.hpp"
#include "infix_element.hpp"
#include "enums.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

opt<Keyword>
do_accept_keyword_opt(Token_Stream& tstrm, initializer_list<Keyword> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // See whether it is one of the acceptable keywords.
    if(!qtok->is_keyword())
      return nullopt;

    auto kwrd = qtok->as_keyword();
    if(::rocket::is_none_of(kwrd, accept))
      return nullopt;

    // Return the keyword and discard this token.
    tstrm.shift();
    return kwrd;
  }

opt<Punctuator>
do_accept_punctuator_opt(Token_Stream& tstrm, initializer_list<Punctuator> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // See whether it is one of the acceptable punctuators.
    if(!qtok->is_punctuator())
      return nullopt;

    auto punct = qtok->as_punctuator();
    if(::rocket::is_none_of(punct, accept))
      return nullopt;

    // Return the punctuator and discard this token.
    tstrm.shift();
    return punct;
  }

opt<cow_string>
do_accept_identifier_opt(Token_Stream& tstrm, bool user_decl)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // See whether it is an identifier.
    if(!qtok->is_identifier())
      return nullopt;

    auto name = qtok->as_identifier();
    if(user_decl && name.starts_with("__"))
      throw Compiler_Error(Compiler_Error::M_format(),
                compiler_status_reserved_identifier_not_declarable, qtok->sloc(),
                "Identifier `$1` not user-declarable", name);

    // Return the identifier and discard this token.
    tstrm.shift();
    return name;
  }

cow_string&
do_concatenate_string_literal_sequence(cow_string& val, Token_Stream& tstrm)
  {
    for(;;) {
      auto qtok = tstrm.peek_opt();
      if(!qtok)
        break;

      // See whether it is a string literal.
      if(!qtok->is_string_literal())
        break;

      // Append the string literal and discard this token.
      val += qtok->as_string_literal();
      tstrm.shift();
    }
    return val;
  }

opt<cow_string>
do_accept_string_literal_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // See whether it is a string literal.
    if(!qtok->is_string_literal())
      return nullopt;

    // Return the string literal and discard this token.
    auto val = qtok->as_string_literal();
    tstrm.shift();
    do_concatenate_string_literal_sequence(val, tstrm);
    return val;
  }

opt<cow_string>
do_accept_json5_key_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // See whether it is a keyword, identifier, or string literal.
    if(qtok->is_keyword()) {
      // Treat the keyword as a plain identifier and discard this token.
      auto kwrd = qtok->as_keyword();
      tstrm.shift();
      return sref(stringify_keyword(kwrd));
    }

    if(qtok->is_identifier()) {
      // Return the identifier and discard this token.
      auto name = qtok->as_identifier();
      tstrm.shift();
      return name;
    }

    if(qtok->is_string_literal()) {
      // Return the string literal and discard this token.
      auto val = qtok->as_string_literal();
      tstrm.shift();
      return val;
    }

    return nullopt;
  }

opt<Value>
do_accept_literal_opt(Token_Stream& tstrm)
  {
    // literal ::=
    //   keyword-literal | string-literal | numeric-literal
    // keyword-literal ::=
    //   "null" | "false" | "true"
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    if(qtok->is_keyword()) {
      switch(weaken_enum(qtok->as_keyword())) {
        case keyword_null:
          // Discard this token and create a `null`.
          tstrm.shift();
          return null_value;

        case keyword_true:
          // Discard this token and create a `true`.
          tstrm.shift();
          return true;

        case keyword_false:
          // Discard this token and create a `false`.
          tstrm.shift();
          return false;

        default:
          return nullopt;
      }
    }

    if(qtok->is_integer_literal()) {
      // Copy the value and discard this token.
      auto val = qtok->as_integer_literal();
      tstrm.shift();
      return val;
    }

    if(qtok->is_real_literal()) {
      // Copy the value and discard this token.
      auto val = qtok->as_real_literal();
      tstrm.shift();
      return val;
    }

    if(qtok->is_string_literal()) {
      // Copy the value and discard this token.
      auto val = qtok->as_string_literal();
      tstrm.shift();
      do_concatenate_string_literal_sequence(val, tstrm);
      return val;
    }

    return nullopt;
  }

opt<bool>
do_accept_negation_opt(Token_Stream& tstrm)
  {
    // negation ::=
    //   "!" | "not"
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_notl });
    if(kpunct)
      return true;

    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_not });
    if(qkwrd)
      return true;

    return nullopt;
  }

opt<cow_vector<phsh_string>>
do_accept_variable_declarator_opt(Token_Stream& tstrm)
  {
    // variable-declarator ::=
    //   identifier | structured-binding-array | structured-binding-object
    // structured-binding-array ::=
    //   "[" identifier * "]"
    // structured-binding-object ::=
    //   "{" identifier * "}"
    cow_vector<phsh_string> names;
    bool comma_allowed = false;

    auto qname = do_accept_identifier_opt(tstrm, true);
    if(qname) {
      // Accept a single identifier.
      names.emplace_back(::std::move(*qname));
      return ::std::move(names);
    }

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of brackets and separated by commas.
      // There must be at least one identifier.
      for(;;) {
        qname = do_accept_identifier_opt(tstrm, true);
        if(!qname)
          break;

        names.emplace_back(::std::move(*qname));

        // Look for the separator.
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        comma_allowed = !kpunct;
        if(!kpunct)
          break;
      }

      if(names.size() < 1)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_identifier_expected, tstrm.next_sloc());

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
      if(!kpunct)
        throw Compiler_Error(Compiler_Error::M_add_format(),
                  comma_allowed ? compiler_status_closed_bracket_or_comma_expected
                      : compiler_status_closed_bracket_or_identifier_expected, tstrm.next_sloc(),
                  "[unmatched `[` at '$1']", op_sloc);

      // Make the list different from a plain, sole one.
      names.insert(0, sref("["));
      names.emplace_back(sref("]"));
      return ::std::move(names);
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of braces and separated by commas.
      // There must be at least one identifier.
      for(;;) {
        qname = do_accept_identifier_opt(tstrm, true);
        if(!qname)
          break;

        names.emplace_back(::std::move(*qname));

        // Look for the separator.
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        comma_allowed = !kpunct;
        if(!kpunct)
          break;
      }

      if(names.size() < 1)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_identifier_expected, tstrm.next_sloc());

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
      if(!kpunct)
        throw Compiler_Error(Compiler_Error::M_add_format(),
                  comma_allowed ? compiler_status_closed_brace_or_comma_expected
                      : compiler_status_closed_brace_or_identifier_expected, tstrm.next_sloc(),
                  "[unmatched `{` at '$1']", op_sloc);

      // Make the list different from a plain, sole one.
      names.insert(0, sref("{"));
      names.emplace_back(sref("}"));
      return ::std::move(names);
    }

    return nullopt;
  }

// Scope flags
// Each type of scope is assigned a unique bit. This determines whether `break`
// or `continue` is allowed inside it. Blocks may be nested, so flags may be OR'd.
enum Scope_Flags : uint32_t
  {
    scope_plain      = 0b00000000,
    scope_switch     = 0b00000001,
    scope_while      = 0b00000010,
    scope_for        = 0b00000100,

    scope_xbreak     = 0b00000111,
    scope_xcontinue  = 0b00000110,
  };

constexpr
Scope_Flags
operator&(Scope_Flags x, Scope_Flags y) noexcept
  { return Scope_Flags(uint32_t(x) & uint32_t(y));  }

constexpr
Scope_Flags
operator|(Scope_Flags x, Scope_Flags y) noexcept
  { return Scope_Flags(uint32_t(x) | uint32_t(y));  }

// Accept a statement; a blockt is converted to a single statement.
opt<Statement>
do_accept_statement_opt(Token_Stream& tstrm, Scope_Flags scfl);

// Accept a statement; a non-block statement is converted to a block consisting of
// a single statement.
opt<Statement::S_block>
do_accept_statement_as_block_opt(Token_Stream& tstrm, Scope_Flags scfl);

bool
do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm);

bool
do_accept_expression_as_rvalue(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    auto sloc = tstrm.next_sloc();
    if(!do_accept_expression(units, tstrm))
      return false;

    Expression_Unit::S_argument_finish xunit = { ::std::move(sloc), false };
    units.emplace_back(::std::move(xunit));
    return true;
  }

opt<Statement::S_expression>
do_accept_expression_opt(Token_Stream& tstrm)
  {
    cow_vector<Expression_Unit> units;
    auto sloc = tstrm.next_sloc();
    if(!do_accept_expression(units, tstrm))
      return nullopt;

    Statement::S_expression xexpr = { ::std::move(sloc), ::std::move(units) };
    return ::std::move(xexpr);
  }

opt<Statement::S_expression>
do_accept_expression_as_rvalue_opt(Token_Stream& tstrm)
  {
    cow_vector<Expression_Unit> units;
    auto sloc = tstrm.next_sloc();
    if(!do_accept_expression_as_rvalue(units, tstrm))
      return nullopt;

    Statement::S_expression xexpr = { ::std::move(sloc), ::std::move(units) };
    return ::std::move(xexpr);
  }

opt<Statement::S_block>
do_accept_block_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // block ::=
    //   "{" statement * "}"
    // statement-list ::=
    //   statement *
    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      return nullopt;

    cow_vector<Statement> body;
    while(auto qstmt = do_accept_statement_opt(tstrm, scfl))
      body.emplace_back(::std::move(*qstmt));

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_brace_or_statement_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", op_sloc);

    Statement::S_block xstmt = {  ::std::move(body) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_block_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    auto qblock = do_accept_block_opt(tstrm, scfl);
    if(!qblock)
      return nullopt;

    return ::std::move(*qblock);
  }

opt<Statement::S_expression>
do_accept_equal_initializer_opt(Token_Stream& tstrm)
  {
    // equal-initializer ::=
    //   "=" expression
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign });
    if(!kpunct)
      return nullopt;

    return do_accept_expression_opt(tstrm);
  }

opt<Statement::S_expression>
do_accept_ref_initializer_opt(Token_Stream& tstrm)
  {
    // ref-initializer ::=
    //   "->" expression
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_arrow });
    if(!kpunct)
      return nullopt;

    return do_accept_expression_opt(tstrm);
  }

opt<Statement>
do_accept_null_statement_opt(Token_Stream& tstrm)
  {
    // null-statement ::=
    //   ";"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      return nullopt;

    Statement::S_expression xstmt = { ::std::move(sloc), { } };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_variable_definition_opt(Token_Stream& tstrm)
  {
    // variable-definition ::=
    //   "var" variable-declarator equal-initailizer ? ( "," variable-declarator
    //   equal-initializer ? ) ?  ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_var });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Source_Location> slocs;
    cow_vector<cow_vector<phsh_string>> decls;
    cow_vector<Statement::S_expression> inits;

    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured binding.
      auto sloc = tstrm.next_sloc();
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit)
        qinit.emplace();

      slocs.emplace_back(::std::move(sloc));
      decls.emplace_back(::std::move(*qdecl));
      inits.emplace_back(::std::move(*qinit));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_variables xstmt = { false, ::std::move(slocs), ::std::move(decls),
                                     ::std::move(inits) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_immutable_variable_definition_opt(Token_Stream& tstrm)
  {
    // immutable-variable-definition ::=
    //   "const" variable-declarator equal-initailizer ( "," variable-declarator
    //   equal-initializer ) ? ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_const });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Source_Location> slocs;
    cow_vector<cow_vector<phsh_string>> decls;
    cow_vector<Statement::S_expression> inits;

    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured binding.
      auto sloc = tstrm.next_sloc();
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_equals_sign_expected, tstrm.next_sloc());

      slocs.emplace_back(::std::move(sloc));
      decls.emplace_back(::std::move(*qdecl));
      inits.emplace_back(::std::move(*qinit));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_variables xstmt = { true, ::std::move(slocs), ::std::move(decls),
                                     ::std::move(inits) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_reference_definition_opt(Token_Stream& tstrm)
  {
    // reference-definition ::=
    //   "ref" identifier ref-initailizer ( "," identifier ref-initializer ) ? ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_ref });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Source_Location> slocs;
    cow_vector<phsh_string> names;
    cow_vector<Statement::S_expression> inits;

    for(;;) {
      // Accept the name of this declared reference.
      auto sloc = tstrm.next_sloc();
      auto qname = do_accept_identifier_opt(tstrm, true);
      if(!qname)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_ref_initializer_opt(tstrm);
      if(!qinit)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_arrow_expected, tstrm.next_sloc());

      slocs.emplace_back(::std::move(sloc));
      names.emplace_back(::std::move(*qname));
      inits.emplace_back(::std::move(*qinit));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_references xstmt = { ::std::move(slocs), ::std::move(names),
                                      ::std::move(inits) };
    return ::std::move(xstmt);
  }

opt<cow_vector<phsh_string>>
do_accept_parameter_list_opt(Token_Stream& tstrm)
  {
    // parameter-list ::=
    //   "..." | identifier ( "," parameter-list ? ) ?
    cow_vector<phsh_string> params;

    for(;;) {
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
      if(kpunct) {
        params.emplace_back(sref("..."));
        break;
      }

      auto qname = do_accept_identifier_opt(tstrm, true);
      if(!qname)
        break;

      params.emplace_back(::std::move(*qname));

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }
    return ::std::move(params);
  }

opt<Statement>
do_accept_function_definition_opt(Token_Stream& tstrm)
  {
    // function-definition ::=
    //   "func" identifier "(" parameter-list ? ")" block
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd)
      return nullopt;

    auto qname = do_accept_identifier_opt(tstrm, true);
    if(!qname)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto kparams = do_accept_parameter_list_opt(tstrm);
    if(!kparams)
      kparams.emplace();

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qbody = do_accept_block_opt(tstrm, scope_plain);
    if(!qbody)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_brace_expected, tstrm.next_sloc());

    Statement::S_function xstmt = { ::std::move(sloc), ::std::move(*qname),
                                    ::std::move(*kparams), ::std::move(qbody->stmts) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_expression_statement_opt(Token_Stream& tstrm)
  {
    // expression-statement ::=
    //   expression ";"
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr)
      return nullopt;

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_expression xstmt = { ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_if_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // if-statement ::=
    //   "if" negation ? "(" expression ")" statement ( "else" statement ) ?
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_if });
    if(!qkwrd)
      return nullopt;

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qbtrue = do_accept_statement_as_block_opt(tstrm, scfl);
    if(!qbtrue)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    decltype(qbtrue) qbfalse;
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_else });
    if(qkwrd)
      qbfalse = do_accept_statement_as_block_opt(tstrm, scfl);

    if(qkwrd && !qbfalse)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    if(!qbfalse)
      qbfalse.emplace();  // empty `else` block

    Statement::S_if xstmt = { *kneg, ::std::move(*qcond), ::std::move(*qbtrue),
                              ::std::move(*qbfalse) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_switch_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // switch-statement ::=
    //   "switch" "(" expression ")" switch-block
    // switch-block ::=
    //   "{" swtich-clause * "}"
    // switch-clause ::=
    //   ( "case" expression | "default" ) ":" statement *
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch });
    if(!qkwrd)
      return nullopt;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qctrl = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qctrl)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    // Parse the block by hand.
    cow_vector<Statement::S_expression> labels;
    cow_vector<Statement::S_block> bodies;

    op_sloc = tstrm.next_sloc();
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_brace_expected, tstrm.next_sloc());

    for(;;) {
      qkwrd = do_accept_keyword_opt(tstrm, { keyword_case, keyword_default });
      if(!qkwrd)
        break;

      if(*qkwrd == keyword_case) {
        // The `case` label requires an expression argument.
        auto qlabel = do_accept_expression_as_rvalue_opt(tstrm);
        if(!qlabel)
          throw Compiler_Error(Compiler_Error::M_status(),
                    compiler_status_expression_expected, tstrm.next_sloc());

        labels.emplace_back(::std::move(*qlabel));
      }
      else
        // The `default` label takes no argument.
        labels.emplace_back();

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
      if(!kpunct)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_colon_expected, tstrm.next_sloc());

      cow_vector<Statement> body;
      while(auto qstmt = do_accept_statement_opt(tstrm, scfl | scope_switch))
        body.emplace_back(::std::move(*qstmt));

      Statement::S_block xstmt = { ::std::move(body) };
      bodies.emplace_back(::std::move(xstmt));
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_brace_or_switch_clause_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", op_sloc);

    Statement::S_switch xstmt = { ::std::move(*qctrl), ::std::move(labels), ::std::move(bodies) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_do_while_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // do-while-statement ::=
    //   "do" statement "while" negation ? "(" expression ")" ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_do });
    if(!qkwrd)
      return nullopt;

    auto qblock = do_accept_statement_as_block_opt(tstrm, scfl | scope_while);
    if(!qblock)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_keyword_while_expected, tstrm.next_sloc());

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_do_while xstmt = { ::std::move(*qblock), *kneg, ::std::move(*qcond) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_while_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // while-statement ::=
    //   "while" negation ? "(" expression ")" statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd)
      return nullopt;

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_statement_as_block_opt(tstrm, scfl | scope_while);
    if(!qblock)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    Statement::S_while xstmt = { *kneg, ::std::move(*qcond), ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_for_complement_range_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                                   Scope_Flags scfl)
  {
    // for-complement-range ::=
    //   "each" identifier "," identifier "->" expression ")" statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_each });
    if(!qkwrd)
      return nullopt;

    auto qkname = do_accept_identifier_opt(tstrm, true);
    if(!qkname)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_comma_expected, tstrm.next_sloc());

    auto qvname = do_accept_identifier_opt(tstrm, true);
    if(!qvname)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_arrow });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_arrow_expected, tstrm.next_sloc());

    auto qinit = do_accept_expression_opt(tstrm);
    if(!qinit)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_statement_as_block_opt(tstrm, scfl | scope_for);
    if(!qblock)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    Statement::S_for_each xstmt = { ::std::move(*qkname), ::std::move(*qvname),
                                    ::std::move(*qinit), ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

Statement::S_block
do_blockify_statement(Statement&& stmt)
  {
    // Make a block consisting of a single statement.
    cow_vector<Statement> stmts;
    stmts.emplace_back(::std::move(stmt));
    Statement::S_block xblock = { ::std::move(stmts) };
    return xblock;
  }

opt<Statement::S_block>
do_accept_for_initializer_opt(Token_Stream& tstrm)
  {
    // for-initializer ::=
    //   null-statement | variable-definition | immutable-variable-definition |
    //   expression-statement
    auto qinit = do_accept_null_statement_opt(tstrm);
    if(qinit)
      return do_blockify_statement(::std::move(*qinit));

    qinit = do_accept_variable_definition_opt(tstrm);
    if(qinit)
      return do_blockify_statement(::std::move(*qinit));

    qinit = do_accept_immutable_variable_definition_opt(tstrm);
    if(qinit)
      return do_blockify_statement(::std::move(*qinit));

    qinit = do_accept_expression_statement_opt(tstrm);
    if(qinit)
      return do_blockify_statement(::std::move(*qinit));

    return nullopt;
  }

Statement::S_expression&
do_set_empty_expression(opt<Statement::S_expression>& qexpr, const Token_Stream& tstrm)
  {
    auto& expr = qexpr.emplace();
    expr.sloc = tstrm.next_sloc();
    return expr;
  }

opt<Statement>
do_accept_for_complement_triplet_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                                     Scope_Flags scfl)
  {
    // for-complement-triplet ::=
    //   for-initializer expression ? ";" expression ? ")" statement
    auto qinit = do_accept_for_initializer_opt(tstrm);
    if(!qinit)
      return nullopt;

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      do_set_empty_expression(qcond, tstrm);

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    auto kstep = do_accept_expression_opt(tstrm);
    if(!kstep)
      do_set_empty_expression(kstep, tstrm);

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_statement_as_block_opt(tstrm, scfl | scope_for);
    if(!qblock)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    Statement::S_for xstmt = { ::std::move(*qinit), ::std::move(*qcond),
                               ::std::move(*kstep), ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_for_complement_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                             Scope_Flags scfl)
  {
    // for-complement ::=
    //   for-complement-range | for-complement-triplet
    if(auto qcompl = do_accept_for_complement_range_opt(tstrm, op_sloc, scfl))
      return qcompl;

    if(auto qcompl = do_accept_for_complement_triplet_opt(tstrm, op_sloc, scfl))
      return qcompl;

    return nullopt;
  }

opt<Statement>
do_accept_for_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // for-statement ::=
    //   "for" "(" for-complement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_for });
    if(!qkwrd)
      return nullopt;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcompl = do_accept_for_complement_opt(tstrm, op_sloc, scfl);
    if(!qcompl)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_for_statement_initializer_expected, tstrm.next_sloc());

    return ::std::move(*qcompl);
  }

opt<pair<Jump_Target, Scope_Flags>>
do_accept_jump_target_opt(Token_Stream& tstrm, initializer_list<Keyword> accept)
  {
    auto qkwrd = do_accept_keyword_opt(tstrm, accept);
    if(!qkwrd)
      return nullopt;

    switch(weaken_enum(*qkwrd)) {
      case keyword_switch:
        return ::std::make_pair(jump_target_switch, scope_switch);

      case keyword_while:
        return ::std::make_pair(jump_target_while, scope_while);

      case keyword_for:
        return ::std::make_pair(jump_target_for, scope_for);

      default:
        ROCKET_ASSERT(false);
    }
  }

opt<Statement>
do_accept_break_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // break-statement ::=
    //   "break" break-target ? ";"
    // break-target ::=
    //   "switch" | "while" | "for"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_break });
    if(!qkwrd)
      return nullopt;

    auto ktarg = do_accept_jump_target_opt(tstrm, { keyword_switch,
                                                    keyword_while, keyword_for });
    if(!ktarg)
      ktarg.emplace(jump_target_unspec, scope_xbreak);

    if(!(scfl & ktarg->second))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_break_no_matching_scope, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_break xstmt = { ::std::move(sloc), ktarg->first };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_continue_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // continue-statement ::=
    //   "continue" continue-target ? ";"
    // continue-target ::=
    //   "while" | "for"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_continue });
    if(!qkwrd)
      return nullopt;

    auto ktarg = do_accept_jump_target_opt(tstrm, { keyword_while, keyword_for });
    if(!ktarg)
      ktarg.emplace(jump_target_unspec, scope_xcontinue);

    if(!(scfl & ktarg->second))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_continue_no_matching_scope, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_continue xstmt = { ::std::move(sloc), ktarg->first };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_throw_statement_opt(Token_Stream& tstrm)
  {
    // throw-statement ::=
    //   "throw" expression ";"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_throw });
    if(!qkwrd)
      return nullopt;

    auto kexpr = do_accept_expression_as_rvalue_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_throw xstmt = { ::std::move(sloc), ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<bool>
do_accept_reference_specifier_opt(Token_Stream& tstrm)
  {
    // reference-specifier ::=
    //   "ref" | "->"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_ref });
    if(qkwrd)
      return true;

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_arrow });
    if(kpunct)
      return true;

    return nullopt;
  }

opt<Statement>
do_accept_return_statement_opt(Token_Stream& tstrm)
  {
    // return-statement ::=
    //   "return" argument ? ";"
    // argument ::=
    //   reference-specifier ? expression | expression
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_return });
    if(!qkwrd)
      return nullopt;

    auto arg_sloc = tstrm.next_sloc();
    auto refsp = do_accept_reference_specifier_opt(tstrm);
    Statement::S_expression xexpr;
    bool succ = do_accept_expression(xexpr.units, tstrm);
    if(refsp && !succ)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                succ ? compiler_status_semicolon_expected : compiler_status_expression_expected,
                tstrm.next_sloc());

    xexpr.sloc = ::std::move(arg_sloc);
    Statement::S_return xstmt = { ::std::move(sloc), refsp.value_or(false), ::std::move(xexpr) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_assert_statement_opt(Token_Stream& tstrm)
  {
    // assert-statement ::=
    //   "assert" expression ( ":" string-literal ) ? ";"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_assert });
    if(!qkwrd)
      return nullopt;

    auto kexpr = do_accept_expression_as_rvalue_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    opt<cow_string> kmsg;
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(kpunct)
      kmsg = do_accept_string_literal_opt(tstrm);

    if(kpunct && !kmsg)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_string_literal_expected, tstrm.next_sloc());

    if(!kmsg)
      kmsg.emplace(sref("[no message]"));

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_assert xstmt = { ::std::move(sloc), ::std::move(*kexpr), ::std::move(*kmsg) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_try_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // try-statement ::=
    //   "try" statement "catch" "(" identifier ")" statement
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_try });
    if(!qkwrd)
      return nullopt;

    auto qbtry = do_accept_statement_as_block_opt(tstrm, scfl);
    if(!qbtry)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    // Note that this is the location of the `catch` block.
    auto sloc_catch = tstrm.next_sloc();
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_catch });
    if(!qkwrd)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_keyword_catch_expected, tstrm.next_sloc());

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto kexcept = do_accept_identifier_opt(tstrm, true);
    if(!kexcept)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qbcatch = do_accept_statement_as_block_opt(tstrm, scfl);
    if(!qbcatch)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    Statement::S_try xstmt = { ::std::move(sloc), ::std::move(*qbtry), ::std::move(sloc_catch),
                               ::std::move(*kexcept), ::std::move(*qbcatch) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_defer_statement_opt(Token_Stream& tstrm)
  {
    // defer-statement ::=
    //  "defer" expression ";"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_defer });
    if(!qkwrd)
      return nullopt;

    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_defer xstmt = { ::std::move(sloc), ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<Statement>
do_accept_nonblock_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // nonblock-statement ::=
    //   null-statement |
    //   variable-definition | immutable-variable-definition | reference-definition |
    //   function-definition | expression-statement |
    //   if-statement | switch-statement | do-while-statement | while-statement | for-statement |
    //   break-statement | continue-statement | throw-statement | return-statement |
    //   assert-statement | try-statement | defer-statement
    if(auto qstmt = do_accept_null_statement_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_variable_definition_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_immutable_variable_definition_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_reference_definition_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_function_definition_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_expression_statement_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_if_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_switch_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_do_while_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_while_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_for_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_break_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_continue_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_throw_statement_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_return_statement_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_assert_statement_opt(tstrm))
      return qstmt;

    if(auto qstmt = do_accept_try_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_defer_statement_opt(tstrm))
      return qstmt;

    return nullopt;
  }

opt<Statement>
do_accept_statement_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // statement ::=
    //   block | nonblock-statement
    const auto sentry = tstrm.copy_recursion_sentry();

    if(auto qstmt = do_accept_block_statement_opt(tstrm, scfl))
      return qstmt;

    if(auto qstmt = do_accept_nonblock_statement_opt(tstrm, scfl))
      return qstmt;

    return nullopt;
  }

opt<Statement::S_block>
do_accept_statement_as_block_opt(Token_Stream& tstrm, Scope_Flags scfl)
  {
    // statement ::=
    //   block | nonblock-statement
    const auto sentry = tstrm.copy_recursion_sentry();

    auto qblock = do_accept_block_opt(tstrm, scfl);
    if(qblock)
      return qblock;

    auto qstmt = do_accept_nonblock_statement_opt(tstrm, scfl);
    if(qstmt)
      return do_blockify_statement(::std::move(*qstmt));

    return nullopt;
  }

struct Prefix_Keyword_Xop
  {
    Keyword kwrd;
    Xop xop;
  }
constexpr s_prefix_keyword_xop[] =
  {
    { keyword_unset,     xop_unset    },
    { keyword_countof,   xop_countof  },
    { keyword_typeof,    xop_typeof   },
    { keyword_not,       xop_notl     },
    { keyword_abs,       xop_abs      },
    { keyword_sign,      xop_sign     },
    { keyword_sqrt,      xop_sqrt     },
    { keyword_isnan,     xop_isnan    },
    { keyword_isinf,     xop_isinf    },
    { keyword_round,     xop_round    },
    { keyword_floor,     xop_floor    },
    { keyword_ceil,      xop_ceil     },
    { keyword_trunc,     xop_trunc    },
    { keyword_iround,    xop_iround   },
    { keyword_ifloor,    xop_ifloor   },
    { keyword_iceil,     xop_iceil    },
    { keyword_itrunc,    xop_itrunc   },
    { keyword_lzcnt,     xop_lzcnt    },
    { keyword_tzcnt,     xop_tzcnt    },
    { keyword_popcnt,    xop_popcnt   },
  };

constexpr
bool
operator==(const Prefix_Keyword_Xop& lhs, Keyword rhs) noexcept
  {
    return lhs.kwrd == rhs;
  }

struct Prefix_Punctuator_Xop
  {
    Punctuator punct;
    Xop xop;
  }
constexpr s_prefix_punctuator_xop[] =
  {
    { punctuator_add,   xop_pos      },
    { punctuator_sub,   xop_neg      },
    { punctuator_notb,  xop_notb     },
    { punctuator_notl,  xop_notl     },
    { punctuator_inc,   xop_inc_pre  },
    { punctuator_dec,   xop_dec_pre  },
  };

constexpr
bool
operator==(const Prefix_Punctuator_Xop& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

bool
do_accept_prefix_operator(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // prefix-operator ::=
    //   "+" | "-" | "~" | "!" | "++" | "--" |
    //   "unset" | "countof" | "typeof" | "not" |
    //   "__abs" | "__sqrt" | "__sign" | "__isnan" | "__isinf" |
    //   "__round" | "__floor" | "__ceil" | "__trunc" | "__iround" | "__ifloor" |
    //   "__iceil" | "__itrunc" | "__lzcnt" | "__tzcnt" | "__popcnt"
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return false;

    auto sloc = qtok->sloc();
    if(qtok->is_keyword()) {
      auto qconf = ::rocket::find(s_prefix_keyword_xop, qtok->as_keyword());
      if(!qconf)
        return false;

      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }

    if(qtok->is_punctuator()) {
      auto qconf = ::rocket::find(s_prefix_punctuator_xop, qtok->as_punctuator());
      if(!qconf)
        return false;

      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }

    return false;
  }

bool
do_accept_local_reference(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get an identifier.
    auto sloc = tstrm.next_sloc();
    auto qname = do_accept_identifier_opt(tstrm, false);
    if(!qname)
      return false;

    // Replace special names. This is what macros in C do.
    if(*qname == "__file") {
      Expression_Unit::S_literal xunit = { sloc.file() };
      units.emplace_back(::std::move(xunit));
      return true;
    }

    if(*qname == "__line") {
      Expression_Unit::S_literal xunit = { sloc.line() };
      units.emplace_back(::std::move(xunit));
      return true;
    }

    Expression_Unit::S_local_reference xunit = { sloc, ::std::move(*qname) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_global_reference(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // global-identifier ::=
    //   "__global" identifier
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_global });
    if(!qkwrd)
      return false;

    auto qname = do_accept_identifier_opt(tstrm, false);
    if(!qname)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    Expression_Unit::S_global_reference xunit = { sloc, ::std::move(*qname) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_literal(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get a literal as a `Value`.
    auto qval = do_accept_literal_opt(tstrm);
    if(!qval)
      return false;

    Expression_Unit::S_literal xunit = { ::std::move(*qval) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_this(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get the keyword `this`.
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_this });
    if(!qkwrd)
      return false;

    Expression_Unit::S_local_reference xunit = { sloc, sref("__this") };
    units.emplace_back(::std::move(xunit));
    return true;
  }

void
do_accept_closure_function_no_name(cow_vector<Expression_Unit>& units, Token_Stream& tstrm,
                                   Source_Location&& sloc)
  {
    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto kparams = do_accept_parameter_list_opt(tstrm);
    if(!kparams)
      kparams.emplace();

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    op_sloc = tstrm.next_sloc();
    auto qblock = do_accept_block_opt(tstrm, scope_plain);
    if(!qblock) {
      // Try `equal-initializer`.
      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(qinit) {
        // Behave as if it was a `return-statement` by value.
        Statement::S_return xstmt = { ::std::move(op_sloc), false, ::std::move(*qinit) };
        qblock = do_blockify_statement(::std::move(xstmt));
      }
    }
    if(!qblock) {
      // Try `ref-initializer`.
      auto qinit = do_accept_ref_initializer_opt(tstrm);
      if(qinit) {
        // Behave as if it was a `return-statement` by reference.
        Statement::S_return xstmt = { ::std::move(op_sloc), true, ::std::move(*qinit) };
        qblock = do_blockify_statement(::std::move(xstmt));
      }
    }
    if(!qblock)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_brace_or_equal_initializer_expected, tstrm.next_sloc());

    auto unique_name = format_string("__closure:$1:$2", sloc.line(), sloc.column());

    Expression_Unit::S_closure_function xunit = { ::std::move(sloc), ::std::move(unique_name),
                                                  ::std::move(*kparams),
                                                  ::std::move(qblock->stmts) };
    units.emplace_back(::std::move(xunit));
  }

bool
do_accept_closure_function(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // closure-function ::=
    //   "func" "(" parameter-list ? ")" closure-body
    // closure-body ::=
    //   block | equal-initializer | ref-initializer
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd)
      return false;

    do_accept_closure_function_no_name(units, tstrm, ::std::move(sloc));
    return true;
  }

bool
do_accept_unnamed_array(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // unnamed-array ::=
    //   "[" array-element-list ? "]"
    // array-element-list ::=
    //   expression ( ( "," | ";" ) array-element-list ? ) ?
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(!kpunct)
      return false;

    uint32_t nelems = 0;
    bool comma_allowed = false;

    for(;;) {
      if(!do_accept_expression_as_rvalue(units, tstrm))
        break;

      nelems += 1;
      if(nelems >= 0x100000)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_too_many_elements, tstrm.next_sloc());

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                comma_allowed ? compiler_status_closed_bracket_or_comma_expected
                    : compiler_status_closed_bracket_or_expression_expected, tstrm.next_sloc(),
                "[unmatched `[` at '$1']", sloc);

    Expression_Unit::S_unnamed_array xunit = { sloc, nelems };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_unnamed_object(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // unnamed-object ::=
    //   "{" object-member-list "}"
    // object-member-list ::=
    //   ( string-literal | identifier ) ( "=" | ":" ) expression ( ( "," | ";" )
    //   object-member-list ? ) ?
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      return false;

    cow_vector<phsh_string> keys;
    bool comma_allowed = false;

    for(;;) {
      auto op_sloc = tstrm.next_sloc();
      auto qkey = do_accept_json5_key_opt(tstrm);
      if(!qkey)
        break;

      if(::rocket::find(keys, *qkey))
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_duplicate_key_in_object, op_sloc);

      // Look for the value with an initiator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign, punctuator_colon });
      if(!kpunct)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_equals_sign_or_colon_expected, tstrm.next_sloc());

      if(!do_accept_expression_as_rvalue(units, tstrm))
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_expression_expected, tstrm.next_sloc());

      // Look for the separator.
      keys.emplace_back(::std::move(*qkey));
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                comma_allowed ? compiler_status_closed_brace_or_comma_expected
                    : compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", sloc);

    Expression_Unit::S_unnamed_object xunit = { sloc, ::std::move(keys) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_nested_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // nested-expression ::=
    //   "(" expression ")"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      return false;

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", sloc);

    return true;
  }

bool
do_accept_fused_multiply_add(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // fused-multiply-add ::=
    //   "__fma" "(" expression "," expression "," expression ")"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_fma });
    if(!qkwrd)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_operator_rpn xunit = { sloc, xop_fma, false };
    units.emplace_back(::std::move(xunit));
    return true;
  }

constexpr Prefix_Keyword_Xop s_prefix_keyword_binary_xop[] =
  {
    { keyword_addm, xop_addm },
    { keyword_subm, xop_subm },
    { keyword_mulm, xop_mulm },
    { keyword_adds, xop_adds },
    { keyword_subs, xop_subs },
    { keyword_muls, xop_muls },
  };

opt<Expression_Unit>
do_accept_prefix_binary_operator_opt(Token_Stream& tstrm)
  {
    // prefix-binary-operator ::=
    //   "__addm" | "__subm" | "__mulm" | "__adds" | "__subs" | "__muls"
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    auto sloc = qtok->sloc();
    if(qtok->is_keyword()) {
      auto qconf = ::rocket::find(s_prefix_keyword_binary_xop, qtok->as_keyword());
      if(!qconf)
        return nullopt;

      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, false };
      return ::std::move(xunit);
    }
    return nullopt;
  }

bool
do_accept_prefix_binary_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // prefix-binary-expression ::=
    //   prefix-binary-operator "(" expression "," expression ")"
    auto qxunit = do_accept_prefix_binary_operator_opt(tstrm);
    if(!qxunit)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    units.emplace_back(::std::move(*qxunit));
    return true;
  }

bool
do_accept_catch_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // catch-expression ::=
    //   "catch" "(" expression ")"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_catch });
    if(!qkwrd)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    Expression_Unit::S_catch xunit;
    if(!do_accept_expression(xunit.operand, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_variadic_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // variadic-function-call ::=
    //   "__vcall" "(" expression "," expression ")"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_vcall });
    if(!qkwrd)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_variadic_call xunit = { ::std::move(sloc) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_import_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // import-function-call ::=
    //   "import" "(" argument-list ")"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_import });
    if(!qkwrd)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    uint32_t nargs = 0;
    bool comma_allowed = false;

    for(;;) {
      auto arg_sloc = tstrm.next_sloc();
      auto refsp = do_accept_reference_specifier_opt(tstrm);
      bool succ = do_accept_expression(units, tstrm);
      if(refsp && !succ)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_expression_expected, tstrm.next_sloc());

      if(!refsp && !succ)
        break;

      Expression_Unit::S_argument_finish xunit = { ::std::move(arg_sloc), refsp.value_or(false) };
      units.emplace_back(::std::move(xunit));
      nargs += 1;
      if(nargs >= 0x100000)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_too_many_elements, tstrm.next_sloc());

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    if(nargs < 1)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_argument_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                comma_allowed ? compiler_status_closed_parenthesis_or_comma_expected
                    : compiler_status_closed_parenthesis_or_argument_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_import_call xunit = { ::std::move(sloc), nargs };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_primary_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // primary-expression ::=
    //   identifier | global-identifier | literal | "this" | closure-function | unnamed-array |
    //   unnamed-object | nested-expression | fused-multiply-add | prefix-binary-expression |
    //   catch-expression | variadic-function-call | import-function-call
    if(do_accept_local_reference(units, tstrm))
      return true;

    if(do_accept_global_reference(units, tstrm))
      return true;

    if(do_accept_literal(units, tstrm))
      return true;

    if(do_accept_this(units, tstrm))
      return true;

    if(do_accept_closure_function(units, tstrm))
      return true;

    if(do_accept_unnamed_array(units, tstrm))
      return true;

    if(do_accept_unnamed_object(units, tstrm))
      return true;

    if(do_accept_nested_expression(units, tstrm))
      return true;

    if(do_accept_fused_multiply_add(units, tstrm))
      return true;

    if(do_accept_prefix_binary_expression(units, tstrm))
      return true;

    if(do_accept_catch_expression(units, tstrm))
      return true;

    if(do_accept_variadic_function_call(units, tstrm))
      return true;

    if(do_accept_import_function_call(units, tstrm))
      return true;

    return false;
  }

struct Postfix_Punctuator_Xop
  {
    Punctuator punct;
    Xop xop;
  }
constexpr s_postfix_punctuator_xop[] =
  {
    { punctuator_inc,     xop_inc_post  },
    { punctuator_dec,     xop_dec_post  },
    { punctuator_head,    xop_head      },
    { punctuator_tail,    xop_tail      },
    { punctuator_random,  xop_random    },
  };

constexpr
bool
operator==(const Postfix_Punctuator_Xop& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

bool
do_accept_postfix_operator(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-operator ::=
    //   "++" | "--" | "[^]" | "[$]" | "[?]" |
    //   postfix-operator | postfix-function-call | postfix-subscript | postfix-member-access
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return false;

    auto sloc = qtok->sloc();
    if(qtok->is_punctuator()) {
      auto qconf = ::rocket::find(s_postfix_punctuator_xop, qtok->as_punctuator());
      if(!qconf)
        return false;

      // Return the postfix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    return false;
  }

bool
do_accept_postfix_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-function-call ::=
    //   "(" argument-list ? ")"
    // argument-list ::=
    //    argument ( "," argument-list ? ) ?
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      return false;

    uint32_t nargs = 0;
    bool comma_allowed = false;

    for(;;) {
      auto arg_sloc = tstrm.next_sloc();
      auto refsp = do_accept_reference_specifier_opt(tstrm);
      bool succ = do_accept_expression(units, tstrm);
      if(refsp && !succ)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_expression_expected, tstrm.next_sloc());

      if(!refsp && !succ)
        break;

      Expression_Unit::S_argument_finish xunit = { ::std::move(arg_sloc), refsp.value_or(false) };
      units.emplace_back(::std::move(xunit));
      nargs += 1;
      if(nargs >= 0x100000)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_too_many_elements, tstrm.next_sloc());

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                comma_allowed ? compiler_status_closed_parenthesis_or_comma_expected
                    : compiler_status_closed_parenthesis_or_argument_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", sloc);

    Expression_Unit::S_function_call xunit = { ::std::move(sloc), nargs };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_postfix_subscript(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-subscript ::=
    //   "[" expression "]"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(!kpunct)
      return false;

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_closed_bracket_expected, tstrm.next_sloc(),
                "[unmatched `[` at '$1']", sloc);

    Expression_Unit::S_operator_rpn xunit = { sloc, xop_subscr, false };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_postfix_member_access(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-member-access ::=
    //   "." ( string-literal | identifier )
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_dot });
    if(!kpunct)
      return false;

    auto qkey = do_accept_json5_key_opt(tstrm);
    if(!qkey)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    Expression_Unit::S_member_access xunit = { sloc, ::std::move(*qkey) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool
do_accept_infix_element(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // infix-element ::=
    //   prefix-operator * primary-expression postfix-operator *
    cow_vector<Expression_Unit> prefixes;
    bool succ;
    do
      succ = do_accept_prefix_operator(prefixes, tstrm);
    while(succ);

    // Get a `primary-expression` without suffixes.
    // Fail if some prefixes have been consumed but no primary expression can be accepted.
    succ = do_accept_primary_expression(units, tstrm);
    if(!succ) {
      if(prefixes.empty())
        return false;

      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());
    }

    // Collect suffixes.
    do {
      succ = do_accept_postfix_operator(units, tstrm) ||
             do_accept_postfix_function_call(units, tstrm) ||
             do_accept_postfix_subscript(units, tstrm) ||
             do_accept_postfix_member_access(units, tstrm);
    }
    while(succ);

    // Append prefixes in reverse order.
    // N.B. Prefix operators have lower precedence than postfix ones.
    units.append(prefixes.move_rbegin(), prefixes.move_rend());
    return true;
  }

opt<Infix_Element>
do_accept_infix_element_opt(Token_Stream& tstrm)
  {
    cow_vector<Expression_Unit> units;
    if(!do_accept_infix_element(units, tstrm))
      return nullopt;

    Infix_Element::S_head xelem = { ::std::move(units) };
    return ::std::move(xelem);
  }

opt<Infix_Element>
do_accept_infix_operator_ternary_opt(Token_Stream& tstrm)
  {
    // infix-operator-ternary ::=
    //   ( "?" | "?=" ) expression ":"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_quest_eq });
    if(!kpunct)
      return nullopt;

    bool assign = *kpunct == punctuator_quest_eq;
    cow_vector<Expression_Unit> btrue;
    if(!do_accept_expression(btrue, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_add_format(),
                compiler_status_colon_expected, tstrm.next_sloc(),
                "[unmatched `$1` at '$2']", stringify_punctuator(*kpunct), sloc);

    Infix_Element::S_ternary xelem = { ::std::move(sloc), assign, ::std::move(btrue), { } };
    return ::std::move(xelem);
  }

opt<Infix_Element>
do_accept_infix_operator_logical_and_opt(Token_Stream& tstrm)
  {
    // infix-operator-logical-and ::=
    //   "&&" | "&&=" | "and"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_andl, punctuator_andl_eq });
    if(!kpunct) {
      auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_and });
      if(!qkwrd)
        return nullopt;

      // `and` is the same with `&&`.
      kpunct.emplace(punctuator_andl);
    }

    bool assign = *kpunct == punctuator_andl_eq;
    Infix_Element::S_logical_and xelem = { sloc, assign, { } };
    return ::std::move(xelem);
  }

opt<Infix_Element>
do_accept_infix_operator_logical_or_opt(Token_Stream& tstrm)
  {
    // infix-operator-logical-or ::=
    //   "||" | "||=" | "or"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_orl, punctuator_orl_eq });
    if(!kpunct) {
      auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_or });
      if(!qkwrd)
        return nullopt;

      // `or` is the same with `||`.
      kpunct.emplace(punctuator_orl);
    }

    bool assign = *kpunct == punctuator_orl_eq;
    Infix_Element::S_logical_or xelem = { sloc, assign, { } };
    return ::std::move(xelem);
  }

opt<Infix_Element>
do_accept_infix_operator_coalescence_opt(Token_Stream& tstrm)
  {
    // infix-operator-coalescence ::=
    //   "??" | "??="
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_coales, punctuator_coales_eq });
    if(!kpunct)
      return nullopt;

    bool assign = *kpunct == punctuator_coales_eq;
    Infix_Element::S_coalescence xelem = { sloc, assign, { } };
    return ::std::move(xelem);
  }

struct Infix_Punctuator_Xop
  {
    Punctuator punct;
    Xop xop;
    bool assign;
  }
constexpr s_infix_punctuator_xop[] =
  {
    { punctuator_add,        xop_add,       0 },
    { punctuator_sub,        xop_sub,       0 },
    { punctuator_mul,        xop_mul,       0 },
    { punctuator_div,        xop_div,       0 },
    { punctuator_mod,        xop_mod,       0 },
    { punctuator_andb,       xop_andb,      0 },
    { punctuator_orb,        xop_orb,       0 },
    { punctuator_xorb,       xop_xorb,      0 },
    { punctuator_sla,        xop_sla,       0 },
    { punctuator_sra,        xop_sra,       0 },
    { punctuator_sll,        xop_sll,       0 },
    { punctuator_srl,        xop_srl,       0 },
    { punctuator_add_eq,     xop_add,       1 },
    { punctuator_sub_eq,     xop_sub,       1 },
    { punctuator_mul_eq,     xop_mul,       1 },
    { punctuator_div_eq,     xop_div,       1 },
    { punctuator_mod_eq,     xop_mod,       1 },
    { punctuator_andb_eq,    xop_andb,      1 },
    { punctuator_orb_eq,     xop_orb,       1 },
    { punctuator_xorb_eq,    xop_xorb,      1 },
    { punctuator_sla_eq,     xop_sla,       1 },
    { punctuator_sra_eq,     xop_sra,       1 },
    { punctuator_sll_eq,     xop_sll,       1 },
    { punctuator_srl_eq,     xop_srl,       1 },
    { punctuator_assign,     xop_assign,    1 },
    { punctuator_cmp_eq,     xop_cmp_eq,    0 },
    { punctuator_cmp_ne,     xop_cmp_ne,    0 },
    { punctuator_cmp_lt,     xop_cmp_lt,    0 },
    { punctuator_cmp_gt,     xop_cmp_gt,    0 },
    { punctuator_cmp_lte,    xop_cmp_lte,   0 },
    { punctuator_cmp_gte,    xop_cmp_gte,   0 },
    { punctuator_spaceship,  xop_cmp_3way,  0 },
  };

constexpr
bool
operator==(const Infix_Punctuator_Xop& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

opt<Infix_Element>
do_accept_infix_operator_general_opt(Token_Stream& tstrm)
  {
    // infix-operator-general ::=
    //   "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
    //   "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
    //   "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    auto sloc = qtok->sloc();
    if(qtok->is_punctuator()) {
      auto qconf = ::rocket::find(s_infix_punctuator_xop, qtok->as_punctuator());
      if(!qconf)
        return nullopt;

      // Return the infix operator and discard this token.
      tstrm.shift();
      Infix_Element::S_general xelem = { sloc, qconf->xop, qconf->assign, { } };
      return ::std::move(xelem);
    }
    return nullopt;
  }

opt<Infix_Element>
do_accept_infix_operator_opt(Token_Stream& tstrm)
  {
    // infix-operator ::=
    //   infix-operator-ternary | infix-operator-logical-and | infix-operator-logical-or |
    //   infix-operator-coalescence | infix-operator-general
    if(auto qelem = do_accept_infix_operator_ternary_opt(tstrm))
      return qelem;

    if(auto qelem = do_accept_infix_operator_logical_and_opt(tstrm))
      return qelem;

    if(auto qelem = do_accept_infix_operator_logical_or_opt(tstrm))
      return qelem;

    if(auto qelem = do_accept_infix_operator_coalescence_opt(tstrm))
      return qelem;

    if(auto qelem = do_accept_infix_operator_general_opt(tstrm))
      return qelem;

    return nullopt;
  }

bool
do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Check for stack overflows.
    const auto sentry = tstrm.copy_recursion_sentry();

    // expression ::=
    //   infix-element infix-carriage *
    // infix-carriage ::=
    //   infix-operator infix-element
    auto qelem = do_accept_infix_element_opt(tstrm);
    if(!qelem)
      return false;

    cow_vector<Infix_Element> stack;
    stack.emplace_back(::std::move(*qelem));

    for(;;) {
      auto qnext = do_accept_infix_operator_opt(tstrm);
      if(!qnext)
        break;

      if(!do_accept_infix_element(qnext->mut_junction(), tstrm))
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_expression_expected, tstrm.next_sloc());

      // Assignment operations have the lowest precedence and group from right to left.
      auto preced_limit = qnext->tell_precedence();
      if(preced_limit == precedence_assignment)
        preced_limit = static_cast<Precedence>(precedence_assignment - 1);

      // Collapse elements that have no lower precedence.
      while((stack.size() >= 2) && (stack.back().tell_precedence() <= preced_limit)) {
        qelem = ::std::move(stack.mut_back());
        stack.pop_back();
        qelem->extract(stack.mut_back().mut_junction());
      }
      stack.emplace_back(::std::move(*qnext));
    }

    // Collapse all elements so we eventually have a single node in the stack.
    while(stack.size() >= 2) {
      qelem = ::std::move(stack.mut_back());
      stack.pop_back();
      qelem->extract(stack.mut_back().mut_junction());
    }
    stack.mut_back().extract(units);
    return true;
  }

}  // namespace

Statement_Sequence::
~Statement_Sequence()
  {
  }

Statement_Sequence&
Statement_Sequence::
reload(Token_Stream&& tstrm)
  {
    // Destroy the contents of `*this` and reuse their storage, if any.
    cow_vector<Statement> stmts;
    this->m_stmts.clear();
    stmts.swap(this->m_stmts);

    // document ::=
    //   statement *
    while(auto qstmt = do_accept_statement_opt(tstrm, scope_plain))
      stmts.emplace_back(::std::move(*qstmt));

    // If there are any non-statement tokens left in the stream, fail.
    if(!tstrm.empty())
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_statement_expected, tstrm.next_sloc());

    // Succeed.
    this->m_stmts = ::std::move(stmts);
    return *this;
  }

Statement_Sequence&
Statement_Sequence::
reload_oneline(Token_Stream&& tstrm)
  {
    // Destroy the contents of `*this` and reuse their storage, if any.
    cow_vector<Statement> stmts;
    this->m_stmts.clear();
    stmts.swap(this->m_stmts);

    // Parse an expression. This is not optional.
    auto sloc = tstrm.next_sloc();
    Statement::S_expression xexpr;
    if(!do_accept_expression(xexpr.units, tstrm))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_expression_expected, tstrm.next_sloc());

    // If there are any non-statement tokens left in the stream, fail.
    if(!tstrm.empty())
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_invalid_expression, tstrm.next_sloc());


    // Build a return-statement that forwards the result by reference.
    xexpr.sloc = sloc;
    Statement::S_return xstmt = { ::std::move(sloc), true, ::std::move(xexpr) };
    stmts.emplace_back(::std::move(xstmt));

    // Succeed.
    this->m_stmts = ::std::move(stmts);
    return *this;
  }

}  // namespace asteria
