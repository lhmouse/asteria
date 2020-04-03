// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement_sequence.hpp"
#include "enums.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "expression_unit.hpp"
#include "statement.hpp"
#include "infix_element.hpp"
#include "../runtime/enums.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

constexpr uint64_t do_cantor_pair(uint64_t x, uint64_t y) noexcept
  {
    return (x + y) * (x + y + 1) / 2 + y;
  }

inline uint64_t do_get_unique_id(const Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return 0;
    else
      return do_cantor_pair(static_cast<unsigned long>(qtok->line()), qtok->offset());
  }

[[noreturn]] inline void do_throw_parser_error(const Token_Stream& tstrm, Parser_Status status)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Parser_Error(status, -1, 0, 0);
    else
      throw Parser_Error(status, qtok->line(), qtok->offset(), qtok->length());
  }

inline Source_Location do_tell_source_location(const Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return Source_Location(::rocket::sref("<end of stream>"), -1);
    else
      return Source_Location(qtok->file(), qtok->line());
  }

opt<Keyword> do_accept_keyword_opt(Token_Stream& tstrm, initializer_list<Keyword> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    // See whether it is one of the acceptable keywords.
    if(!qtok->is_keyword()) {
      return nullopt;
    }
    auto kwrd = qtok->as_keyword();
    if(::rocket::is_none_of(kwrd, accept)) {
      return nullopt;
    }
    // Return the keyword and discard this token.
    tstrm.shift();
    return kwrd;
  }

opt<Punctuator> do_accept_punctuator_opt(Token_Stream& tstrm, initializer_list<Punctuator> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    // See whether it is one of the acceptable punctuators.
    if(!qtok->is_punctuator()) {
      return nullopt;
    }
    auto punct = qtok->as_punctuator();
    if(::rocket::is_none_of(punct, accept)) {
      return nullopt;
    }
    // Return the punctuator and discard this token.
    tstrm.shift();
    return punct;
  }

opt<cow_string> do_accept_identifier_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    // See whether it is an identifier.
    if(!qtok->is_identifier()) {
      return nullopt;
    }
    auto name = qtok->as_identifier();
    // Return the identifier and discard this token.
    tstrm.shift();
    return name;
  }

cow_string& do_concatenate_string_literal_sequence(cow_string& val, Token_Stream& tstrm)
  {
    for(;;) {
      auto qtok = tstrm.peek_opt();
      if(!qtok) {
        break;
      }
      // See whether it is a string literal.
      if(!qtok->is_string_literal()) {
        break;
      }
      val += qtok->as_string_literal();
      // Append the string literal and discard this token.
      tstrm.shift();
    }
    return val;
  }

opt<cow_string> do_accept_string_literal_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    // See whether it is a string literal.
    if(!qtok->is_string_literal()) {
      return nullopt;
    }
    auto val = qtok->as_string_literal();
    // Return the string literal and discard this token.
    tstrm.shift();
    do_concatenate_string_literal_sequence(val, tstrm);
    return val;
  }

opt<cow_string> do_accept_json5_key_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    // See whether it is a keyword, identifier, or string literal.
    if(qtok->is_keyword()) {
      auto kwrd = qtok->as_keyword();
      // Treat the keyword as a plain identifier and discard this token.
      tstrm.shift();
      return ::rocket::sref(stringify_keyword(kwrd));
    }
    if(qtok->is_identifier()) {
      auto name = qtok->as_identifier();
      // Return the identifier and discard this token.
      tstrm.shift();
      return name;
    }
    if(qtok->is_string_literal()) {
      auto val = qtok->as_string_literal();
      // Return the string literal and discard this token.
      tstrm.shift();
      do_concatenate_string_literal_sequence(val, tstrm);
      return val;
    }
    return nullopt;
  }

Value do_generate_null()
  {
    return nullptr;
  }
Value do_generate_false()
  {
    return false;
  }
Value do_generate_true()
  {
    return true;
  }
Value do_generate_nan()
  {
    return ::std::numeric_limits<double>::quiet_NaN();
  }
Value do_generate_infinity()
  {
    return ::std::numeric_limits<double>::infinity();
  }

using Value_Generator = Value ();

struct Literal_Element
  {
    Keyword kwrd;
    Value_Generator& vgen;
  }
constexpr s_literal_table[] =
  {
    { keyword_null,      do_generate_null      },
    { keyword_false,     do_generate_false     },
    { keyword_true,      do_generate_true      },
    { keyword_nan,       do_generate_nan       },
    { keyword_infinity,  do_generate_infinity  },
  };

constexpr bool operator==(const Literal_Element& lhs, Keyword rhs) noexcept
  {
    return lhs.kwrd == rhs;
  }

opt<Value> do_accept_literal_opt(Token_Stream& tstrm)
  {
    // literal ::=
    //   keyword-literal | string-literal | numeric-literal
    // keyword-literal ::=
    //   "null" | "false" | "true" | "nan" | "infinity"
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    if(qtok->is_keyword()) {
      auto qcnf = ::std::find(begin(s_literal_table), end(s_literal_table), qtok->as_keyword());
      if(qcnf == end(s_literal_table)) {
        return nullopt;
      }
      // Discard this token and create a new value using the generator.
      tstrm.shift();
      return qcnf->vgen();
    }
    if(qtok->is_integer_literal()) {
      auto val = qtok->as_integer_literal();
      // Copy the value and discard this token.
      tstrm.shift();
      return val;
    }
    if(qtok->is_real_literal()) {
      auto val = qtok->as_real_literal();
      // Copy the value and discard this token.
      tstrm.shift();
      return val;
    }
    if(qtok->is_string_literal()) {
      auto val = qtok->as_string_literal();
      // Copy the value and discard this token.
      tstrm.shift();
      do_concatenate_string_literal_sequence(val, tstrm);
      return val;
    }
    return nullopt;
  }

opt<bool> do_accept_negation_opt(Token_Stream& tstrm)
  {
    // negation ::=
    //   "!" | "not"
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_notl });
    if(kpunct) {
      return true;
    }
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_not });
    if(qkwrd) {
      return true;
    }
    return nullopt;
  }

opt<cow_vector<phsh_string>> do_accept_identifier_list_opt(Token_Stream& tstrm)
  {
    // identifier-list-opt ::=
    //   identifier-list | ""
    // identifier-list ::=
    //   identifier identifier-list-opt
    auto qname = do_accept_identifier_opt(tstrm);
    if(!qname) {
      return nullopt;
    }
    cow_vector<phsh_string> names;
    for(;;) {
      names.emplace_back(::std::move(*qname));
      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
      qname = do_accept_identifier_opt(tstrm);
      if(!qname) {
        do_throw_parser_error(tstrm, parser_status_identifier_expected);
      }
    }
    return ::std::move(names);
  }

