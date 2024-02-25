// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
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
    if(user_decl && (name[0] == '_') && (name[1] == '_'))
      throw Compiler_Error(xtc_format,
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

    if(qtok->is_keyword()) {
      // Treat the keyword as a plain identifier and discard this token.
      const char* kwstr = qtok->as_keyword_c_str();
      tstrm.shift();
      return ::rocket::sref(kwstr);
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
    //   "null" | "false" | "true" | string-literal | numeric-literal
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    if(qtok->is_keyword() && (qtok->as_keyword() == keyword_null)) {
      // Discard this token and create a `null`.
      tstrm.shift();
      return null_value;
    }

    if(qtok->is_keyword() && (qtok->as_keyword() == keyword_false)) {
      // Discard this token and create a `false`.
      tstrm.shift();
      return false;
    }

    if(qtok->is_keyword() && (qtok->as_keyword() == keyword_true)) {
      // Discard this token and create a `true`.
      tstrm.shift();
      return true;
    }

    if(qtok->is_integer_literal()) {
      // Copy the value and discard this token.
      int64_t val = qtok->as_integer_literal();
      tstrm.shift();
      return val;
    }

    if(qtok->is_real_literal()) {
      // Copy the value and discard this token.
      double val = qtok->as_real_literal();
      tstrm.shift();
      return val;
    }

    if(qtok->is_string_literal()) {
      // Copy the value and discard this token.
      cow_string val = qtok->as_string_literal();
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
    //  identifier-list ::=
    //    identifier ( "," identifier-list ? ) ?
    //  structured-binding-array ::=
    //    "[" identifier-list "]"
    //  structured-binding-object ::=
    //    "{" identifier-list "}"
    cow_vector<phsh_string> names;
    bool comma_allowed = false;

    auto op_sloc = tstrm.next_sloc();
    auto qname = do_accept_identifier_opt(tstrm, true);
    if(qname) {
      // Accept a single identifier.
      names.emplace_back(move(*qname));
      return move(names);
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of brackets and separated
      // by commas. There must be at least one identifier.
      names.emplace_back(&"[");

      for(;;) {
        auto name_sloc = tstrm.next_sloc();
        qname = do_accept_identifier_opt(tstrm, true);
        if(!qname)
          break;

        if(::rocket::find(names, *qname))
          throw Compiler_Error(xtc_status,
                    compiler_status_duplicate_name_in_structured_binding, name_sloc);

        names.emplace_back(move(*qname));

        // Look for the separator.
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        comma_allowed = !kpunct;
        if(!kpunct)
          break;
      }

      if(names.size() < 1)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
      if(!kpunct)
        throw Compiler_Error(xtc_status_format,
                  comma_allowed ? compiler_status_closing_bracket_or_comma_expected
                                : compiler_status_closing_bracket_or_identifier_expected,
                  tstrm.next_sloc(),
                  "[unmatched `[` at '$1']", op_sloc);

      names.emplace_back(&"]");
      return move(names);
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of braces and separated
      // by commas. There must be at least one identifier.
      names.emplace_back(&"{");

      for(;;) {
        auto name_sloc = tstrm.next_sloc();
        qname = do_accept_identifier_opt(tstrm, true);
        if(!qname)
          break;

        if(::rocket::find(names, *qname))
          throw Compiler_Error(xtc_status,
                    compiler_status_duplicate_name_in_structured_binding, name_sloc);

        names.emplace_back(move(*qname));

        // Look for the separator.
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        comma_allowed = !kpunct;
        if(!kpunct)
          break;
      }

      if(names.size() < 1)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
      if(!kpunct)
        throw Compiler_Error(xtc_status_format,
                  comma_allowed ? compiler_status_closing_brace_or_comma_expected
                                : compiler_status_closing_brace_or_identifier_expected,
                  tstrm.next_sloc(),
                  "[unmatched `{` at '$1']", op_sloc);

      names.emplace_back(&"}");
      return move(names);
    }

    return nullopt;
  }

// Each type of scope is assigned a unique bit. This determines whether `break`
// or `continue` is allowed. Blocks may be nested, so flags may be OR'd.
enum scope_flags : uint32_t
  {
    scope_plain   = 0b00000000,
    scope_switch  = 0b00000001,
    scope_while   = 0b00000010,
    scope_for     = 0b00000100,
  };

ROCKET_DEFINE_ENUM_OPERATORS(scope_flags)

opt<Statement>
do_accept_statement_opt(Token_Stream& tstrm, scope_flags scope);

opt<Statement::S_block>
do_accept_nondeclaration_statement_as_block_opt(Token_Stream& tstrm, scope_flags scope);

bool
do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm);

opt<Statement::S_expression>
do_accept_expression_opt(Token_Stream& tstrm)
  {
    auto sloc = tstrm.next_sloc();
    cow_vector<Expression_Unit> units;
    if(!do_accept_expression(units, tstrm))
      return nullopt;

    Statement::S_expression xexpr = { move(sloc), move(units) };
    return move(xexpr);
  }

bool
do_accept_expression_and_check(cow_vector<Expression_Unit>& units, Token_Stream& tstrm, bool by_ref)
  {
    auto sloc = tstrm.next_sloc();
    if(!do_accept_expression(units, tstrm))
      return false;

    if(by_ref || units.back().maybe_unreadable()) {
      // XXX: This may convert the result to an rvalue.
      Expression_Unit::S_check_argument xunit = { move(sloc), by_ref };
      units.emplace_back(move(xunit));
    }
    return true;
  }

opt<Statement::S_expression>
do_accept_expression_as_rvalue_opt(Token_Stream& tstrm)
  {
    cow_vector<Expression_Unit> units;
    auto sloc = tstrm.next_sloc();
    if(!do_accept_expression_and_check(units, tstrm, false))
      return nullopt;

    Statement::S_expression xexpr = { move(sloc), move(units) };
    return move(xexpr);
  }

opt<Statement::S_block>
do_accept_statement_block_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // statement-block ::=
    //   "{" statement * "}"
    // statement-list ::=
    //   statement *
    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      return nullopt;

    cow_vector<Statement> body;
    while(auto qstmt = do_accept_statement_opt(tstrm, scope))
      body.emplace_back(move(*qstmt));

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_brace_or_statement_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", op_sloc);

    Statement::S_block xstmt = { move(body) };
    return move(xstmt);
  }

