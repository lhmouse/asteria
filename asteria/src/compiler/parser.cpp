// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "infix_element.hpp"
#include "../syntax/statement.hpp"
#include "../syntax/xprunit.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    Parser_Error do_make_parser_error(const Token_Stream& tstrm, Parser_Status status)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return status;
        }
        return Parser_Error(qtok->line(), qtok->offset(), qtok->length(), status);
      }

    Source_Location do_tell_source_location(const Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return std::make_pair(rocket::sref("<end of stream>"), -1);
        }
        return Source_Location(qtok->file(), qtok->line());
      }

    opt<Keyword> do_accept_keyword_opt(Token_Stream& tstrm, std::initializer_list<Keyword> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is one of the acceptable keywords.
        if(!qtok->is_keyword()) {
          return rocket::nullopt;
        }
        auto keyword = qtok->as_keyword();
        if(rocket::is_none_of(keyword, accept)) {
          return rocket::nullopt;
        }
        // Return the keyword and discard this token.
        tstrm.shift();
        return keyword;
      }

    opt<Punctuator> do_accept_punctuator_opt(Token_Stream& tstrm, std::initializer_list<Punctuator> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is one of the acceptable punctuators.
        if(!qtok->is_punctuator()) {
          return rocket::nullopt;
        }
        auto punct = qtok->as_punctuator();
        if(rocket::is_none_of(punct, accept)) {
          return rocket::nullopt;
        }
        // Return the punctuator and discard this token.
        tstrm.shift();
        return punct;
      }

    opt<cow_string> do_accept_identifier_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is an identifier.
        if(!qtok->is_identifier()) {
          return rocket::nullopt;
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
          return rocket::nullopt;
        }
        // See whether it is a string literal.
        if(!qtok->is_string_literal()) {
          return rocket::nullopt;
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
          return rocket::nullopt;
        }
        // See whether it is a keyword, identifier, or string literal.
        if(qtok->is_keyword()) {
          auto keyword = qtok->as_keyword();
          // Treat the keyword as a plain identifier and discard this token.
          tstrm.shift();
          return rocket::sref(stringify_keyword(keyword));
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
        return rocket::nullopt;
      }

    Value do_generate_null()
      {
        return G_null();
      }
    Value do_generate_false()
      {
        return G_boolean(false);
      }
    Value do_generate_true()
      {
        return G_boolean(true);
      }
    Value do_generate_nan()
      {
        return G_real(NAN);
      }
    Value do_generate_infinity()
      {
        return G_real(INFINITY);
      }

    struct Literal_Element
      {
        Keyword keyword;
        Value (*generator)();
      }
    const s_literal_table[] =
      {
        { keyword_null,      do_generate_null      },
        { keyword_false,     do_generate_false     },
        { keyword_true,      do_generate_true      },
        { keyword_nan,       do_generate_nan       },
        { keyword_infinity,  do_generate_infinity  },
      };

    constexpr bool operator==(const Literal_Element& lhs, Keyword rhs) noexcept
      {
        return lhs.keyword == rhs;
      }

    opt<Value> do_accept_literal_value_opt(Token_Stream& tstrm)
      {
        // literal ::=
        //   null-literal | boolean-literal | string-literal | numeric-literal | nan-literal | infinity-literal
        // null-literal ::=
        //   "null"
        // boolean-litearl ::=
        //   "false" | "true"
        // nan-literal ::=
        //   "nan"
        // infinity-literal ::=
        //   "infinity"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_keyword()) {
          auto qconf = std::find(std::begin(s_literal_table), std::end(s_literal_table), qtok->as_keyword());
          if(qconf == std::end(s_literal_table)) {
            return rocket::nullopt;
          }
          auto generator = qconf->generator;
          // Discard this token and create a new value using the generator.
          tstrm.shift();
          return (*generator)();
        }
        if(qtok->is_integer_literal()) {
          auto val = G_integer(qtok->as_integer_literal());
          // Copy the value and discard this token.
          tstrm.shift();
          return val;
        }
        if(qtok->is_real_literal()) {
          auto val = G_real(qtok->as_real_literal());
          // Copy the value and discard this token.
          tstrm.shift();
          return val;
        }
        if(qtok->is_string_literal()) {
          auto val = G_string(qtok->as_string_literal());
          // Copy the value and discard this token.
          tstrm.shift();
          do_concatenate_string_literal_sequence(val, tstrm);
          return val;
        }
        return rocket::nullopt;
      }

    opt<bool> do_accept_negation_opt(Token_Stream& tstrm)
      {
        // negation ::=
        //   "!" | "not"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_keyword() && (qtok->as_keyword() == keyword_not)) {
          // Discard this token.
          tstrm.shift();
          return true;
        }
        if(qtok->is_punctuator() && (qtok->as_punctuator() == punctuator_notl)) {
          // Discard this token.
          tstrm.shift();
          return true;
        }
        return rocket::nullopt;
      }

    // Accept a statement; a blockt is converted to a single statement.
    extern opt<Statement> do_accept_statement_opt(Token_Stream& tstrm);
    // Accept a statement; a non-block statement is converted to a block consisting of a single statement.
    extern opt<cow_vector<Statement>> do_accept_statement_as_block_opt(Token_Stream& tstrm);

    extern bool do_accept_expression(cow_vector<Xprunit>& units, Token_Stream& tstrm);

    opt<cow_vector<Xprunit>> do_accept_expression_opt(Token_Stream& tstrm)
      {
        // expression-opt ::=
        //   expression | ""
        cow_vector<Xprunit> units;
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          return rocket::nullopt;
        }
        return rocket::move(units);
      }

    opt<cow_vector<Statement>> do_accept_block_opt(Token_Stream& tstrm)
      {
        // block ::=
        //   "{" statement-list-opt "}"
        // statement-list-opt ::=
        //   statement-list | ""
        // statement-list ::=
        //   statement statement-list-opt
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        cow_vector<Statement> body;
        for(;;) {
          auto qstmt = do_accept_statement_opt(tstrm);
          if(!qstmt) {
            break;
          }
          body.emplace_back(rocket::move(*qstmt));
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_brace_or_statement_expected);
        }
        return rocket::move(body);
      }

    opt<Statement> do_accept_block_statement_opt(Token_Stream& tstrm)
      {
        auto qbody = do_accept_block_opt(tstrm);
        if(!qbody) {
          return rocket::nullopt;
        }
        Statement::S_block xstmt = { rocket::move(*qbody) };
        return rocket::move(xstmt);
      }

    opt<cow_vector<Xprunit>> do_accept_equal_initializer_opt(Token_Stream& tstrm)
      {
        // equal-initializer-opt ::=
        //   equal-initializer | ""
        // equal-initializer ::=
        //   "=" expression
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign });
        if(!kpunct) {
          return rocket::nullopt;
        }
        return do_accept_expression_opt(tstrm);
      }

    opt<Statement> do_accept_null_statement_opt(Token_Stream& tstrm)
      {
        // null-statement ::=
        //   ";"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Statement::S_expression xstmt = { rocket::clear };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_variable_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // variable-definition ::=
        //   "var" identifier equal-initailizer-opt ( "," identifier equal-initializer-opt | "" ) ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_var });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        cow_bivector<phsh_string, cow_vector<Xprunit>> vars;
        for(;;) {
          auto qname = do_accept_identifier_opt(tstrm);
          if(!qname) {
            throw do_make_parser_error(tstrm, parser_status_identifier_expected);
          }
          auto qinit = do_accept_equal_initializer_opt(tstrm);
          if(!qinit) {
            qinit.emplace();
          }
          vars.emplace_back(rocket::move(*qname), rocket::move(*qinit));
          // Look for the separator. The first declaration is required.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_variable xstmt = { rocket::move(sloc), false, rocket::move(vars) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_immutable_variable_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // immutable-variable-definition ::=
        //   "const" identifier equal-initailizer ( "," identifier equal-initializer | "" ) ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_const });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        cow_bivector<phsh_string, cow_vector<Xprunit>> vars;
        for(;;) {
          auto qname = do_accept_identifier_opt(tstrm);
          if(!qname) {
            throw do_make_parser_error(tstrm, parser_status_identifier_expected);
          }
          auto qinit = do_accept_equal_initializer_opt(tstrm);
          if(!qinit) {
            throw do_make_parser_error(tstrm, parser_status_equals_sign_expected);
          }
          vars.emplace_back(rocket::move(*qname), rocket::move(*qinit));
          // Look for the separator. The first declaration is required.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_variable xstmt = { rocket::move(sloc), true, rocket::move(vars) };
        return rocket::move(xstmt);
      }

    opt<cow_vector<phsh_string>> do_accept_parameter_list_opt(Token_Stream& tstrm)
      {
        // parameter-list-opt ::=
        //   identifier parameter-list-carriage-opt | "..." | ""
        // parameter-list-carriage-opt ::=
        //   "," ( identifier comma-parameter-list-opt | "..." ) | ""
        auto qname = do_accept_identifier_opt(tstrm);
        if(qname) {
          cow_vector<phsh_string> names;
          for(;;) {
            names.emplace_back(rocket::move(*qname));
            // Look for the separator.
            auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
            if(!kpunct) {
              break;
            }
            qname = do_accept_identifier_opt(tstrm);
            if(!qname) {
              // The parameter list may end with an ellipsis.
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
              if(kpunct) {
                names.emplace_back(rocket::sref("..."));
                break;
              }
              throw do_make_parser_error(tstrm, parser_status_parameter_or_ellipsis_expected);
            }
          }
          return rocket::move(names);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_ellipsis });
        if(kpunct) {
          cow_vector<phsh_string> names;
          names.emplace_back(rocket::sref("..."));
          return rocket::move(names);
        }
        return rocket::nullopt;
      }

    opt<cow_vector<phsh_string>> do_accept_parameter_list_declaration_opt(Token_Stream& tstrm)
      {
        // parameter-list-declaration ::=
        //   "(" parameter-list-opt ")"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        auto qnames = do_accept_parameter_list_opt(tstrm);
        if(!qnames) {
          qnames.emplace();
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        return rocket::move(*qnames);
      }

    opt<Statement> do_accept_function_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // function-definition ::=
        //   "func" identifier parameter-declaration block
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qname = do_accept_identifier_opt(tstrm);
        if(!qname) {
          throw do_make_parser_error(tstrm, parser_status_identifier_expected);
        }
        auto kparams = do_accept_parameter_list_declaration_opt(tstrm);
        if(!kparams) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qbody = do_accept_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_open_brace_expected);
        }
        Statement::S_function xstmt = { rocket::move(sloc), rocket::move(*qname), rocket::move(*kparams), rocket::move(*qbody) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_expression_statement_opt(Token_Stream& tstrm)
      {
        // expression-statement ::=
        //   expression ";"
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_expression xstmt = { rocket::move(*kexpr) };
        return rocket::move(xstmt);
      }

    opt<cow_vector<Statement>> do_accept_else_branch_opt(Token_Stream& tstrm)
      {
        // else-branch-opt ::=
        //   "else" statement | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_else });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        return rocket::move(*qbody);
      }

    opt<Statement> do_accept_if_statement_opt(Token_Stream& tstrm)
      {
        // if-statement ::=
        //   "if" negation-opt "(" expression ")" statement else-branch-opt
        // negation-opt ::=
        //   negation | ""
        // negation ::=
        //   "!" | "not"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_if });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qbtrue = do_accept_statement_as_block_opt(tstrm);
        if(!qbtrue) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        auto qbfalse = do_accept_else_branch_opt(tstrm);
        if(!qbfalse) {
          qbfalse.emplace();
        }
        Statement::S_if xstmt = { *kneg, rocket::move(*qcond), rocket::move(*qbtrue), rocket::move(*qbfalse) };
        return rocket::move(xstmt);
      }

    opt<pair<cow_vector<Xprunit>, cow_vector<Statement>>> do_accept_case_clause_opt(Token_Stream& tstrm)
      {
        // case-clause ::=
        //   "case" expression ":" statement-list-opt
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_case });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_colon_expected);
        }
        auto qclause = do_accept_statement_as_block_opt(tstrm);
        if(!qclause) {
          qclause.emplace();
        }
        return std::make_pair(rocket::move(*qcond), rocket::move(*qclause));
      }

    opt<pair<cow_vector<Xprunit>, cow_vector<Statement>>> do_accept_default_clause_opt(Token_Stream& tstrm)
      {
        // default-clause ::=
        //   "default" ":" statement-list-opt
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_default });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_colon_expected);
        }
        auto qclause = do_accept_statement_as_block_opt(tstrm);
        if(!qclause) {
          qclause.emplace();
        }
        return std::make_pair(rocket::clear, rocket::move(*qclause));
      }

    opt<pair<cow_vector<Xprunit>, cow_vector<Statement>>> do_accept_switch_clause_opt(Token_Stream& tstrm)
      {
        // switch-clause ::=
        //   case-clause | default-clause
        auto qclause = do_accept_case_clause_opt(tstrm);
        if(qclause) {
          return qclause;
        }
        qclause = do_accept_default_clause_opt(tstrm);
        if(qclause) {
          return qclause;
        }
        return qclause;
      }

    opt<cow_bivector<cow_vector<Xprunit>, cow_vector<Statement>>> do_accept_switch_clause_list_opt(Token_Stream& tstrm)
      {
        // switch-clause-list ::=
        //   switch-clause switch-clause-list-opt
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        cow_bivector<cow_vector<Xprunit>, cow_vector<Statement>> clauses;
        for(;;) {
          auto qclause = do_accept_switch_clause_opt(tstrm);
          if(!qclause) {
            break;
          }
          clauses.emplace_back(rocket::move(*qclause));
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_brace_or_switch_clause_expected);
        }
        return rocket::move(clauses);
      }

    opt<Statement> do_accept_switch_statement_opt(Token_Stream& tstrm)
      {
        // switch-statement ::=
        //   "switch" "(" expression ")" switch-block
        // switch-block ::=
        //   "{" swtich-clause-list-opt "}"
        // switch-clause-list-opt ::=
        //   switch-clause-list | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_switch });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qctrl = do_accept_expression_opt(tstrm);
        if(!qctrl) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qclauses = do_accept_switch_clause_list_opt(tstrm);
        if(!qclauses) {
          throw do_make_parser_error(tstrm, parser_status_open_brace_expected);
        }
        Statement::S_switch xstmt = { rocket::move(*qctrl), rocket::move(*qclauses) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_do_while_statement_opt(Token_Stream& tstrm)
      {
        // do-while-statement ::=
        //   "do" statement "while" negation-opt "(" expression ")" ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_do });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_do_while xstmt = { rocket::move(*qbody), *kneg, rocket::move(*qcond) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_while_statement_opt(Token_Stream& tstrm)
      {
        // while-statement ::=
        //   "while" negation-opt "(" expression ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_while });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        Statement::S_while xstmt = { *kneg, rocket::move(*qcond), rocket::move(*qbody) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_for_complement_range_opt(Token_Stream& tstrm)
      {
        // for-complement-range ::=
        //   "each" identifier "," identifier ":" expression ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_each });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qkname = do_accept_identifier_opt(tstrm);
        if(!qkname) {
          throw do_make_parser_error(tstrm, parser_status_identifier_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_comma_expected);
        }
        auto qmname = do_accept_identifier_opt(tstrm);
        if(!qmname) {
          throw do_make_parser_error(tstrm, parser_status_identifier_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_colon_expected);
        }
        auto qinit = do_accept_expression_opt(tstrm);
        if(!qinit) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        Statement::S_for_each xstmt = { rocket::move(*qkname), rocket::move(*qmname), rocket::move(*qinit), rocket::move(*qbody) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_for_initializer_opt(Token_Stream& tstrm)
      {
        // for-initializer ::=
        //   null-statement | variable-definition | immutable-variable-definition | expression-statement
        auto qinit = do_accept_null_statement_opt(tstrm);
        if(qinit) {
          return qinit;
        }
        qinit = do_accept_variable_definition_opt(tstrm);
        if(qinit) {
          return qinit;
        }
        qinit = do_accept_immutable_variable_definition_opt(tstrm);
        if(qinit) {
          return qinit;
        }
        return qinit;
      }

    opt<Statement> do_accept_for_complement_triplet_opt(Token_Stream& tstrm)
      {
        // for-complement-triplet ::=
        //   for-initializer expression-opt ";" expression-opt ")" statement
        auto qinit = do_accept_for_initializer_opt(tstrm);
        if(!qinit) {
          return rocket::nullopt;
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          qcond.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        auto kstep = do_accept_expression_opt(tstrm);
        if(!kstep) {
          kstep.emplace();
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        cow_vector<Statement> rvinit;
        rvinit.emplace_back(rocket::move(*qinit));
        Statement::S_for xstmt = { rocket::move(rvinit), rocket::move(*qcond), rocket::move(*kstep), rocket::move(*qbody) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_for_complement_opt(Token_Stream& tstrm)
      {
        // for-complement ::=
        //   for-complement-range | for-complement-triplet
        auto qcompl = do_accept_for_complement_range_opt(tstrm);
        if(qcompl) {
          return qcompl;
        }
        qcompl = do_accept_for_complement_triplet_opt(tstrm);
        if(qcompl) {
          return qcompl;
        }
        return qcompl;
      }

    opt<Statement> do_accept_for_statement_opt(Token_Stream& tstrm)
      {
        // for-statement ::=
        //   "for" "(" for-complement
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_for });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qcompl = do_accept_for_complement_opt(tstrm);
        if(!qcompl) {
          throw do_make_parser_error(tstrm, parser_status_for_statement_initializer_expected);
        }
        return rocket::move(*qcompl);
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
        return rocket::nullopt;
      }

    opt<Statement> do_accept_break_statement_opt(Token_Stream& tstrm)
      {
        // break-statement ::=
        //   "break" break-target-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_break });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qtarget = do_accept_break_target_opt(tstrm);
        if(!qtarget) {
          qtarget.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_break xstmt = { *qtarget };
        return rocket::move(xstmt);
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
        return rocket::nullopt;
      }

    opt<Statement> do_accept_continue_statement_opt(Token_Stream& tstrm)
      {
        // continue-statement ::=
        //   "continue" continue-target-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_continue });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qtarget = do_accept_continue_target_opt(tstrm);
        if(!qtarget) {
          qtarget.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_continue xstmt = { *qtarget };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_throw_statement_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // throw-statement ::=
        //   "throw" expression ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_throw });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_throw xstmt = { rocket::move(sloc), rocket::move(*kexpr) };
        return rocket::move(xstmt);
      }

    opt<bool> do_accept_reference_specifier_opt(Token_Stream& tstrm)
      {
        // reference-specifier-opt ::=
        //   "&" | ""
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_andb });
        if(!kpunct) {
          return rocket::nullopt;
        }
        return true;
      }

    opt<Statement> do_accept_return_statement_opt(Token_Stream& tstrm)
      {
        // return-statement ::=
        //   "return" reference-specifier-opt expression-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_return });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qref = do_accept_reference_specifier_opt(tstrm);
        if(!qref) {
          qref.emplace();
        }
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          kexpr.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_return xstmt = { *qref, rocket::move(*kexpr) };
        return rocket::move(xstmt);
      }

    opt<cow_string> do_accept_assert_message_opt(Token_Stream& tstrm)
      {
        // assert-message ::=
        //   ":" string-literal | ""
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
        if(!kpunct) {
          return rocket::nullopt;
        }
        auto kmsg = do_accept_string_literal_opt(tstrm);
        if(!kmsg) {
          throw do_make_parser_error(tstrm, parser_status_string_literal_expected);
        }
        return rocket::move(*kmsg);
      }

    opt<Statement> do_accept_assert_statement_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // assert-statement ::=
        //   "assert" negation-opt expression assert-message-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_assert });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        auto kmsg = do_accept_assert_message_opt(tstrm);
        if(!kmsg) {
          kmsg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_semicolon_expected);
        }
        Statement::S_assert xstmt = { rocket::move(sloc), *kneg, rocket::move(*kexpr), rocket::move(*kmsg) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_try_statement_opt(Token_Stream& tstrm)
      {
        // try-statement ::=
        //   "try" statement "catch" "(" identifier ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_try });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbtry = do_accept_statement_as_block_opt(tstrm);
        if(!qbtry) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        // Note that this is the location of the `catch` block.
        auto sloc = do_tell_source_location(tstrm);
        qkwrd = do_accept_keyword_opt(tstrm, { keyword_catch });
        if(!qkwrd) {
          throw do_make_parser_error(tstrm, parser_status_keyword_catch_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto kexcept = do_accept_identifier_opt(tstrm);
        if(!kexcept) {
          throw do_make_parser_error(tstrm, parser_status_identifier_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        auto qbcatch = do_accept_statement_as_block_opt(tstrm);
        if(!qbcatch) {
          throw do_make_parser_error(tstrm, parser_status_statement_expected);
        }
        Statement::S_try xstmt = { rocket::move(*qbtry), rocket::move(sloc), rocket::move(*kexcept), rocket::move(*qbcatch) };
        return rocket::move(xstmt);
      }

    opt<Statement> do_accept_nonblock_statement_opt(Token_Stream& tstrm)
      {
        // nonblock-statement ::=
        //   null-statement |
        //   variable-definition | immutable-variable-definition | function-definition |
        //   expression-statement |
        //   if-statement | switch-statement | do-while-statement | while-statement | for-statement |
        //   break-statement | continue-statement | throw-statement | return-statement | assert-statement |
        //   try-statement
        auto qstmt = do_accept_null_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_variable_definition_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_immutable_variable_definition_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_function_definition_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_expression_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_if_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_switch_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_do_while_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_while_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_for_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_break_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_continue_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_throw_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_return_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_assert_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_try_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        return qstmt;
      }

    opt<Statement> do_accept_statement_opt(Token_Stream& tstrm)
      {
        // statement ::=
        //   block | nonblock-statement
        auto qstmt = do_accept_block_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        qstmt = do_accept_nonblock_statement_opt(tstrm);
        if(qstmt) {
          return qstmt;
        }
        return qstmt;
      }

    opt<cow_vector<Statement>> do_accept_statement_as_block_opt(Token_Stream& tstrm)
      {
        // statement ::=
        //   block | nonblock-statement
        auto qblock = do_accept_block_opt(tstrm);
        if(qblock) {
          return qblock;
        }
        auto qstmt = do_accept_nonblock_statement_opt(tstrm);
        if(qstmt) {
          qblock.emplace().emplace_back(rocket::move(*qstmt));
          return qblock;
        }
        return qblock;
      }

    struct Keyword_Element
      {
        Keyword keyword;
        Xop xop;
      }
    constexpr s_keyword_table[] =
      {
        { keyword_unset,     xop_unset     },
        { keyword_lengthof,  xop_lengthof  },
        { keyword_typeof,    xop_typeof    },
        { keyword_not,       xop_notl      },
        { keyword_abs,       xop_abs       },
        { keyword_signb,     xop_signb     },
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
        return lhs.keyword == rhs;
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

    bool do_accept_prefix_operator(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // prefix-operator ::=
        //   "+" | "-" | "~" | "!" | "++" | "--" |
        //   "unset" | "lengthof" | "typeof" | "not" |
        //   "__abs" | "__sqrt" | "__signb" | "__isnan" | "__isinf" |
        //   "__round" | "__floor" | "__ceil" | "__trunc" | "__iround" | "__ifloor" | "__iceil" | "__itrunc"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        if(qtok->is_keyword()) {
          auto qconf = std::find(std::begin(s_keyword_table), std::end(s_keyword_table), qtok->as_keyword());
          if(qconf == std::end(s_keyword_table)) {
            return false;
          }
          // Return the prefix operator and discard this token.
          tstrm.shift();
          Xprunit::S_operator_rpn xunit = { qconf->xop, false };
          units.emplace_back(rocket::move(xunit));
          return true;
        }
        if(qtok->is_punctuator()) {
          auto qconf = std::find(std::begin(s_punctuator_table), std::end(s_punctuator_table), qtok->as_punctuator());
          if(qconf == std::end(s_punctuator_table)) {
            return false;
          }
          // Return the prefix operator and discard this token.
          tstrm.shift();
          Xprunit::S_operator_rpn xunit = { qconf->xop, false };
          units.emplace_back(rocket::move(xunit));
          return true;
        }
        return false;
      }

    bool do_accept_named_reference(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // Get an identifier.
        auto qname = do_accept_identifier_opt(tstrm);
        if(!qname) {
          return false;
        }
        // Replace special names. This is what macros in C do.
        if(*qname == "__file") {
          Xprunit::S_literal xunit = { G_string(sloc.file()) };
          units.emplace_back(rocket::move(xunit));
          return true;
        }
        if(*qname == "__line") {
          Xprunit::S_literal xunit = { G_integer(sloc.line()) };
          units.emplace_back(rocket::move(xunit));
          return true;
        }
        Xprunit::S_named_reference xunit = { rocket::move(*qname) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_literal(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Get a literal as a `Value`.
        auto qval = do_accept_literal_value_opt(tstrm);
        if(!qval) {
          return false;
        }
        Xprunit::S_literal xunit = { rocket::move(*qval) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_this(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Get the keyword `this`.
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_this });
        if(!qkwrd) {
          return false;
        }
        Xprunit::S_named_reference xunit = { rocket::sref("__this") };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    opt<cow_vector<Statement>> do_accept_closure_body_opt(Token_Stream& tstrm)
      {
        // closure-body ::=
        //   block | equal-initializer
        auto qblock = do_accept_block_opt(tstrm);
        if(qblock) {
          return qblock;
        }
        auto qinit = do_accept_equal_initializer_opt(tstrm);
        if(qinit) {
          // In the case of an `equal-initializer`, behave as if it was a `return-statement`.
          // Note that the result is returned by value.
          Statement::S_return xstmt = { false, rocket::move(*qinit) };
          qblock.emplace().emplace_back(rocket::move(xstmt));
          return qblock;
        }
        return qblock;
      }

    bool do_accept_closure_function(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // closure-function ::=
        //   "func" parameter-declaration closure-body
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_func });
        if(!qkwrd) {
          return false;
        }
        auto kparams = do_accept_parameter_list_declaration_opt(tstrm);
        if(!kparams) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        auto qbody = do_accept_closure_body_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, parser_status_open_brace_or_equal_initializer_expected);
        }
        Xprunit::S_closure_function xunit = { rocket::move(sloc), rocket::move(*kparams), rocket::move(*qbody) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_unnamed_array(cow_vector<Xprunit>& units, Token_Stream& tstrm)
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
        for(;;) {
          if(nelems >= INT32_MAX) {
            throw do_make_parser_error(tstrm, parser_status_too_many_array_elements);
          }
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            break;
          }
          nelems += 1;
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_bracket_expected);
        }
        Xprunit::S_unnamed_array xunit = { nelems };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_unnamed_object(cow_vector<Xprunit>& units, Token_Stream& tstrm)
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
        for(;;) {
          auto duplicate_key_error = do_make_parser_error(tstrm, parser_status_duplicate_object_key);
          auto qkey = do_accept_json5_key_opt(tstrm);
          if(!qkey) {
            break;
          }
          if(rocket::any_of(keys, [&](const phsh_string& r) { return r == *qkey;  })) {
            throw duplicate_key_error;
          }
          // Look for the initializer.
          kpunct = do_accept_punctuator_opt(tstrm, { punctuator_assign, punctuator_colon });
          if(!kpunct) {
            throw do_make_parser_error(tstrm, parser_status_equals_sign_or_colon_expected);
          }
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            throw do_make_parser_error(tstrm, parser_status_expression_expected);
          }
          keys.emplace_back(rocket::move(*qkey));
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_brace_expected);
        }
        Xprunit::S_unnamed_object xunit = { rocket::move(keys) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_nested_expression(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // nested-expression ::=
        //   "(" expression ")"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          return false;
        }
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        return true;
      }

    bool do_accept_fused_multiply_add(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // fused-multiply-add ::=
        //   "__fma" "(" expression "," expression "," expression ")"
        auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_fma });
        if(!qkwrd) {
          return false;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_open_parenthesis_expected);
        }
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_comma_expected);
        }
        succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_comma_expected);
        }
        succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_expected);
        }
        Xprunit::S_operator_fma xunit = { false };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_primary_expression(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // primary-expression ::=
        //   identifier | literal | "this" | closure-function | unnamed-array | unnamed-object | nested-expression |
        //   fused-multiply-add
        bool succ = do_accept_named_reference(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_literal(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_this(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_closure_function(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_unnamed_array(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_unnamed_object(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_nested_expression(units, tstrm);
        if(succ) {
          return succ;
        }
        succ = do_accept_fused_multiply_add(units, tstrm);
        if(succ) {
          return succ;
        }
        return succ;
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
      };

    constexpr bool operator==(const Postfix_Operator_Element& lhs, Punctuator rhs) noexcept
      {
        return lhs.punct == rhs;
      }

    bool do_accept_postfix_operator(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-operator ::=
        //   "++" | "--"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        if(qtok->is_punctuator()) {
          auto qconf = std::find(std::begin(s_postfix_operator_table), std::end(s_postfix_operator_table), qtok->as_punctuator());
          if(qconf == std::end(s_postfix_operator_table)) {
            return false;
          }
          // Return the postfix operator and discard this token.
          tstrm.shift();
          Xprunit::S_operator_rpn xunit = { qconf->xop, false };
          units.emplace_back(rocket::move(xunit));
          return true;
        }
        return false;
      }

    bool do_accept_postfix_function_call(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // postfix-function-call ::=
        //   "(" argument-list-opt ")"
        // argument-list ::=
        //   argument ( "," argument-list | "" )
        // argument ::=
        //   reference-specifier-opt expression
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_op });
        if(!kpunct) {
          return false;
        }
        cow_vector<bool> args_by_refs;
        for(;;) {
          auto qref = do_accept_reference_specifier_opt(tstrm);
          if(!qref) {
            qref.emplace();
          }
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            if(args_by_refs.empty()) {
              break;
            }
            throw do_make_parser_error(tstrm, parser_status_expression_expected);
          }
          args_by_refs.emplace_back(*qref);
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_parenthesis_or_argument_expected);
        }
        Xprunit::S_function_call xunit = { rocket::move(sloc), rocket::move(args_by_refs) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_postfix_subscript(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-subscript ::=
        //   "[" expression "]"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op });
        if(!kpunct) {
          return false;
        }
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_closed_bracket_expected);
        }
        Xprunit::S_operator_rpn xunit = { xop_subscr, false };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_postfix_member_access(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-member-access ::=
        //   "." ( string-literal | identifier )
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_dot });
        if(!kpunct) {
          return false;
        }
        auto qkey = do_accept_json5_key_opt(tstrm);
        if(!qkey) {
          throw do_make_parser_error(tstrm, parser_status_identifier_expected);
        }
        Xprunit::S_member_access xunit = { rocket::move(*qkey) };
        units.emplace_back(rocket::move(xunit));
        return true;
      }

    bool do_accept_infix_element(cow_vector<Xprunit>& units, Token_Stream& tstrm)
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
        cow_vector<Xprunit> prefixes;
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
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        do {
          succ = do_accept_postfix_operator(units, tstrm) ||
                 do_accept_postfix_function_call(units, tstrm) ||
                 do_accept_postfix_subscript(units, tstrm) ||
                 do_accept_postfix_member_access(units, tstrm);
        } while(succ);
        // Append prefixes in reverse order.
        // N.B. Prefix operators have lower precedence than postfix ones.
        std::move(prefixes.mut_rbegin(), prefixes.mut_rend(), std::back_inserter(units));
        return true;
      }

    opt<Infix_Element> do_accept_infix_head_opt(Token_Stream& tstrm)
      {
        // infix-head ::=
        //   infix-element
        cow_vector<Xprunit> units;
        bool succ = do_accept_infix_element(units, tstrm);
        if(!succ) {
          return rocket::nullopt;
        }
        Infix_Element::S_head xelem = { rocket::move(units) };
        return rocket::move(xelem);
      }

    opt<Infix_Element> do_accept_infix_operator_ternary_opt(Token_Stream& tstrm)
      {
        // infix-operator-ternary ::=
        //   ( "?" | "?=" ) expression ":"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_quest_eq });
        if(!kpunct) {
          return rocket::nullopt;
        }
        auto qbtrue = do_accept_expression_opt(tstrm);
        if(!qbtrue) {
          throw do_make_parser_error(tstrm, parser_status_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { punctuator_quest, punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, parser_status_colon_expected);
        }
        Infix_Element::S_ternary xelem = { *kpunct == punctuator_quest_eq, rocket::move(*qbtrue), rocket::clear };
        return rocket::move(xelem);
      }

    opt<Infix_Element> do_accept_infix_operator_logical_and_opt(Token_Stream& tstrm)
      {
        // infix-operator-logical-and ::=
        //   "&&" | "&&=" | "and"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_andl, punctuator_andl_eq });
        if(!kpunct) {
          auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_and });
          if(!qkwrd) {
            return rocket::nullopt;
          }
          kpunct.emplace(punctuator_andl);
        }
        Infix_Element::S_logical_and xelem = { *kpunct == punctuator_andl_eq, rocket::clear };
        return rocket::move(xelem);
      }

    opt<Infix_Element> do_accept_infix_operator_logical_or_opt(Token_Stream& tstrm)
      {
        // infix-operator-logical-or ::=
        //   "||" | "||=" | "or"
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_orl, punctuator_orl_eq });
        if(!kpunct) {
          auto qkwrd = do_accept_keyword_opt(tstrm, { keyword_or });
          if(!qkwrd) {
            return rocket::nullopt;
          }
          kpunct.emplace(punctuator_orl);
        }
        Infix_Element::S_logical_or xelem = { *kpunct == punctuator_orl_eq, rocket::clear };
        return rocket::move(xelem);
      }

    opt<Infix_Element> do_accept_infix_operator_coalescence_opt(Token_Stream& tstrm)
      {
        // infix-operator-coalescence ::=
        //   "??" | "??="
        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_coales, punctuator_coales_eq });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Infix_Element::S_coalescence xelem = { *kpunct == punctuator_coales_eq, rocket::clear };
        return rocket::move(xelem);
      }

    struct Infix_Operator_Element
      {
        Punctuator punct;
        Xop xop;
        bool assign;
      }
    constexpr s_infix_operator_table[] =
      {
        { punctuator_add,        xop_add,       false  },
        { punctuator_sub,        xop_sub,       false  },
        { punctuator_mul,        xop_mul,       false  },
        { punctuator_div,        xop_div,       false  },
        { punctuator_mod,        xop_mod,       false  },
        { punctuator_andb,       xop_andb,      false  },
        { punctuator_orb,        xop_orb,       false  },
        { punctuator_xorb,       xop_xorb,      false  },
        { punctuator_sla,        xop_sla,       false  },
        { punctuator_sra,        xop_sra,       false  },
        { punctuator_sll,        xop_sll,       false  },
        { punctuator_srl,        xop_srl,       false  },
        { punctuator_add_eq,     xop_add,       true   },
        { punctuator_sub_eq,     xop_sub,       true   },
        { punctuator_mul_eq,     xop_mul,       true   },
        { punctuator_div_eq,     xop_div,       true   },
        { punctuator_mod_eq,     xop_mod,       true   },
        { punctuator_andb_eq,    xop_andb,      true   },
        { punctuator_orb_eq,     xop_orb,       true   },
        { punctuator_xorb_eq,    xop_xorb,      true   },
        { punctuator_sla_eq,     xop_sla,       true   },
        { punctuator_sra_eq,     xop_sra,       true   },
        { punctuator_sll_eq,     xop_sll,       true   },
        { punctuator_srl_eq,     xop_srl,       true   },
        { punctuator_assign,     xop_assign,    true   },
        { punctuator_cmp_eq,     xop_cmp_eq,    false  },
        { punctuator_cmp_ne,     xop_cmp_ne,    false  },
        { punctuator_cmp_lt,     xop_cmp_lt,    false  },
        { punctuator_cmp_gt,     xop_cmp_gt,    false  },
        { punctuator_cmp_lte,    xop_cmp_lte,   false  },
        { punctuator_cmp_gte,    xop_cmp_gte,   false  },
        { punctuator_spaceship,  xop_cmp_3way,  false  },
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
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_punctuator()) {
          auto qconf = std::find(std::begin(s_infix_operator_table), std::end(s_infix_operator_table), qtok->as_punctuator());
          if(qconf == std::end(s_infix_operator_table)) {
            return rocket::nullopt;
          }
          // Return the infix operator and discard this token.
          tstrm.shift();
          Infix_Element::S_general xelem = { qconf->xop, qconf->assign, rocket::clear };
          return rocket::move(xelem);
        }
        return rocket::nullopt;
      }

    opt<Infix_Element> do_accept_infix_operator_opt(Token_Stream& tstrm)
      {
        // infix-operator ::=
        //   infix-operator-ternary | infix-operator-logical-and | infix-operator-logical-or |
        //   infix-operator-coalescence | infix-operator-general
        auto qelem = do_accept_infix_operator_ternary_opt(tstrm);
        if(qelem) {
          return qelem;
        }
        qelem = do_accept_infix_operator_logical_and_opt(tstrm);
        if(qelem) {
          return qelem;
        }
        qelem = do_accept_infix_operator_logical_or_opt(tstrm);
        if(qelem) {
          return qelem;
        }
        qelem = do_accept_infix_operator_coalescence_opt(tstrm);
        if(qelem) {
          return qelem;
        }
        qelem = do_accept_infix_operator_general_opt(tstrm);
        if(qelem) {
          return qelem;
        }
        return qelem;
      }

    bool do_accept_expression(cow_vector<Xprunit>& units, Token_Stream& tstrm)
      {
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
        stack.emplace_back(rocket::move(*qelem));
        for(;;) {
          auto qnext = do_accept_infix_operator_opt(tstrm);
          if(!qnext) {
            break;
          }
          bool succ = do_accept_infix_element(qnext->open_junction(), tstrm);
          if(!succ) {
            throw do_make_parser_error(tstrm, parser_status_expression_expected);
          }
          // Assignment operations have the lowest precedence and group from right to left.
          if(stack.back().tell_precedence() < precedence_assignment) {
            // Collapse elements that have no lower precedence and group from left to right.
            auto preced_next = qnext->tell_precedence();
            while((stack.size() >= 2) && (stack.back().tell_precedence() <= preced_next)) {
              qelem = rocket::move(stack.mut_back());
              stack.pop_back();
              qelem->extract(stack.mut_back().open_junction());
            }
          }
          stack.emplace_back(rocket::move(*qnext));
        }
        while(stack.size() >= 2) {
          qelem = rocket::move(stack.mut_back());
          stack.pop_back();
          qelem->extract(stack.mut_back().open_junction());
        }
        stack.mut_back().extract(units);
        return true;
      }

    }  // namespace