opt<cow_vector<phsh_string>> do_accept_variable_declarator_opt(Token_Stream& tstrm)
  {
    // variable-declarator ::=
    //   identifier | structured-binding-array | structured-binding-object
    // structured-binding-array ::=
    //   "[" identifier-list "]"
    // structured-binding-object ::=
    //   "{" identifier-list "}"
    auto qname = do_accept_identifier_opt(tstrm);
    if(qname) {
      // Accept a single identifier.
      cow_vector<phsh_string> names;
      names.emplace_back(::std::move(*qname));
      return names;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of brackets and separated by commas.
      // There must be at least one identifier.
      auto qnames = do_accept_identifier_list_opt(tstrm);
      if(!qnames) {
        do_throw_parser_error(tstrm, parser_status_identifier_expected);
      }
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
      if(!kpunct) {
        do_throw_parser_error(tstrm, parser_status_closed_bracket_expected);
      }
      // Make the list different from a plain, sole one.
      qnames->insert(0, ::rocket::sref("["));
      qnames->emplace_back(::rocket::sref("]"));
      return qnames;
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(kpunct) {
      // Accept a list of identifiers wrapped in a pair of braces and separated by commas.
      // There must be at least one identifier.
      auto qnames = do_accept_identifier_list_opt(tstrm);
      if(!qnames) {
        do_throw_parser_error(tstrm, parser_status_identifier_expected);
      }
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
      if(!kpunct) {
        do_throw_parser_error(tstrm, parser_status_closed_brace_expected);
      }
      // Make the list different from a plain, sole one.
      qnames->insert(0, ::rocket::sref("{"));
      qnames->emplace_back(::rocket::sref("}"));
      return qnames;
    }
    return nullopt;
  }

// Accept a statement; a blockt is converted to a single statement.
opt<Statement> do_accept_statement_opt(Token_Stream& tstrm);
// Accept a statement; a non-block statement is converted to a block consisting of a single statement.
opt<Statement::S_block> do_accept_statement_as_block_opt(Token_Stream& tstrm);

bool do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm);

opt<Statement::S_expression> do_accept_expression_opt(Token_Stream& tstrm)
  {
    // expression-opt ::=
    //   expression | ""
    auto sloc = do_tell_source_location(tstrm);
    cow_vector<Expression_Unit> units;
    bool succ = do_accept_expression(units, tstrm);
    if(!succ) {
      return nullopt;
    }
    Statement::S_expression xexpr = { ::std::move(sloc), ::std::move(units) };
    return ::std::move(xexpr);
  }

opt<Statement::S_block> do_accept_block_opt(Token_Stream& tstrm)
  {
    // block ::=
    //   "{" statement-list-opt "}"
    // statement-list-opt ::=
    //   statement-list | ""
    // statement-list ::=
    //   statement statement-list-opt
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct) {
      return nullopt;
    }
    cow_vector<Statement> body;
    for(;;) {
      auto qstmt = do_accept_statement_opt(tstrm);
      if(!qstmt) {
        break;
      }
      body.emplace_back(::std::move(*qstmt));
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_brace_or_statement_expected);
    }
    Statement::S_block xstmt = { ::std::move(sloc), ::std::move(body) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_block_statement_opt(Token_Stream& tstrm)
  {
    auto qblock = do_accept_block_opt(tstrm);
    if(!qblock) {
      return nullopt;
    }
    return ::std::move(*qblock);
  }

opt<Statement::S_expression> do_accept_equal_initializer_opt(Token_Stream& tstrm)
  {
    // equal-initializer-opt ::=
    //   equal-initializer | ""
    // equal-initializer ::=
    //   "=" expression
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign });
    if(!kpunct) {
      return nullopt;
    }
    return do_accept_expression_opt(tstrm);
  }