opt<Statement::S_block>
do_accept_null_statement_opt(Token_Stream& tstrm)
  {
    // null-statement ::=
    //   ";"
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      return nullopt;

    Statement::S_block xstmt = { };
    return move(xstmt);
  }

opt<Statement::S_expression>
do_accept_equal_initializer_opt(Token_Stream& tstrm)
  {
    // equal-initializer ::=
    //   "=" expression
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign });
    if(!kpunct)
      return nullopt;

    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    return move(*kexpr);
  }

opt<Statement::S_expression>
do_accept_ref_initializer_opt(Token_Stream& tstrm)
  {
    // ref-initializer ::=
    //   "->" expression
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_arrow });
    if(!kpunct)
      return nullopt;

    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    return move(*kexpr);
  }

opt<Statement>
do_accept_variable_definition_opt(Token_Stream& tstrm)
  {
    // variable-definition ::=
    //   "var" variable-declarator equal-initializer ? ( "," variable-declarator
    //   equal-initializer ? ) ?  ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_var });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Statement::variable_declaration> decls;

    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured
      // binding.
      auto sloc = tstrm.next_sloc();
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit)
        qinit.emplace();

      Statement::variable_declaration decl = { move(sloc), move(*qdecl), move(*qinit) };
      decls.emplace_back(move(decl));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_variables xstmt = { false, move(decls) };
    return move(xstmt);
  }

opt<Statement>
do_accept_immutable_variable_definition_opt(Token_Stream& tstrm)
  {
    // immutable-variable-definition ::=
    //   "const" variable-declarator equal-initializer ( "," variable-declarator
    //   equal-initializer ) ? ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_const });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Statement::variable_declaration> decls;

    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured
      // binding.
      auto sloc = tstrm.next_sloc();
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit)
        throw Compiler_Error(xtc_status,
                  compiler_status_equals_sign_expected, tstrm.next_sloc());

      Statement::variable_declaration decl = { move(sloc), move(*qdecl), move(*qinit) };
      decls.emplace_back(move(decl));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_variables xstmt = { true, move(decls) };
    return move(xstmt);
  }

opt<Statement>
do_accept_reference_definition_opt(Token_Stream& tstrm)
  {
    // reference-definition ::=
    //   "ref" identifier ref-initializer ( "," identifier ref-initializer ) ? ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_ref });
    if(!qkwrd)
      return nullopt;

    // Each declaractor has its own source location.
    cow_vector<Statement::reference_declaration> decls;

    for(;;) {
      // Accept the name of this declared reference.
      auto sloc = tstrm.next_sloc();
      auto qname = do_accept_identifier_opt(tstrm, true);
      if(!qname)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());

      auto qinit = do_accept_ref_initializer_opt(tstrm);
      if(!qinit)
        throw Compiler_Error(xtc_status,
                  compiler_status_arrow_expected, tstrm.next_sloc());

      Statement::reference_declaration decl = { move(sloc), move(*qname), move(*qinit) };
      decls.emplace_back(move(decl));

      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct)
        break;
    }

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_references xstmt = { move(decls) };
    return move(xstmt);
  }

opt<Statement>
do_accept_function_definition_opt(Token_Stream& tstrm)
  {
    // function-definition ::=
    //   "func" identifier "(" parameter-list ? ")" statement-block
    // parameter-list ::=
    //   "..." | identifier ( "," parameter-list ? ) ?
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd)
      return nullopt;

    auto qname = do_accept_identifier_opt(tstrm, true);
    if(!qname)
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    cow_vector<phsh_string> params;
    bool comma_allowed = false;

    for(;;) {
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
      if(kpunct) {
        params.emplace_back(&"...");
        break;
      }

      auto param_sloc = tstrm.next_sloc();
      auto kparam = do_accept_identifier_opt(tstrm, true);
      if(!kparam)
        break;

      if(::rocket::find(params, *kparam))
        throw Compiler_Error(xtc_status,
                  compiler_status_duplicate_name_in_parameter_list, param_sloc);

      params.emplace_back(move(*kparam));

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_parenthesis_or_comma_expected
                              : compiler_status_closing_parenthesis_or_parameter_expected,
                tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    // Parse the function body. This is not the same as a block due to the implicit
    // `return;` at the end.
    op_sloc = tstrm.next_sloc();
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_brace_expected, tstrm.next_sloc());

    cow_vector<Statement> body;
    while(auto qstmt = do_accept_statement_opt(tstrm, scope_plain))
      body.emplace_back(move(*qstmt));

    auto cl_sloc = tstrm.next_sloc();
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_brace_or_statement_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", op_sloc);

    // Add an implicit return.
    Statement::S_return xendf = { move(cl_sloc), true, Statement::S_expression() };
    body.emplace_back(move(xendf));

    Statement::S_function xstmt = { move(sloc), move(*qname), move(params), move(body) };
    return move(xstmt);
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
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_expression xstmt = { move(*kexpr) };
    return move(xstmt);
  }