bool Parser::load(Token_Stream& tstrm, const Compiler_Options& /*options*/)
  {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor = nullptr;
    // document ::=
    //   statement-list-opt
    cow_vector<Statement> stmts;
    try {
      // Parse the document recursively.
      for(;;) {
        auto qstmt = do_accept_statement_opt(tstrm);
        if(!qstmt) {
          break;
        }
        stmts.emplace_back(rocket::move(*qstmt));
      }
      auto qtok = tstrm.peek_opt();
      if(qtok) {
        ASTERIA_DEBUG_LOG("Excess token: ", *qtok);
        throw do_make_parser_error(tstrm, parser_status_statement_expected);
      }
    }
    catch(const Parser_Error& error) {  // `Parser_Error` is not derived from `std::exception`. Don't play with this at home.
      ASTERIA_DEBUG_LOG("Caught `Parser_Error`: ", error);
      this->m_stor = error;
      return false;
    }
    this->m_stor = rocket::move(stmts);
    return true;
  }

Parser_Error Parser::get_parser_error() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return parser_status_no_data_loaded;
      }
    case state_error:
      {
        return this->m_stor.as<Parser_Error>();
      }
    case state_success:
      {
        return parser_status_success;
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

const cow_vector<Statement>& Parser::get_statements() const
  {
    switch(this->state()) {
    case state_empty:
      {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
    case state_error:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
    case state_success:
      {
        return this->m_stor.as<cow_vector<Statement>>();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