opt<Statement> do_accept_null_statement_opt(Token_Stream& tstrm)
  {
    // null-statement ::=
    //   ";"
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      return nullopt;
    }
    Statement::S_expression xstmt = { ::std::move(sloc), nullopt };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_variable_definition_opt(Token_Stream& tstrm)
  {
    // variable-definition ::=
    //   "var" variable-declarator equal-initailizer-opt ( "," variable-declarator equal-initializer-opt | "" ) ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_var });
    if(!qkwrd) {
      return nullopt;
    }
    // Each declaractor has its own source location.
    cow_vector<Source_Location> slocs;
    cow_vector<cow_vector<phsh_string>> decls;
    cow_vector<Statement::S_expression> inits;
    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured binding.
      auto sloc = do_tell_source_location(tstrm);
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl) {
        do_throw_parser_error(tstrm, parser_status_identifier_expected);
      }
      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit) {
        qinit.emplace();
      }
      slocs.emplace_back(::std::move(sloc));
      decls.emplace_back(::std::move(*qdecl));
      inits.emplace_back(::std::move(*qinit));
      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_variables xstmt = { false, ::std::move(slocs), ::std::move(decls), ::std::move(inits) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_immutable_variable_definition_opt(Token_Stream& tstrm)
  {
    // immutable-variable-definition ::=
    //   "const" variable-declarator equal-initailizer ( "," identifier equal-initializer | "" ) ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_const });
    if(!qkwrd) {
      return nullopt;
    }
    // Each declaractor has its own source location.
    cow_vector<Source_Location> slocs;
    cow_vector<cow_vector<phsh_string>> decls;
    cow_vector<Statement::S_expression> inits;
    for(;;) {
      // Accept a declarator, which may denote a single variable or a structured binding.
      auto sloc = do_tell_source_location(tstrm);
      auto qdecl = do_accept_variable_declarator_opt(tstrm);
      if(!qdecl) {
        do_throw_parser_error(tstrm, parser_status_identifier_expected);
      }
      auto qinit = do_accept_equal_initializer_opt(tstrm);
      if(!qinit) {
        do_throw_parser_error(tstrm, parser_status_equals_sign_expected);
      }
      slocs.emplace_back(::std::move(sloc));
      decls.emplace_back(::std::move(*qdecl));
      inits.emplace_back(::std::move(*qinit));
      // Look for the separator.
      auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_variables xstmt = { true, ::std::move(slocs), ::std::move(decls), ::std::move(inits) };
    return ::std::move(xstmt);
  }

opt<cow_vector<phsh_string>> do_accept_parameter_list_opt(Token_Stream& tstrm)
  {
    // parameter-list-opt ::=
    //   parameter-list | ""
    // parameter-list ::=
    //   "..." | identifier ( "," parameter-list | "" )
    cow_vector<phsh_string> params;
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
    if(kpunct) {
      params.emplace_back(::rocket::sref("..."));
      return ::std::move(params);
    }
    auto qname = do_accept_identifier_opt(tstrm);
    if(!qname) {
      return nullopt;
    }
    for(;;) {
      params.emplace_back(::std::move(*qname));
      // The code above looks so ugly...
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
      if(kpunct) {
        params.emplace_back(::rocket::sref("..."));
        return ::std::move(params);
      }
      qname = do_accept_identifier_opt(tstrm);
      if(!qname) {
        do_throw_parser_error(tstrm, parser_status_parameter_or_ellipsis_expected);
      }
    }
    return ::std::move(params);
  }

opt<Statement> do_accept_function_definition_opt(Token_Stream& tstrm)
  {
    // function-definition ::=
    //   "func" identifier "(" parameter-list-opt ")" block
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd) {
      return nullopt;
    }
    auto qname = do_accept_identifier_opt(tstrm);
    if(!qname) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto kparams = do_accept_parameter_list_opt(tstrm);
    if(!kparams) {
      kparams.emplace();
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qbody = do_accept_block_opt(tstrm);
    if(!qbody) {
      do_throw_parser_error(tstrm, parser_status_open_brace_expected);
    }
    Statement::S_function xstmt = { ::std::move(sloc), ::std::move(*qname), ::std::move(*kparams),
                                    ::std::move(qbody->stmts) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_expression_statement_opt(Token_Stream& tstrm)
  {
    // expression-statement ::=
    //   expression ";"
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr) {
      return nullopt;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_expression xstmt = { ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<Statement::S_block> do_accept_else_branch_opt(Token_Stream& tstrm)
  {
    // else-branch-opt ::=
    //   "else" statement | ""
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_else });
    if(!qkwrd) {
      return nullopt;
    }
    auto qblock = do_accept_statement_as_block_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    return ::std::move(*qblock);
  }

opt<Statement> do_accept_if_statement_opt(Token_Stream& tstrm)
  {
    // if-statement ::=
    //   "if" negation-opt "(" expression ")" statement else-branch-opt
    // negation-opt ::=
    //   "!" | "not" | ""
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_if });
    if(!qkwrd) {
      return nullopt;
    }
    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg) {
      kneg.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto qcond = do_accept_expression_opt(tstrm);
    if(!qcond) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qbtrue = do_accept_statement_as_block_opt(tstrm);
    if(!qbtrue) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    auto qbfalse = do_accept_else_branch_opt(tstrm);
    if(!qbfalse) {
      qbfalse.emplace();
    }
    Statement::S_if xstmt = { *kneg, ::std::move(*qcond), ::std::move(*qbtrue), ::std::move(*qbfalse) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_switch_statement_opt(Token_Stream& tstrm)
  {
    // switch-statement ::=
    //   "switch" "(" expression ")" switch-block
    // switch-block ::=
    //   "{" swtich-clause-list-opt "}"
    // switch-clause-list-opt ::=
    //   switch-clause-list | ""
    // switch-clause-list ::=
    //   switch-clause switch-clause-list-opt
    // switch-clause ::=
    //   ( "case" expression | "default" ) ":" statement-list-opt
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch });
    if(!qkwrd) {
      return nullopt;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto qctrl = do_accept_expression_opt(tstrm);
    if(!qctrl) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    // Parse the block by hand.
    cow_vector<Statement::S_expression> labels;
    cow_vector<Statement::S_block> bodies;
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_brace_expected);
    }
    for(;;) {
      qkwrd = do_accept_keyword_opt(tstrm, { keyword_case, keyword_default });
      if(!qkwrd) {
        break;
      }
      Statement::S_expression label;
      if(*qkwrd == keyword_case) {
        // The `case` label requires an expression argument.
        auto qlabel = do_accept_expression_opt(tstrm);
        if(!qlabel) {
          do_throw_parser_error(tstrm, parser_status_expression_expected);
        }
        label = ::std::move(*qlabel);
      }
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
      if(!kpunct) {
        do_throw_parser_error(tstrm, parser_status_colon_expected);
      }
      auto qblock = do_accept_statement_as_block_opt(tstrm);
      if(!qblock) {
        qblock.emplace();
      }
      labels.emplace_back(::std::move(label));
      bodies.emplace_back(::std::move(*qblock));
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_brace_or_switch_clause_expected);
    }
    Statement::S_switch xstmt = { ::std::move(*qctrl), ::std::move(labels), ::std::move(bodies) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_do_while_statement_opt(Token_Stream& tstrm)
  {
    // do-while-statement ::=
    //   "do" statement "while" negation-opt "(" expression ")" ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_do });
    if(!qkwrd) {
      return nullopt;
    }
    auto qblock = do_accept_statement_as_block_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd) {
      return nullopt;
    }
    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg) {
      kneg.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto qcond = do_accept_expression_opt(tstrm);
    if(!qcond) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_do_while xstmt = { ::std::move(*qblock), *kneg, ::std::move(*qcond) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_while_statement_opt(Token_Stream& tstrm)
  {
    // while-statement ::=
    //   "while" negation-opt "(" expression ")" statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
    if(!qkwrd) {
      return nullopt;
    }
    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg) {
      kneg.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto qcond = do_accept_expression_opt(tstrm);
    if(!qcond) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qblock = do_accept_statement_as_block_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    Statement::S_while xstmt = { *kneg, ::std::move(*qcond), ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_for_complement_range_opt(Token_Stream& tstrm)
  {
    // for-complement-range ::=
    //   "each" identifier "," identifier ":" expression ")" statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_each });
    if(!qkwrd) {
      return nullopt;
    }
    auto qkname = do_accept_identifier_opt(tstrm);
    if(!qkname) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_comma_expected);
    }
    auto qmname = do_accept_identifier_opt(tstrm);
    if(!qmname) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_colon_expected);
    }
    auto qinit = do_accept_expression_opt(tstrm);
    if(!qinit) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qblock = do_accept_statement_as_block_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    Statement::S_for_each xstmt = { ::std::move(*qkname), ::std::move(*qmname), ::std::move(*qinit),
                                    ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

Statement::S_block do_blockify_statement(Source_Location&& sloc, Statement&& stmt)
  {
    cow_vector<Statement> stmts;
    stmts.emplace_back(::std::move(stmt));
    // Make a block consisting of a single statement.
    Statement::S_block xblock = { ::std::move(sloc), ::std::move(stmts) };
    return xblock;
  }

opt<Statement::S_block> do_accept_for_initializer_opt(Token_Stream& tstrm)
  {
    // for-initializer ::=
    //   null-statement | variable-definition | immutable-variable-definition | expression-statement
    auto sloc = do_tell_source_location(tstrm);
    auto qinit = do_accept_null_statement_opt(tstrm);
    if(qinit) {
      return do_blockify_statement(::std::move(sloc), ::std::move(*qinit));
    }
    qinit = do_accept_variable_definition_opt(tstrm);
    if(qinit) {
      return do_blockify_statement(::std::move(sloc), ::std::move(*qinit));
    }
    qinit = do_accept_immutable_variable_definition_opt(tstrm);
    if(qinit) {
      return do_blockify_statement(::std::move(sloc), ::std::move(*qinit));
    }
    return nullopt;
  }

opt<Statement> do_accept_for_complement_triplet_opt(Token_Stream& tstrm)
  {
    // for-complement-triplet ::=
    //   for-initializer expression-opt ";" expression-opt ")" statement
    auto qinit = do_accept_for_initializer_opt(tstrm);
    if(!qinit) {
      return nullopt;
    }
    auto qcond = do_accept_expression_opt(tstrm);
    if(!qcond) {
      qcond.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    auto kstep = do_accept_expression_opt(tstrm);
    if(!kstep) {
      kstep.emplace();
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qblock = do_accept_statement_as_block_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    Statement::S_for xstmt = { ::std::move(*qinit), ::std::move(*qcond), ::std::move(*kstep),
                               ::std::move(*qblock) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_for_complement_opt(Token_Stream& tstrm)
  {
    // for-complement ::=
    //   for-complement-range | for-complement-triplet
    if(auto qcompl = do_accept_for_complement_range_opt(tstrm)) {
      return qcompl;
    }
    if(auto qcompl = do_accept_for_complement_triplet_opt(tstrm)) {
      return qcompl;
    }
    return nullopt;
  }

opt<Statement> do_accept_for_statement_opt(Token_Stream& tstrm)
  {
    // for-statement ::=
    //   "for" "(" for-complement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_for });
    if(!qkwrd) {
      return nullopt;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto qcompl = do_accept_for_complement_opt(tstrm);
    if(!qcompl) {
      do_throw_parser_error(tstrm, parser_status_for_statement_initializer_expected);
    }
    return ::std::move(*qcompl);
  }

opt<Jump_Target> do_accept_break_target_opt(Token_Stream& tstrm)
  {
    // break-target-opt ::=
    //   "switch" | "while" | "for" | ""
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch, keyword_while, keyword_for });
    if(qkwrd == keyword_switch) {
      return jump_target_switch;
    }
    if(qkwrd == keyword_while) {
      return jump_target_while;
    }
    if(qkwrd == keyword_for) {
      return jump_target_for;
    }
    return nullopt;
  }

opt<Statement> do_accept_break_statement_opt(Token_Stream& tstrm)
  {
    // break-statement ::=
    //   "break" break-target-opt ";"
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_break });
    if(!qkwrd) {
      return nullopt;
    }
    auto qtarget = do_accept_break_target_opt(tstrm);
    if(!qtarget) {
      qtarget.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_break xstmt = { ::std::move(sloc), *qtarget };
    return ::std::move(xstmt);
  }

opt<Jump_Target> do_accept_continue_target_opt(Token_Stream& tstrm)
  {
    // continue-target-opt ::=
    //   "while" | "for" | ""
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_while, keyword_for });
    if(qkwrd == keyword_while) {
      return jump_target_while;
    }
    if(qkwrd == keyword_for) {
      return jump_target_for;
    }
    return nullopt;
  }

opt<Statement> do_accept_continue_statement_opt(Token_Stream& tstrm)
  {
    // continue-statement ::=
    //   "continue" continue-target-opt ";"
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_continue });
    if(!qkwrd) {
      return nullopt;
    }
    auto qtarget = do_accept_continue_target_opt(tstrm);
    if(!qtarget) {
      qtarget.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_continue xstmt = { ::std::move(sloc), *qtarget };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_throw_statement_opt(Token_Stream& tstrm)
  {
    // throw-statement ::=
    //   "throw" expression ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_throw });
    if(!qkwrd) {
      return nullopt;
    }
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_throw xstmt = { ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<bool> do_accept_argument_opt(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // argument ::=
    //   reference-specifier expression | expression
    // reference-specifier-opt ::=
    //   "&" | ""
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_andb });
    if(kpunct) {
      bool succ = do_accept_expression(units, tstrm);
      if(!succ) {
        do_throw_parser_error(tstrm, parser_status_expression_expected);
      }
      return true;  // by ref
    }
    bool succ = do_accept_expression(units, tstrm);
    if(succ) {
      return false;  // by value
    }
    return nullopt;
  }

opt<Statement> do_accept_return_statement_opt(Token_Stream& tstrm)
  {
    // return-statement ::=
    //   "return" ( argument | "" ) ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_return });
    if(!qkwrd) {
      return nullopt;
    }
    auto sloc = do_tell_source_location(tstrm);
    cow_vector<Expression_Unit> units;
    auto qref = do_accept_argument_opt(units, tstrm);
    if(!qref) {
      qref.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_return xstmt = { *qref, { ::std::move(sloc), ::std::move(units) } };
    return ::std::move(xstmt);
  }

opt<cow_string> do_accept_assert_message_opt(Token_Stream& tstrm)
  {
    // assert-message ::=
    //   ":" string-literal | ""
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct) {
      return nullopt;
    }
    auto kmsg = do_accept_string_literal_opt(tstrm);
    if(!kmsg) {
      do_throw_parser_error(tstrm, parser_status_string_literal_expected);
    }
    return ::std::move(*kmsg);
  }

opt<Statement> do_accept_assert_statement_opt(Token_Stream& tstrm)
  {
    // assert-statement ::=
    //   "assert" negation-opt expression assert-message-opt ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_assert });
    if(!qkwrd) {
      return nullopt;
    }
    auto kneg = do_accept_negation_opt(tstrm);
    if(!kneg) {
      kneg.emplace();
    }
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    auto kmsg = do_accept_assert_message_opt(tstrm);
    if(!kmsg) {
      kmsg.emplace();
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_assert xstmt = { *kneg, ::std::move(*kexpr), ::std::move(*kmsg) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_try_statement_opt(Token_Stream& tstrm)
  {
    // try-statement ::=
    //   "try" statement "catch" "(" identifier ")" statement
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_try });
    if(!qkwrd) {
      return nullopt;
    }
    auto qbtry = do_accept_statement_as_block_opt(tstrm);
    if(!qbtry) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    // Note that this is the location of the `catch` block.
    auto sloc = do_tell_source_location(tstrm);
    qkwrd = do_accept_keyword_opt(tstrm, { keyword_catch });
    if(!qkwrd) {
      do_throw_parser_error(tstrm, parser_status_keyword_catch_expected);
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto kexcept = do_accept_identifier_opt(tstrm);
    if(!kexcept) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qbcatch = do_accept_statement_as_block_opt(tstrm);
    if(!qbcatch) {
      do_throw_parser_error(tstrm, parser_status_statement_expected);
    }
    Statement::S_try xstmt = { ::std::move(*qbtry), ::std::move(sloc), ::std::move(*kexcept),
                               ::std::move(*qbcatch) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_defer_statement_opt(Token_Stream& tstrm)
  {
    // defer-statement ::=
    //  "defer" expression ";"
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_defer });
    if(!qkwrd) {
      return nullopt;
    }
    auto kexpr = do_accept_expression_opt(tstrm);
    if(!kexpr) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_semicolon_expected);
    }
    Statement::S_defer xstmt = { ::std::move(*kexpr) };
    return ::std::move(xstmt);
  }

opt<Statement> do_accept_nonblock_statement_opt(Token_Stream& tstrm)
  {
    // nonblock-statement ::=
    //   null-statement |
    //   variable-definition | immutable-variable-definition | function-definition |
    //   expression-statement |
    //   if-statement | switch-statement | do-while-statement | while-statement | for-statement |
    //   break-statement | continue-statement | throw-statement | return-statement | assert-statement |
    //   try-statement | defer-statement
    if(auto qstmt = do_accept_null_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_variable_definition_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_immutable_variable_definition_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_function_definition_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_expression_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_if_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_switch_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_do_while_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_while_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_for_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_break_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_continue_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_throw_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_return_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_assert_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_try_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_defer_statement_opt(tstrm)) {
      return qstmt;
    }
    return nullopt;
  }

opt<Statement> do_accept_statement_opt(Token_Stream& tstrm)
  {
    // Check for stack overflows.
    const auto sentry = tstrm.copy_recursion_sentry();
    // statement ::=
    //   block | nonblock-statement
    if(auto qstmt = do_accept_block_statement_opt(tstrm)) {
      return qstmt;
    }
    if(auto qstmt = do_accept_nonblock_statement_opt(tstrm)) {
      return qstmt;
    }
    return nullopt;
  }

opt<Statement::S_block> do_accept_statement_as_block_opt(Token_Stream& tstrm)
  {
    // Check for stack overflows.
    const auto sentry = tstrm.copy_recursion_sentry();
    // statement ::=
    //   block | nonblock-statement
    auto qblock = do_accept_block_opt(tstrm);
    if(qblock) {
      return qblock;
    }
    auto sloc = do_tell_source_location(tstrm);
    auto qstmt = do_accept_nonblock_statement_opt(tstrm);
    if(qstmt) {
      return do_blockify_statement(::std::move(sloc), ::std::move(*qstmt));
    }
    return nullopt;
  }

struct Keyword_Element
  {
    Keyword kwrd;
    Xop xop;
  }
constexpr s_keyword_table[] =
  {
    { keyword_unset,     xop_unset     },
    { keyword_lengthof,  xop_lengthof  },
    { keyword_typeof,    xop_typeof    },
    { keyword_not,       xop_notl      },
    { keyword_abs,       xop_abs       },
    { keyword_sign,      xop_sign      },
    { keyword_sqrt,      xop_sqrt      },
    { keyword_isnan,     xop_isnan     },
    { keyword_isinf,     xop_isinf     },
    { keyword_round,     xop_round     },
    { keyword_floor,     xop_floor     },
    { keyword_ceil,      xop_ceil      },
    { keyword_trunc,     xop_trunc     },
    { keyword_iround,    xop_iround    },
    { keyword_ifloor,    xop_ifloor    },
    { keyword_iceil,     xop_iceil     },
    { keyword_itrunc,    xop_itrunc    },
  };

constexpr bool operator==(const Keyword_Element& lhs, Keyword rhs) noexcept
  {
    return lhs.kwrd == rhs;
  }

struct Punctuator_Element
  {
    Punctuator punct;
    Xop xop;
  }
constexpr s_punctuator_table[] =
  {
    { punctuator_add,   xop_pos      },
    { punctuator_sub,   xop_neg      },
    { punctuator_notb,  xop_notb     },
    { punctuator_notl,  xop_notl     },
    { punctuator_inc,   xop_inc_pre  },
    { punctuator_dec,   xop_dec_pre  },
  };

constexpr bool operator==(const Punctuator_Element& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

bool do_accept_prefix_operator(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // prefix-operator ::=
    //   "+" | "-" | "~" | "!" | "++" | "--" |
    //   "unset" | "lengthof" | "typeof" | "not" |
    //   "__abs" | "__sqrt" | "__sign" | "__isnan" | "__isinf" |
    //   "__round" | "__floor" | "__ceil" | "__trunc" | "__iround" | "__ifloor" | "__iceil" | "__itrunc"
    auto sloc = do_tell_source_location(tstrm);
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return false;
    }
    if(qtok->is_keyword()) {
      auto qcnf = ::std::find(begin(s_keyword_table), end(s_keyword_table), qtok->as_keyword());
      if(qcnf == end(s_keyword_table)) {
        return false;
      }
      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qcnf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    if(qtok->is_punctuator()) {
      auto qcnf = ::std::find(begin(s_punctuator_table), end(s_punctuator_table), qtok->as_punctuator());
      if(qcnf == end(s_punctuator_table)) {
        return false;
      }
      // Return the prefix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qcnf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    return false;
  }

bool do_accept_named_reference(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get an identifier.
    auto sloc = do_tell_source_location(tstrm);
    auto qname = do_accept_identifier_opt(tstrm);
    if(!qname) {
      return false;
    }
    // Replace special names. This is what macros in C do.
    if(*qname == "__file") {
      Expression_Unit::S_literal xunit = { V_string(sloc.file()) };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    if(*qname == "__line") {
      Expression_Unit::S_literal xunit = { V_integer(sloc.line()) };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    Expression_Unit::S_named_reference xunit = { sloc, ::std::move(*qname) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_global_reference(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // global-identifier ::=
    //   "__global" identifier
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_global });
    if(!qkwrd) {
      return false;
    }
    auto qname = do_accept_identifier_opt(tstrm);
    if(!qname) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    Expression_Unit::S_global_reference xunit = { sloc, ::std::move(*qname) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_literal(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get a literal as a `Value`.
    auto qval = do_accept_literal_opt(tstrm);
    if(!qval) {
      return false;
    }
    Expression_Unit::S_literal xunit = { ::std::move(*qval) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_this(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Get the keyword `this`.
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_this });
    if(!qkwrd) {
      return false;
    }
    Expression_Unit::S_named_reference xunit = { sloc, ::rocket::sref("__this") };
    units.emplace_back(::std::move(xunit));
    return true;
  }

opt<Statement::S_block> do_accept_closure_body_opt(Token_Stream& tstrm)
  {
    // closure-body ::=
    //   block | equal-initializer
    auto qblock = do_accept_block_opt(tstrm);
    if(qblock) {
      return qblock;
    }
    auto sloc = do_tell_source_location(tstrm);
    auto qinit = do_accept_equal_initializer_opt(tstrm);
    if(qinit) {
      // In the case of an `equal-initializer`, behave as if it was a `return-statement`.
      // Note that the result is returned by value.
      Statement::S_return xstmt = { false, ::std::move(*qinit) };
      return do_blockify_statement(::std::move(sloc), ::std::move(xstmt));
    }
    return nullopt;
  }

bool do_accept_closure_function(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // closure-function ::=
    //   "func" "(" parameter-list-opt ")" closure-body
    auto sloc = do_tell_source_location(tstrm);
    auto uniq = do_get_unique_id(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
    if(!qkwrd) {
      return false;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    auto kparams = do_accept_parameter_list_opt(tstrm);
    if(!kparams) {
      kparams.emplace();
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    auto qblock = do_accept_closure_body_opt(tstrm);
    if(!qblock) {
      do_throw_parser_error(tstrm, parser_status_open_brace_or_equal_initializer_expected);
    }
    Expression_Unit::S_closure_function xunit = { ::std::move(sloc), uniq, ::std::move(*kparams),
                                                  ::std::move(qblock->stmts) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_unnamed_array(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // unnamed-array ::=
    //   "[" array-element-list-opt "]"
    // array-element-list-opt ::=
    //   array-element-list | ""
    // array-element-list ::=
    //   expression ( ( "," | ";" ) array-element-list-opt | "" )
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(!kpunct) {
      return false;
    }
    uint32_t nelems = 0;
    bool comma_allowed = false;
    for(;;) {
      bool succ = do_accept_expression(units, tstrm);
      if(!succ) {
        break;
      }
      if(nelems >= INT32_MAX) {
        do_throw_parser_error(tstrm, parser_status_too_many_elements);
      }
      nelems += 1;
      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct) {
        break;
      }
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, comma_allowed ? parser_status_closed_bracket_or_comma_expected
                                                 : parser_status_closed_bracket_or_expression_expected);
    }
    Expression_Unit::S_unnamed_array xunit = { nelems };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_unnamed_object(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // unnamed-object ::=
    //   "{" key-mapped-list-opt "}"
    // key-mapped-list-opt ::=
    //   key-mapped-list | ""
    // key-mapped-list ::=
    //   ( string-literal | identifier ) ( "=" | ":" ) expression ( ( "," | ";" ) key-mapped-list-opt | "" )
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
    if(!kpunct) {
      return false;
    }
    cow_vector<phsh_string> keys;
    bool comma_allowed = false;
    for(;;) {
      auto qkey = do_accept_json5_key_opt(tstrm);
      if(!qkey) {
        break;
      }
      // Look for the initializer.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign, punctuator_colon });
      if(!kpunct) {
        do_throw_parser_error(tstrm, parser_status_equals_sign_or_colon_expected);
      }
      bool succ = do_accept_expression(units, tstrm);
      if(!succ) {
        do_throw_parser_error(tstrm, parser_status_expression_expected);
      }
      keys.emplace_back(::std::move(*qkey));
      // Look for the separator.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
      comma_allowed = !kpunct;
      if(!kpunct) {
        break;
      }
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, comma_allowed ? parser_status_closed_brace_or_comma_expected
                                                 : parser_status_closed_brace_or_json5_key_expected);
    }
    Expression_Unit::S_unnamed_object xunit = { ::std::move(keys) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_nested_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // nested-expression ::=
    //   "(" expression ")"
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      return false;
    }
    bool succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    return true;
  }

bool do_accept_fused_multiply_add(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // fused-multiply-add ::=
    //   "__fma" "(" expression "," expression "," expression ")"
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_fma });
    if(!qkwrd) {
      return false;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    bool succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_comma_expected);
    }
    succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_comma_expected);
    }
    succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    Expression_Unit::S_operator_rpn xunit = { sloc, xop_fma, false };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_variadic_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // variadic-function-call ::=
    //   "__vcall" "(" expression "," expression ")"
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_vcall });
    if(!qkwrd) {
      return false;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    bool succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_comma_expected);
    }
    succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    Expression_Unit::S_variadic_call xunit = { ::std::move(sloc) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

opt<bool> do_accept_function_argument_opt(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    auto qref = do_accept_argument_opt(units, tstrm);
    if(!qref) {
      return nullopt;
    }
    Expression_Unit::S_argument_finish xunit = { *qref };
    units.emplace_back(::std::move(xunit));
    return *qref;
  }

bool do_accept_import_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // import-function-call ::=
    //   "import" "(" argument-list ")"
    auto sloc = do_tell_source_location(tstrm);
    auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_import });
    if(!qkwrd) {
      return false;
    }
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_open_parenthesis_expected);
    }
    uint32_t nargs = 0;
    for(;;) {
      auto qref = do_accept_function_argument_opt(units, tstrm);
      if(!qref) {
        do_throw_parser_error(tstrm, parser_status_argument_expected);
      }
      if(nargs >= INT32_MAX) {
        do_throw_parser_error(tstrm, parser_status_too_many_elements);
      }
      nargs += 1;
      // Look for the next argument.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    Expression_Unit::S_import_call xunit = { ::std::move(sloc), nargs };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_primary_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // primary-expression ::=
    //   identifier | global-identifier | literal | "this" | closure-function | unnamed-array | unnamed-object |
    //   nested-expression | fused-multiply-add | variadic-function-call | import-function-call
    if(do_accept_named_reference(units, tstrm)) {
      return true;
    }
    if(do_accept_global_reference(units, tstrm)) {
      return true;
    }
    if(do_accept_literal(units, tstrm)) {
      return true;
    }
    if(do_accept_this(units, tstrm)) {
      return true;
    }
    if(do_accept_closure_function(units, tstrm)) {
      return true;
    }
    if(do_accept_unnamed_array(units, tstrm)) {
      return true;
    }
    if(do_accept_unnamed_object(units, tstrm)) {
      return true;
    }
    if(do_accept_nested_expression(units, tstrm)) {
      return true;
    }
    if(do_accept_fused_multiply_add(units, tstrm)) {
      return true;
    }
    if(do_accept_variadic_function_call(units, tstrm)) {
      return true;
    }
    if(do_accept_import_function_call(units, tstrm)) {
      return true;
    }
    return false;
  }

struct Postfix_Operator_Element
  {
    Punctuator punct;
    Xop xop;
  }
constexpr s_postfix_operator_table[] =
  {
    { punctuator_inc,   xop_inc_post  },
    { punctuator_dec,   xop_dec_post  },
    { punctuator_head,  xop_head      },
    { punctuator_tail,  xop_tail      },
  };

constexpr bool operator==(const Postfix_Operator_Element& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

bool do_accept_postfix_operator(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-operator ::=
    //   "++" | "--" | "[^]" | "[$]"
    auto sloc = do_tell_source_location(tstrm);
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return false;
    }
    if(qtok->is_punctuator()) {
      auto qcnf = ::std::find(begin(s_postfix_operator_table), end(s_postfix_operator_table), qtok->as_punctuator());
      if(qcnf == end(s_postfix_operator_table)) {
        return false;
      }
      // Return the postfix operator and discard this token.
      tstrm.shift();
      Expression_Unit::S_operator_rpn xunit = { sloc, qcnf->xop, false };
      units.emplace_back(::std::move(xunit));
      return true;
    }
    return false;
  }

bool do_accept_postfix_function_call(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-function-call ::=
    //   "(" argument-list-opt ")"
    // argument-list-opt ::=
    //   argument-list | ""
    // argument-list ::=
    //   argument ( "," argument-list | "" )
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
    if(!kpunct) {
      return false;
    }
    uint32_t nargs = 0;
    for(;;) {
      auto qref = do_accept_function_argument_opt(units, tstrm);
      if(!nargs && !qref) {
        break;
      }
      if(!qref) {
        do_throw_parser_error(tstrm, parser_status_argument_expected);
      }
      if(nargs >= INT32_MAX) {
        do_throw_parser_error(tstrm, parser_status_too_many_elements);
      }
      nargs += 1;
      // Look for the next argument.
      kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
      if(!kpunct) {
        break;
      }
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_parenthesis_expected);
    }
    Expression_Unit::S_function_call xunit = { ::std::move(sloc), nargs };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_postfix_subscript(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-subscript ::=
    //   "[" expression "]"
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
    if(!kpunct) {
      return false;
    }
    bool succ = do_accept_expression(units, tstrm);
    if(!succ) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_closed_bracket_expected);
    }
    Expression_Unit::S_operator_rpn xunit = { sloc, xop_subscr, false };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_postfix_member_access(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // postfix-member-access ::=
    //   "." ( string-literal | identifier )
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_dot });
    if(!kpunct) {
      return false;
    }
    auto qkey = do_accept_json5_key_opt(tstrm);
    if(!qkey) {
      do_throw_parser_error(tstrm, parser_status_identifier_expected);
    }
    Expression_Unit::S_member_access xunit = { sloc, ::std::move(*qkey) };
    units.emplace_back(::std::move(xunit));
    return true;
  }