opt<Statement>
do_accept_if_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // if-statement ::=
    //   "if" negation ? "(" expression ")" nondeclaration-statement ( "else"
    //   nondeclaration-statement ) ?
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_if });
    if(!qkwrd)
      return nullopt;

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qbtrue = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope);
    if(!qbtrue)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    decltype(qbtrue) qbfalse;
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_else });
    if(qkwrd)
      qbfalse = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope);

    if(qkwrd && !qbfalse)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    if(!qbfalse)
      qbfalse.emplace();  // empty `else` block

    Statement::S_if xstmt = { *kneg, move(*qcond), move(*qbtrue), move(*qbfalse) };
    return move(xstmt);
  }

opt<Statement>
do_accept_switch_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // switch-statement ::=
    //   "switch" "(" expression ")" "{" switch-clause * "}"
    // switch-clause ::=
    //   ( "case" expression | "default" ) ":" statement *
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch });
    if(!qkwrd)
      return nullopt;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qctrl = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qctrl)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    // Parse the block by hand.
    cow_vector<Statement::switch_clause> clauses;

    op_sloc = tstrm.next_sloc();
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_brace_expected, tstrm.next_sloc());

    for(;;) {
      auto label_sloc = tstrm.next_sloc();
      qkwrd = do_accept_keyword_opt(tstrm, { keyword_case, keyword_default });
      if(!qkwrd)
        break;

      auto& clause = clauses.emplace_back();
      if(*qkwrd == keyword_case) {
        // The `case` label requires an expression argument.
        auto qlabel = do_accept_expression_as_rvalue_opt(tstrm);
        if(!qlabel)
          throw Compiler_Error(xtc_status,
                    compiler_status_expression_expected, tstrm.next_sloc());

        // Set the label.
        clause.label = move(*qlabel);
        ROCKET_ASSERT(!clause.label.units.empty());
      }
      else {
        // The `default` label takes no argument. There shall be no more than
        // one `default` label within each `switch` statement.
        for(size_t i = 0;  i < clauses.size() - 1;  ++i)
          if(clauses.at(i).label.units.empty())
            throw Compiler_Error(xtc_status,
                      compiler_status_multiple_default, label_sloc);
      }

      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
      if(!kpunct)
        throw Compiler_Error(xtc_status,
                  compiler_status_colon_expected, tstrm.next_sloc());

      while(auto qstmt = do_accept_statement_opt(tstrm, scope | scope_switch))
        clause.body.emplace_back(move(*qstmt));
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_brace_or_switch_clause_expected, tstrm.next_sloc(),
                "[unmatched `{` at '$1']", op_sloc);

    Statement::S_switch xstmt = { move(*qctrl), move(clauses) };
    return move(xstmt);
  }

opt<Statement>
do_accept_do_while_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // do-while-statement ::=
    //   "do" nondeclaration-statement "while" negation ? "(" expression ")" ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_do });
    if(!qkwrd)
      return nullopt;

    auto qblock = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope | scope_while);
    if(!qblock)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd)
      throw Compiler_Error(xtc_status,
                compiler_status_keyword_while_expected, tstrm.next_sloc());

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_do_while xstmt = { move(*qblock), *kneg, move(*qcond) };
    return move(xstmt);
  }

opt<Statement>
do_accept_while_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // while-statement ::=
    //   "while" negation ? "(" expression ")" nondeclaration-statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd)
      return nullopt;

    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg)
      kneg.emplace();

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcond = do_accept_expression_as_rvalue_opt(tstrm);
    if(!qcond)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope | scope_while);
    if(!qblock)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    Statement::S_while xstmt = { *kneg, move(*qcond), move(*qblock) };
    return move(xstmt);
  }

opt<Statement>
do_accept_for_complement_range_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                                   scope_flags scope)
  {
    // for-complement-range ::=
    //   "each" identifier ( ( "," | ":" | "=" ) identifier ) ? "->" expression ")"
    //   nondeclaration-statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_each });
    if(!qkwrd)
      return nullopt;

    cow_string key;
    auto qmapped = do_accept_identifier_opt(tstrm, true);
    if(!qmapped)
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_colon, punctuator_assign });
    if(kpunct) {
      // Move the first identifier into the key and expect the name for mapped
      // references.
      key = move(*qmapped);
      qmapped = do_accept_identifier_opt(tstrm, true);
      if(!qmapped)
        throw Compiler_Error(xtc_status,
                  compiler_status_identifier_expected, tstrm.next_sloc());
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_arrow });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_arrow_expected, tstrm.next_sloc());

    auto sloc_init = tstrm.next_sloc();
    auto qinit = do_accept_expression_opt(tstrm);
    if(!qinit)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope | scope_for);
    if(!qblock)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    Statement::S_for_each xstmt = { move(key), move(*qmapped), move(sloc_init), move(*qinit),
                                    move(*qblock) };
    return move(xstmt);
  }