bool do_accept_infix_element(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // infix-element ::=
    //   prefix-operator-list-opt primary-expression postfix-operator-list-opt
    // prefix-operator-list-opt ::=
    //   prefix-operator-list | ""
    // prefix-operator-list ::=
    //   prefix-operator prefix-operator-list-opt
    // postfix-operator-list-opt ::=
    //   postfix-operator-list | ""
    // postfix-operator-list ::=
    //   postfix-operator | postfix-function-call | postfix-subscript | postfix-member-access
    cow_vector<Expression_Unit> prefixes;
    bool succ;
    do {
      succ = do_accept_prefix_operator(prefixes, tstrm);
    } while(succ);
    // Get a `primary-expression` with suffixes.
    // Fail if some prefixes have been consumed but no primary expression can be accepted.
    succ = do_accept_primary_expression(units, tstrm);
    if(!succ) {
      if(prefixes.empty()) {
        return false;
      }
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    do {
      succ = do_accept_postfix_operator(units, tstrm) ||
             do_accept_postfix_function_call(units, tstrm) ||
             do_accept_postfix_subscript(units, tstrm) ||
             do_accept_postfix_member_access(units, tstrm);
    } while(succ);
    // Append prefixes in reverse order.
    // N.B. Prefix operators have lower precedence than postfix ones.
    units.append(::std::make_move_iterator(prefixes.mut_rbegin()),
                 ::std::make_move_iterator(prefixes.mut_rend()));
    return true;
  }

opt<Infix_Element> do_accept_infix_head_opt(Token_Stream& tstrm)
  {
    // infix-head ::=
    //   infix-element
    cow_vector<Expression_Unit> units;
    bool succ = do_accept_infix_element(units, tstrm);
    if(!succ) {
      return nullopt;
    }
    Infix_Element::S_head xelem = { ::std::move(units) };
    return ::std::move(xelem);
  }

opt<Infix_Element> do_accept_infix_operator_ternary_opt(Token_Stream& tstrm)
  {
    // infix-operator-ternary ::=
    //   ( "?" | "?=" ) expression ":"
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_quest_eq });
    if(!kpunct) {
      return nullopt;
    }
    cow_vector<Expression_Unit> btrue;
    if(!do_accept_expression(btrue, tstrm)) {
      do_throw_parser_error(tstrm, parser_status_expression_expected);
    }
    kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_colon });
    if(!kpunct) {
      do_throw_parser_error(tstrm, parser_status_colon_expected);
    }
    Infix_Element::S_ternary xelem = { sloc, *kpunct == punctuator_quest_eq, ::std::move(btrue), nullopt };
    return ::std::move(xelem);
  }

opt<Infix_Element> do_accept_infix_operator_logical_and_opt(Token_Stream& tstrm)
  {
    // infix-operator-logical-and ::=
    //   "&&" | "&&=" | "and"
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_andl, punctuator_andl_eq });
    if(!kpunct) {
      auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_and });
      if(!qkwrd) {
        return nullopt;
      }
      kpunct.emplace(punctuator_andl);
    }
    Infix_Element::S_logical_and xelem = { sloc, *kpunct == punctuator_andl_eq, nullopt };
    return ::std::move(xelem);
  }

opt<Infix_Element> do_accept_infix_operator_logical_or_opt(Token_Stream& tstrm)
  {
    // infix-operator-logical-or ::=
    //   "||" | "||=" | "or"
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_orl, punctuator_orl_eq });
    if(!kpunct) {
      auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_or });
      if(!qkwrd) {
        return nullopt;
      }
      kpunct.emplace(punctuator_orl);
    }
    Infix_Element::S_logical_or xelem = { sloc, *kpunct == punctuator_orl_eq, nullopt };
    return ::std::move(xelem);
  }

opt<Infix_Element> do_accept_infix_operator_coalescence_opt(Token_Stream& tstrm)
  {
    // infix-operator-coalescence ::=
    //   "??" | "??="
    auto sloc = do_tell_source_location(tstrm);
    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_coales, punctuator_coales_eq });
    if(!kpunct) {
      return nullopt;
    }
    Infix_Element::S_coalescence xelem = { sloc, *kpunct == punctuator_coales_eq, nullopt };
    return ::std::move(xelem);
  }