opt<Statement>
do_accept_for_initializer_opt(Token_Stream& tstrm)
  {
    // for-initializer ::=
    //   null-statement | variable-definition | immutable-variable-definition |
    //   expression-statement
    if(auto qblock = do_accept_null_statement_opt(tstrm))
      return move(*qblock);

    if(auto qinit = do_accept_variable_definition_opt(tstrm))
      return move(*qinit);

    if(auto qinit = do_accept_immutable_variable_definition_opt(tstrm))
      return move(*qinit);

    if(auto qinit = do_accept_expression_statement_opt(tstrm))
      return move(*qinit);

    return nullopt;
  }

opt<Statement>
do_accept_for_complement_triplet_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                                     scope_flags scope)
  {
    // for-complement-triplet ::=
    //   for-initializer expression ? ";" expression ? ")" nondeclaration-statement
    auto qstmt = do_accept_for_initializer_opt(tstrm);
    if(!qstmt)
      return nullopt;

    cow_vector<Statement> init;
    init.emplace_back(move(*qstmt));

    Statement::S_expression cond;
    cond.sloc = tstrm.next_sloc();
    do_accept_expression_and_check(cond.units, tstrm, false);

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_expression step;
    step.sloc = tstrm.next_sloc();
    do_accept_expression(step.units, tstrm);

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qblock = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope | scope_for);
    if(!qblock)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    Statement::S_for xstmt = { move(init), move(cond), move(step), move(*qblock) };
    return move(xstmt);
  }

opt<Statement>
do_accept_for_complement_opt(Token_Stream& tstrm, const Source_Location& op_sloc,
                             scope_flags scope)
  {
    // for-complement ::=
    //   for-complement-range | for-complement-triplet
    if(auto qcompl = do_accept_for_complement_range_opt(tstrm, op_sloc, scope))
      return move(*qcompl);

    if(auto qcompl = do_accept_for_complement_triplet_opt(tstrm, op_sloc, scope))
      return move(*qcompl);

    return nullopt;
  }

opt<Statement>
do_accept_for_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // for-statement ::=
    //   "for" "(" for-complement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_for });
    if(!qkwrd)
      return nullopt;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto qcompl = do_accept_for_complement_opt(tstrm, op_sloc, scope);
    if(!qcompl)
      throw Compiler_Error(xtc_status,
                compiler_status_for_statement_initializer_expected, tstrm.next_sloc());

    return move(*qcompl);
  }

opt<Statement>
do_accept_break_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // break-statement ::=
    //   "break" break-target ? ";"
    // break-target ::=
    //   "switch" | "while" | "for"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_break });
    if(!qkwrd)
      return nullopt;

    scope_flags scope_check = scope_switch | scope_while | scope_for;
    Jump_Target target = jump_target_unspec;

    qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch, keyword_while, keyword_for });
    if(qkwrd && (*qkwrd == keyword_switch)) {
      // `switch`
      scope_check = scope_switch;
      target = jump_target_switch;
    }
    else if(qkwrd && (*qkwrd == keyword_while)) {
     // `do`...`while` and `while`
      scope_check = scope_while;
      target = jump_target_while;
    }
    else if(qkwrd && (*qkwrd == keyword_for)) {
     // `for` and `for`...`each`
      scope_check = scope_for;
      target = jump_target_for;
    }

    if(!(scope & scope_check))
      throw Compiler_Error(xtc_status,
                compiler_status_break_no_matching_scope, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_break xstmt = { move(sloc), target };
    return move(xstmt);
  }

opt<Statement>
do_accept_continue_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // continue-statement ::=
    //   "continue" continue-target ? ";"
    // continue-target ::=
    //   "while" | "for"
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_continue });
    if(!qkwrd)
      return nullopt;

    scope_flags scope_check = scope_while | scope_for;
    Jump_Target target = jump_target_unspec;

    qkwrd = do_accept_keyword_opt(tstrm, { keyword_while, keyword_for });
    if(qkwrd && (*qkwrd == keyword_while)) {
     // `do`...`while` and `while`
      scope_check = scope_while;
      target = jump_target_while;
    }
    else if(qkwrd && (*qkwrd == keyword_for)) {
     // `for` and `for`...`each`
      scope_check = scope_for;
      target = jump_target_for;
    }

    if(!(scope & scope_check))
      throw Compiler_Error(xtc_status,
                compiler_status_continue_no_matching_scope, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_continue xstmt = { move(sloc), target };
    return move(xstmt);
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_throw xstmt = { move(sloc), move(*kexpr) };
    return move(xstmt);
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                succ ? compiler_status_semicolon_expected : compiler_status_expression_expected,
                tstrm.next_sloc());

    xexpr.sloc = move(arg_sloc);
    Statement::S_return xstmt = { move(sloc), refsp.value_or(false), move(xexpr) };
    return move(xstmt);
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    opt<cow_string> kmsg;
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(kpunct)
      kmsg = do_accept_string_literal_opt(tstrm);

    if(kpunct && !kmsg)
      throw Compiler_Error(xtc_status,
                compiler_status_string_literal_expected, tstrm.next_sloc());

    if(!kmsg)
      kmsg.emplace(&"[no message]");

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_assert xstmt = { move(sloc), move(*kexpr), move(*kmsg) };
    return move(xstmt);
  }