struct Infix_Operator_Element
  {
    Punctuator punct;
    Xop xop;
    bool assign;
  }
constexpr s_infix_operator_table[] =
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

constexpr bool operator==(const Infix_Operator_Element& lhs, Punctuator rhs) noexcept
  {
    return lhs.punct == rhs;
  }

opt<Infix_Element> do_accept_infix_operator_general_opt(Token_Stream& tstrm)
  {
    // infix-operator-general ::=
    //   "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
    //   "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
    //   "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"
    auto sloc = do_tell_source_location(tstrm);
    auto qtok = tstrm.peek_opt();
    if(!qtok) {
      return nullopt;
    }
    if(qtok->is_punctuator()) {
      auto qcnf = ::std::find(begin(s_infix_operator_table), end(s_infix_operator_table), qtok->as_punctuator());
      if(qcnf == end(s_infix_operator_table)) {
        return nullopt;
      }
      // Return the infix operator and discard this token.
      tstrm.shift();
      Infix_Element::S_general xelem = { sloc, qcnf->xop, qcnf->assign, nullopt };
      return ::std::move(xelem);
    }
    return nullopt;
  }

opt<Infix_Element> do_accept_infix_operator_opt(Token_Stream& tstrm)
  {
    // infix-operator ::=
    //   infix-operator-ternary | infix-operator-logical-and | infix-operator-logical-or |
    //   infix-operator-coalescence | infix-operator-general
    if(auto qelem = do_accept_infix_operator_ternary_opt(tstrm)) {
      return qelem;
    }
    if(auto qelem = do_accept_infix_operator_logical_and_opt(tstrm)) {
      return qelem;
    }
    if(auto qelem = do_accept_infix_operator_logical_or_opt(tstrm)) {
      return qelem;
    }
    if(auto qelem = do_accept_infix_operator_coalescence_opt(tstrm)) {
      return qelem;
    }
    if(auto qelem = do_accept_infix_operator_general_opt(tstrm)) {
      return qelem;
    }
    return nullopt;
  }