opt<Statement>
do_accept_try_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // try-statement ::=
    //   "try" nondeclaration-statement "catch" "(" identifier ")"
    //   nondeclaration-statement
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_try });
    if(!qkwrd)
      return nullopt;

    auto qbtry = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope);
    if(!qbtry)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    // Note that this is the location of the `catch` block.
    auto sloc_catch = tstrm.next_sloc();
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_catch });
    if(!qkwrd)
      throw Compiler_Error(xtc_status,
                compiler_status_keyword_catch_expected, tstrm.next_sloc());

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    auto kexcept = do_accept_identifier_opt(tstrm, true);
    if(!kexcept)
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    auto qbcatch = do_accept_nondeclaration_statement_as_block_opt(tstrm, scope);
    if(!qbcatch)
      throw Compiler_Error(xtc_status,
                compiler_status_nondeclaration_statement_expected, tstrm.next_sloc());

    Statement::S_try xstmt = { move(sloc), move(*qbtry), move(sloc_catch), move(*kexcept),
                               move(*qbcatch) };
    return move(xstmt);
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_semicolon_expected, tstrm.next_sloc());

    Statement::S_defer xstmt = { move(sloc), move(*kexpr) };
    return move(xstmt);
  }

opt<Statement>
do_accept_statement_opt(Token_Stream& tstrm, scope_flags scope)
  {
    const auto sentry = tstrm.copy_recursion_sentry();

    // statement ::=
    //   variable-definition | immutable-variable-definition | reference-definition |
    //   function-definition | defer-statement | null-statement |
    //   nondeclaration-statement
    // nondeclaration-statement ::=
    //   if-statement | switch-statement | do-while-statement | while-statement |
    //   for-statement | break-statement | continue-statement | throw-statement |
    //   return-statement | assert-statement | try-statement | statement-block |
    //   expression-statement
    if(auto qstmt = do_accept_variable_definition_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_immutable_variable_definition_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_reference_definition_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_function_definition_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_defer_statement_opt(tstrm))
      return move(*qstmt);

    if(auto qblock = do_accept_null_statement_opt(tstrm))
      return move(*qblock);

    if(auto qstmt = do_accept_if_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_switch_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_do_while_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_while_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_for_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_break_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_continue_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qstmt = do_accept_throw_statement_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_return_statement_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_assert_statement_opt(tstrm))
      return move(*qstmt);

    if(auto qstmt = do_accept_try_statement_opt(tstrm, scope))
      return move(*qstmt);

    if(auto qblock = do_accept_statement_block_opt(tstrm, scope))
      return move(*qblock);

    if(auto qstmt = do_accept_expression_statement_opt(tstrm))
      return move(*qstmt);

    return nullopt;
  }

Statement::S_block
do_blockify_statement(Statement&& stmt)
  {
    // Make a block consisting of a single statement.
    cow_vector<Statement> stmts;
    stmts.emplace_back(move(stmt));
    Statement::S_block xblock = { move(stmts) };
    return xblock;
  }

opt<Statement::S_block>
do_accept_nondeclaration_statement_as_block_opt(Token_Stream& tstrm, scope_flags scope)
  {
    // nondeclaration-statement ::=
    //   if-statement | switch-statement | do-while-statement | while-statement |
    //   for-statement | break-statement | continue-statement | throw-statement |
    //   return-statement | assert-statement | try-statement | statement-block |
    //   expression-statement
    if(auto qstmt = do_accept_if_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_switch_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_do_while_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_while_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_for_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_break_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_continue_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_throw_statement_opt(tstrm))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_return_statement_opt(tstrm))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_assert_statement_opt(tstrm))
      return do_blockify_statement(move(*qstmt));

    if(auto qstmt = do_accept_try_statement_opt(tstrm, scope))
      return do_blockify_statement(move(*qstmt));

    if(auto qblock = do_accept_statement_block_opt(tstrm, scope))
      return move(*qblock);

    if(auto qstmt = do_accept_expression_statement_opt(tstrm))
      return do_blockify_statement(move(*qstmt));

    return nullopt;
  }

struct Prefix_Keyword_Xop
  {
    Keyword kwrd;
    Xop xop;
  }