bool do_accept_expression(cow_vector<Expression_Unit>& units, Token_Stream& tstrm)
  {
    // Check for stack overflows.
    const auto sentry = tstrm.copy_recursion_sentry();
    // expression ::=
    //   infix-head infix-carriage-list-opt
    // infix-carriage-list-opt ::=
    //   infix-carriage-list | ""
    // infix-carriage-list ::=
    //   infix-carriage infix-carriage-list-opt
    // infix-carriage ::=
    //   infix-operator infix-element
    auto qelem = do_accept_infix_head_opt(tstrm);
    if(!qelem) {
      return false;
    }
    cow_vector<Infix_Element> stack;
    stack.emplace_back(::std::move(*qelem));
    for(;;) {
      auto qnext = do_accept_infix_operator_opt(tstrm);
      if(!qnext) {
        break;
      }
      bool succ = do_accept_infix_element(qnext->open_junction(), tstrm);
      if(!succ) {
        do_throw_parser_error(tstrm, parser_status_expression_expected);
      }
      // Assignment operations have the lowest precedence and group from right to left.
      if(stack.back().tell_precedence() < precedence_assignment) {
        // Collapse elements that have no lower precedence and group from left to right.
        auto preced_next = qnext->tell_precedence();
        while((stack.size() >= 2) && (stack.back().tell_precedence() <= preced_next)) {
          qelem = ::std::move(stack.mut_back());
          stack.pop_back();
          qelem->extract(stack.mut_back().open_junction());
        }
      }
      stack.emplace_back(::std::move(*qnext));
    }
    while(stack.size() >= 2) {
      qelem = ::std::move(stack.mut_back());
      stack.pop_back();
      qelem->extract(stack.mut_back().open_junction());
    }
    stack.mut_back().extract(units);
    return true;
  }

}  // namespace

Statement_Sequence& Statement_Sequence::reload(Token_Stream& tstrm, const Compiler_Options& /*opts*/)
  {
    // Parse the document recursively.
    cow_vector<Statement> stmts;
    // Destroy the contents of `*this` and reuse their storage, if any.
    stmts.swap(this->m_stmts);
    stmts.clear();

    // document ::=
    //   statement-list-opt
    while(auto qstmt = do_accept_statement_opt(tstrm))
      stmts.emplace_back(::std::move(*qstmt));

    // If there are any non-statement tokens left in the stream, fail.
    if(!tstrm.empty())
      do_throw_parser_error(tstrm, parser_status_statement_expected);

    // Succeed.
    this->m_stmts = ::std::move(stmts);
    return *this;
  }

}  // namespace Asteria