constexpr s_prefix_keyword_xop[] =
  {
    { keyword_unset,    xop_unset    },
    { keyword_countof,  xop_countof  },
    { keyword_typeof,   xop_typeof   },
    { keyword_not,      xop_notl     },
    { keyword_abs,      xop_abs      },
    { keyword_sign,     xop_sign     },
    { keyword_sqrt,     xop_sqrt     },
    { keyword_isnan,    xop_isnan    },
    { keyword_isinf,    xop_isinf    },
    { keyword_isvoid,   xop_isvoid   },
    { keyword_round,    xop_round    },
    { keyword_floor,    xop_floor    },
    { keyword_ceil,     xop_ceil     },
    { keyword_trunc,    xop_trunc    },
    { keyword_iround,   xop_iround   },
    { keyword_ifloor,   xop_ifloor   },
    { keyword_iceil,    xop_iceil    },
    { keyword_itrunc,   xop_itrunc   },
    { keyword_lzcnt,    xop_lzcnt    },
    { keyword_tzcnt,    xop_tzcnt    },
    { keyword_popcnt,   xop_popcnt   },
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
    { punctuator_add,   xop_pos   },
    { punctuator_sub,   xop_neg   },
    { punctuator_notb,  xop_notb  },
    { punctuator_notl,  xop_notl  },
    { punctuator_inc,   xop_inc   },
    { punctuator_dec,   xop_dec   },
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
    //   "unset" | "countof" | "typeof" | "not" | "__abs" | "__sqrt" | "__sign" |
    //   "__isnan" | "__isinf" | "__round" | "__floor" | "__ceil" | "__trunc" |
    //   "__iround" | "__ifloor" | "__iceil" | "__itrunc" | "__lzcnt" | "__tzcnt" |
    //   "__popcnt" | "__isvoid"
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
      units.emplace_back(move(xunit));
      return true;
    }

    if(qtok->is_punctuator()) {
      auto qconf = ::rocket::find(s_prefix_punctuator_xop, qtok->as_punctuator());
      if(!qconf)
        return false;

      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, false };
      units.emplace_back(move(xunit));
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
      units.emplace_back(move(xunit));
      return true;
    }

    if(*qname == "__line") {
      Expression_Unit::S_literal xunit = { sloc.line() };
      units.emplace_back(move(xunit));
      return true;
    }

    Expression_Unit::S_local_reference xunit = { sloc, move(*qname) };
    units.emplace_back(move(xunit));
    return true;
  }

bool
do_accept_global_reference(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // extern-identifier ::=
    //   "extern" identifier
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_extern });
    if(!qkwrd)
      return false;

    auto qname = do_accept_identifier_opt(tstrm, false);
    if(!qname)
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    Expression_Unit::S_global_reference xunit = { sloc, move(*qname) };
    units.emplace_back(move(xunit));
    return true;
  }

bool
do_accept_literal(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get a literal as a `Value`.
    auto qval = do_accept_literal_opt(tstrm);
    if(!qval)
      return false;

    Expression_Unit::S_literal xunit = { move(*qval) };
    units.emplace_back(move(xunit));
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

    Expression_Unit::S_local_reference xunit = { sloc, &"__this" };
    units.emplace_back(move(xunit));
    return true;
  }

bool
do_accept_closure_function(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // closure-function ::=
    //   "func" "(" parameter-list ? ")" closure-body
    // closure-body ::=
    //   statement-block | equal-initializer | ref-initializer
    auto sloc = tstrm.next_sloc();
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd)
      return false;

    auto op_sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    cow_vector<phsh_string> params;
    bool comma_allowed = false;

    for(;;) {
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
      if(kpunct) {
        params.emplace_back(&"...");
        break;
      }

      auto param_sloc = tstrm.next_sloc();
      auto kparam = do_accept_identifier_opt(tstrm, true);
      if(!kparam)
        break;

      if(::rocket::find(params, *kparam))
        throw Compiler_Error(xtc_status,
                  compiler_status_duplicate_name_in_parameter_list, param_sloc);

      params.emplace_back(move(*kparam));

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_parenthesis_or_comma_expected
                              : compiler_status_closing_parenthesis_or_parameter_expected,
                tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    op_sloc = tstrm.next_sloc();
    auto qblock = do_accept_statement_block_opt(tstrm, scope_plain);

    if(!qblock) {
      // Try `equal-initializer`.
      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(qinit) {
        // Behave as if it was a `return-statement` by value.
        Statement::S_return xstmt = { move(op_sloc), false, move(*qinit) };
        qblock = do_blockify_statement(move(xstmt));
      }
    }

    if(!qblock) {
      // Try `ref-initializer`.
      auto qinit = do_accept_ref_initializer_opt(tstrm);
      if(qinit) {
        // Behave as if it was a `return-statement` by reference.
        Statement::S_return xstmt = { move(op_sloc), true, move(*qinit) };
        qblock = do_blockify_statement(move(xstmt));
      }
    }

    if(!qblock)
      throw Compiler_Error(xtc_status,
                compiler_status_open_brace_or_initializer_expected, tstrm.next_sloc());

    auto unique_name = format_string("__closure:$1:$2", sloc.line(), sloc.column());

    Expression_Unit::S_closure_function xunit = { move(sloc), move(unique_name), move(params),
                                                  move(qblock->stmts) };
    units.emplace_back(move(xunit));
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
      if(!do_accept_expression_and_check(units, tstrm, false))
        break;

      nelems += 1;
      if(nelems >= 0x100000)
        throw Compiler_Error(xtc_status,
                  compiler_status_too_many_elements, tstrm.next_sloc());

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_bracket_or_comma_expected
                              : compiler_status_closing_bracket_or_expression_expected,
                tstrm.next_sloc(),
                "[unmatched `[` at '$1']", sloc);

    Expression_Unit::S_unnamed_array xunit = { sloc, nelems };
    units.emplace_back(move(xunit));
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
        throw Compiler_Error(xtc_status,
                  compiler_status_duplicate_key_in_object, op_sloc);

      // Look for the value with an initiator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign, punctuator_colon });
      if(!kpunct)
        throw Compiler_Error(xtc_status,
                  compiler_status_equals_sign_or_colon_expected, tstrm.next_sloc());

      if(!do_accept_expression_and_check(units, tstrm, false))
        throw Compiler_Error(xtc_status,
                  compiler_status_expression_expected, tstrm.next_sloc());

      // Look for the separator.
      keys.emplace_back(move(*qkey));
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_brace_or_comma_expected
                              : compiler_status_closing_brace_or_json5_key_expected,
                tstrm.next_sloc(),
                "[unmatched `{` at '$1']", sloc);

    Expression_Unit::S_unnamed_object xunit = { sloc, move(keys) };
    units.emplace_back(move(xunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
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
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_operator_rpn xunit = { sloc, xop_fma, false };
    units.emplace_back(move(xunit));
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
      return move(xunit);
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
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    units.emplace_back(move(*qxunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    Expression_Unit::S_catch xunit;
    if(!do_accept_expression(xunit.operand, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    units.emplace_back(move(xunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    // The first argument is the target function. The second argument is the
    // argument generator.
    cow_vector<Expression_Unit::argument> args;
    args.append(2);

    if(!do_accept_expression(args.mut(0).units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct)
      throw Compiler_Error(xtc_status,
                compiler_status_comma_expected, tstrm.next_sloc());

    if(!do_accept_expression(args.mut(1).units, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_parenthesis_expected, tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_variadic_call xunit = { move(sloc), move(args) };
    units.emplace_back(move(xunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_open_parenthesis_expected, tstrm.next_sloc());

    cow_vector<Expression_Unit::argument> args;
    bool comma_allowed = false;

    for(;;) {
      auto arg_sloc = tstrm.next_sloc();
      auto ref_sp = do_accept_reference_specifier_opt(tstrm);
      Expression_Unit::argument arg;
      bool succ = do_accept_expression_and_check(arg.units, tstrm, ref_sp.value_or(false));
      if(!ref_sp && !succ)
        break;
      else if(!succ)
        throw Compiler_Error(xtc_status,
                  compiler_status_expression_expected, tstrm.next_sloc());

      args.emplace_back(move(arg));

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    if(args.size() < 1)
      throw Compiler_Error(xtc_status,
                compiler_status_argument_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_parenthesis_or_comma_expected
                              : compiler_status_closing_parenthesis_or_argument_expected,
                tstrm.next_sloc(),
                "[unmatched `(` at '$1']", op_sloc);

    Expression_Unit::S_import_call xunit = { move(sloc), move(args) };
    units.emplace_back(move(xunit));
    return true;
  }

bool
do_accept_primary_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // primary-expression ::=
    //   identifier | extern-identifier | literal | "this" | closure-function |
    //   unnamed-array | unnamed-object | nested-expression | fused-multiply-add |
    //   prefix-binary-expression | catch-expression | variadic-function-call |
    //   import-function-call
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
    { punctuator_inc,     xop_inc     },
    { punctuator_dec,     xop_dec     },
    { punctuator_head,    xop_head    },
    { punctuator_tail,    xop_tail    },
    { punctuator_random,  xop_random  },
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
    //   "++" | "--" | "[^]" | "[$]" | "[?]" | postfix-function-call |
    //   postfix-subscript | postfix-member-access | postfix-operator
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
      Expression_Unit::S_operator_rpn xunit = { sloc, qconf->xop, true };
      units.emplace_back(move(xunit));
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

    cow_vector<Expression_Unit::argument> args;
    bool comma_allowed = false;

    for(;;) {
      auto arg_sloc = tstrm.next_sloc();
      auto ref_sp = do_accept_reference_specifier_opt(tstrm);
      Expression_Unit::argument arg;
      bool succ = do_accept_expression_and_check(arg.units, tstrm, ref_sp.value_or(false));
      if(!ref_sp && !succ)
        break;
      else if(!succ)
        throw Compiler_Error(xtc_status,
                  compiler_status_expression_expected, tstrm.next_sloc());

      args.emplace_back(move(arg));

      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      comma_allowed = !kpunct;
      if(!kpunct)
        break;
    }

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                comma_allowed ? compiler_status_closing_parenthesis_or_comma_expected
                              : compiler_status_closing_parenthesis_or_argument_expected,
                tstrm.next_sloc(),
                "[unmatched `(` at '$1']", sloc);

    Expression_Unit::S_function_call xunit = { move(sloc), move(args) };
    units.emplace_back(move(xunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_closing_bracket_expected, tstrm.next_sloc(),
                "[unmatched `[` at '$1']", sloc);

    Expression_Unit::S_operator_rpn xunit = { sloc, xop_index, false };
    units.emplace_back(move(xunit));
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
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    Expression_Unit::S_literal xname = { move(*qkey) };
    units.emplace_back(move(xname));

    Expression_Unit::S_operator_rpn xunit = { sloc, xop_index, false };
    units.emplace_back(move(xunit));
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

      throw Compiler_Error(xtc_status,
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

    Infix_Element::S_head xelem = { move(units) };
    return move(xelem);
  }

opt<Infix_Element>
do_accept_infix_ternary_opt(Token_Stream& tstrm)
  {
    // infix-ternary ::=
    //   ( "?" | "?=" ) expression ":"
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_quest_eq });
    if(!kpunct)
      return nullopt;

    bool assign = *kpunct == punctuator_quest_eq;
    cow_vector<Expression_Unit> btrue;
    if(!do_accept_expression(btrue, tstrm))
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct)
      throw Compiler_Error(xtc_status_format,
                compiler_status_colon_expected, tstrm.next_sloc(),
                "[unmatched `$1` at '$2']", !assign ? "?" : "?=", sloc);

    Infix_Element::S_ternary xelem = { move(sloc), assign, move(btrue), { } };
    return move(xelem);
  }

opt<Infix_Element>
do_accept_infix_logical_and_opt(Token_Stream& tstrm)
  {
    // infix-logical-and ::=
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
    return move(xelem);
  }

opt<Infix_Element>
do_accept_infix_logical_or_opt(Token_Stream& tstrm)
  {
    // infix-logical-or ::=
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
    return move(xelem);
  }

opt<Infix_Element>
do_accept_infix_coalescence_opt(Token_Stream& tstrm)
  {
    // infix-coalescence ::=
    //   "??" | "??="
    auto sloc = tstrm.next_sloc();
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_coales, punctuator_coales_eq });
    if(!kpunct)
      return nullopt;

    bool assign = *kpunct == punctuator_coales_eq;
    Infix_Element::S_coalescence xelem = { sloc, assign, { } };
    return move(xelem);
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
    { punctuator_cmp_3way,   xop_cmp_3way,  0 },
    { punctuator_cmp_un,     xop_cmp_un,    0 },
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
    //   "+" | "-" | "*" | "/" | "%" | "<<" | ">>" | "<<<" | ">>>" | "&" | "|" |
    //   "^" | "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" |
    //   "&=" | "|=" | "^=" | "=" | "==" | "!=" | "<" | ">" | "<=" | ">=" | "<=>" |
    //   "</>"
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
      return move(xelem);
    }
    return nullopt;
  }

opt<Infix_Element>
do_accept_infix_operator_opt(Token_Stream& tstrm)
  {
    // infix-operator ::=
    //   infix-ternary | infix-logical-and | infix-logical-or | infix-coalescence |
    //   infix-operator-general
    if(auto qelem = do_accept_infix_ternary_opt(tstrm))
      return move(*qelem);

    if(auto qelem = do_accept_infix_logical_and_opt(tstrm))
      return move(*qelem);

    if(auto qelem = do_accept_infix_logical_or_opt(tstrm))
      return move(*qelem);

    if(auto qelem = do_accept_infix_coalescence_opt(tstrm))
      return move(*qelem);

    if(auto qelem = do_accept_infix_operator_general_opt(tstrm))
      return move(*qelem);

    return nullopt;
  }

bool
do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    const auto sentry = tstrm.copy_recursion_sentry();

    // expression ::=
    //   infix-element infix-carriage *
    // infix-carriage ::=
    //   infix-operator infix-element
    auto qelem = do_accept_infix_element_opt(tstrm);
    if(!qelem)
      return false;

    cow_vector<Infix_Element> stack;
    stack.emplace_back(move(*qelem));

    for(;;) {
      auto qnext = do_accept_infix_operator_opt(tstrm);
      if(!qnext)
        break;

      if(!do_accept_infix_element(qnext->mut_junction(), tstrm))
        throw Compiler_Error(xtc_status,
                  compiler_status_expression_expected, tstrm.next_sloc());

      // Assignment operators have the lowest precedence and group from right to
      // left, so they are a special case.
      auto next_precedence = qnext->tell_precedence();
      if(next_precedence == precedence_assignment)
        next_precedence = static_cast<Precedence>(precedence_assignment - 1);

      while((stack.size() >= 2) && (stack.back().tell_precedence() <= next_precedence)) {
        // Collapse elements that have no lower precedence.
        qelem = move(stack.mut_back());
        stack.pop_back();
        qelem->extract(stack.mut_back().mut_junction());
      }
      stack.emplace_back(move(*qnext));
    }

    while(stack.size() >= 2) {
      // Collapse all elements so we eventually have a single node in the stack.
      qelem = move(stack.mut_back());
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

void
Statement_Sequence::
clear() noexcept
  {
    this->m_stmts.clear();
  }

void
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
      stmts.emplace_back(move(*qstmt));

    // If there are any non-statement tokens left in the stream, fail.
    if(!tstrm.empty())
      throw Compiler_Error(xtc_status,
                compiler_status_statement_expected, tstrm.next_sloc());

    // Add an implicit return.
    Statement::S_return xendf = { tstrm.next_sloc(), true, Statement::S_expression() };
    stmts.emplace_back(move(xendf));

    // Succeed.
    this->m_stmts = move(stmts);
  }

void
Statement_Sequence::
reload_oneline(Token_Stream&& tstrm)
  {
    // Destroy the contents of `*this` and reuse their storage, if any.
    cow_vector<Statement> stmts;
    this->m_stmts.clear();
    stmts.swap(this->m_stmts);

    // Parse an expression. This is required.
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr)
      throw Compiler_Error(xtc_status,
                compiler_status_expression_expected, tstrm.next_sloc());

    // If there are any non-statement tokens left in the stream, fail.
    if(!tstrm.empty())
      throw Compiler_Error(xtc_status,
                compiler_status_invalid_expression, tstrm.next_sloc());

    // Build a return-statement that forwards the result by reference.
    Statement::S_return xstmt = { kexpr->sloc, true, move(*kexpr) };
    stmts.emplace_back(move(xstmt));

    // Succeed.
    this->m_stmts = move(stmts);
  }

}  // namespace asteria
