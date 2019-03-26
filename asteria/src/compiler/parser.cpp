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

    Parser_Error do_make_parser_error(const Token_Stream& tstrm, Parser_Error::Code code)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return Parser_Error(UINT32_MAX, SIZE_MAX, 0, code);
        }
        return Parser_Error(qtok->line(), qtok->offset(), qtok->length(), code);
      }

    Source_Location do_tell_source_location(const Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return Source_Location(rocket::sref("<end of stream>"), UINT32_MAX);
        }
        return Source_Location(qtok->file(), qtok->line());
      }

    Optional<Token::Keyword> do_accept_keyword_opt(Token_Stream& tstrm, std::initializer_list<Token::Keyword> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is one of acceptable keywords.
        auto qalt = qtok->opt<Token::S_keyword>();
        if(!qalt) {
          return rocket::nullopt;
        }
        if(rocket::is_none_of(qalt->keyword, accept)) {
          return rocket::nullopt;
        }
        // Return the keyword and discard this token.
        auto keyword = qalt->keyword;
        tstrm.shift();
        return keyword;
      }

    Optional<Token::Punctuator> do_accept_punctuator_opt(Token_Stream& tstrm, std::initializer_list<Token::Punctuator> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is one of acceptable punctuator.
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return rocket::nullopt;
        }
        if(rocket::is_none_of(qalt->punct, accept)) {
          return rocket::nullopt;
        }
        // Return the punctuator and discard this token.
        auto punct = qalt->punct;
        tstrm.shift();
        return punct;
      }

    Optional<Cow_String> do_accept_identifier_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is an identifier.
        auto qalt = qtok->opt<Token::S_identifier>();
        if(!qalt) {
          return rocket::nullopt;
        }
        // Return the identifier and discard this token.
        auto name = qalt->name;
        tstrm.shift();
        return name;
      }

    Optional<Cow_String> do_accept_string_literal_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        // See whether it is an string literal.
        auto qalt = qtok->opt<Token::S_string_literal>();
        if(!qalt) {
          return rocket::nullopt;
        }
        // Return the string literal and discard this token.
        auto value = qalt->value;
        tstrm.shift();
        return value;
      }

    Optional<Cow_String> do_accept_json5_key_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        Optional<Cow_String> qname;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            // Treat the keyword as a plain identifier.
            qname = rocket::sref(Token::get_keyword(alt.keyword));
            break;
          }
        case Token::index_identifier:
          {
            const auto& alt = qtok->check<Token::S_identifier>();
            // Return the identifier as is.
            qname = alt.name;
            break;
          }
        case Token::index_string_literal:
          {
            const auto& alt = qtok->check<Token::S_string_literal>();
            // Return the string literal as is.
            qname = alt.value;
            break;
          }
        default:
          return rocket::nullopt;
        }
        // Discard this token.
        tstrm.shift();
        return qname;
      }

    Optional<Value> do_accept_literal_value_opt(Token_Stream& tstrm)
      {
        // literal ::=
        //   null-literal | boolean-literal | string-literal | noescape-string-literal |
        //   numeric-literal | nan-literal | infinity-literal
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
        Optional<Value> qvalue;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            // Hmm... use a lookup table?
            struct Keyword_Table
              {
                Token::Keyword keyword;
                void (*setter)(Optional<Value>&);
              }
            static const s_table[] =
              {
                { Token::keyword_null,      [](Optional<Value>& r) { r = D_null();  }          },
                { Token::keyword_false,     [](Optional<Value>& r) { r = D_boolean(false);  }  },
                { Token::keyword_true,      [](Optional<Value>& r) { r = D_boolean(true);  }   },
                { Token::keyword_nan,       [](Optional<Value>& r) { r = D_real(NAN);  }       },
                { Token::keyword_infinity,  [](Optional<Value>& r) { r = D_real(INFINITY);  }  },
              };
            auto qconf = std::find_if(std::begin(s_table), std::end(s_table),
                                      [&](const Keyword_Table& r) { return alt.keyword == r.keyword;  });
            if(qconf == std::end(s_table)) {
              return rocket::nullopt;
            }
            // Set the value.
            (*(qconf->setter))(qvalue);
            break;
          }
        case Token::index_integer_literal:
          {
            const auto& alt = qtok->check<Token::S_integer_literal>();
            // Copy the value as is.
            qvalue = D_integer(alt.value);
            break;
          }
        case Token::index_real_literal:
          {
            const auto& alt = qtok->check<Token::S_real_literal>();
            // Copy the value as is.
            qvalue = D_real(alt.value);
            break;
          }
        case Token::index_string_literal:
          {
            const auto& alt = qtok->check<Token::S_string_literal>();
            // Copy the value as is.
            qvalue = D_string(alt.value);
            break;
          }
        default:
          return rocket::nullopt;
        }
        // Discard this token.
        tstrm.shift();
        return qvalue;
      }

    Optional<bool> do_accept_negation_opt(Token_Stream& tstrm)
      {
        // negation ::=
        //   "!" | "not"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        Optional<bool> kneg;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            if(alt.keyword != Token::keyword_not) {
              return rocket::nullopt;
            }
            kneg = true;
            break;
          }
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            if(alt.punct != Token::punctuator_notl) {
              return rocket::nullopt;
            }
            kneg = true;
            break;
          }
        default:
          return rocket::nullopt;
        }
        // Discard this token.
        tstrm.shift();
        return kneg;
      }

    // Accept a statement; a blockt is converted to a single statement.
    extern Optional<Statement> do_accept_statement_opt(Token_Stream& tstrm);
    // Accept a statement; a non-block statement is converted to a block consisting of a single statement.
    extern Optional<Cow_Vector<Statement>> do_accept_statement_as_block_opt(Token_Stream& tstrm);

    extern bool do_accept_expression(Cow_Vector<Xprunit>& units, Token_Stream& tstrm);

    Optional<Cow_Vector<Xprunit>> do_accept_expression_opt(Token_Stream& tstrm)
      {
        // expression-opt ::=
        //   expression | ""
        Cow_Vector<Xprunit> units;
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          return rocket::nullopt;
        }
        return rocket::move(units);
      }

    Optional<Cow_Vector<Statement>> do_accept_block_opt(Token_Stream& tstrm)
      {
        // block ::=
        //   "{" statement-list-opt "}"
        // statement-list-opt ::=
        //   statement-list | ""
        // statement-list ::=
        //   statement statement-list-opt
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Cow_Vector<Statement> body;
        for(;;) {
          auto qstmt = do_accept_statement_opt(tstrm);
          if(!qstmt) {
            break;
          }
          body.emplace_back(rocket::move(*qstmt));
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_statement_expected);
        }
        return rocket::move(body);
      }

    Optional<Statement> do_accept_block_statement_opt(Token_Stream& tstrm)
      {
        auto qbody = do_accept_block_opt(tstrm);
        if(!qbody) {
          return rocket::nullopt;
        }
        Statement::S_block stmt_c = { rocket::move(*qbody) };
        return rocket::move(stmt_c);
      }

    Optional<Cow_Vector<Xprunit>> do_accept_equal_initializer_opt(Token_Stream& tstrm)
      {
        // equal-initializer-opt ::=
        //   equal-initializer | ""
        // equal-initializer ::=
        //   "=" expression
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_assign });
        if(!kpunct) {
          return rocket::nullopt;
        }
        return do_accept_expression_opt(tstrm);
      }

    Optional<Statement> do_accept_null_statement_opt(Token_Stream& tstrm)
      {
        // null-statement ::=
        //   ";"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Statement::S_expression stmt_c = { rocket::clear };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_variable_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // variable-definition ::=
        //   "var" identifier equal-initailizer-opt ( "," identifier equal-initializer-opt | "" ) ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_var });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        Cow_Vector<std::pair<PreHashed_String, Cow_Vector<Xprunit>>> vars;
        for(;;) {
          auto qname = do_accept_identifier_opt(tstrm);
          if(!qname) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          auto qinit = do_accept_equal_initializer_opt(tstrm);
          if(!qinit) {
            qinit.emplace();
          }
          vars.emplace_back(rocket::move(*qname), rocket::move(*qinit));
          // Look for the separator. The first declaration is required.
          auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { rocket::move(sloc), false, rocket::move(vars) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_immutable_variable_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // immutable-variable-definition ::=
        //   "const" identifier equal-initailizer ( "," identifier equal-initializer | "" ) ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_const });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        Cow_Vector<std::pair<PreHashed_String, Cow_Vector<Xprunit>>> vars;
        for(;;) {
          auto qname = do_accept_identifier_opt(tstrm);
          if(!qname) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          auto qinit = do_accept_equal_initializer_opt(tstrm);
          if(!qinit) {
            throw do_make_parser_error(tstrm, Parser_Error::code_equals_sign_expected);
          }
          vars.emplace_back(rocket::move(*qname), rocket::move(*qinit));
          // Look for the separator. The first declaration is required.
          auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { rocket::move(sloc), true, rocket::move(vars) };
        return rocket::move(stmt_c);
      }

    Optional<Cow_Vector<PreHashed_String>> do_accept_parameter_list_opt(Token_Stream& tstrm)
      {
        // parameter-list ::=
        //   "(" ( identifier-list | "" ) ")"
        // identifier-list ::=
        //   identifier ( "," identifier-list | "" )
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Cow_Vector<PreHashed_String> names;
        for(;;) {
          auto qname = do_accept_identifier_opt(tstrm);
          if(!qname) {
            if(names.empty()) {
              break;
            }
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          names.emplace_back(rocket::move(*qname));
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_or_parameter_expected);
        }
        return rocket::move(names);
      }

    Optional<Statement> do_accept_function_definition_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // function-definition ::=
        //   "func" identifier parameter-list block
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_func });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qname = do_accept_identifier_opt(tstrm);
        if(!qname) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        auto kparams = do_accept_parameter_list_opt(tstrm);
        if(!kparams) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qbody = do_accept_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_brace_expected);
        }
        Statement::S_function stmt_c = { rocket::move(sloc), rocket::move(*qname), rocket::move(*kparams), rocket::move(*qbody) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_expression_statement_opt(Token_Stream& tstrm)
      {
        // expression-statement ::=
        //   expression ";"
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_expression stmt_c = { rocket::move(*kexpr) };
        return rocket::move(stmt_c);
      }

    Optional<Cow_Vector<Statement>> do_accept_else_branch_opt(Token_Stream& tstrm)
      {
        // else-branch-opt ::=
        //   "else" statement | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_else });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        return rocket::move(*qbody);
      }

    Optional<Statement> do_accept_if_statement_opt(Token_Stream& tstrm)
      {
        // if-statement ::=
        //   "if" negation-opt "(" expression ")" statement else-branch-opt
        // negation-opt ::=
        //   negation | ""
        // negation ::=
        //   "!" | "not"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_if });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qbtrue = do_accept_statement_as_block_opt(tstrm);
        if(!qbtrue) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        auto qbfalse = do_accept_else_branch_opt(tstrm);
        if(!qbfalse) {
          qbfalse.emplace();
        }
        Statement::S_if stmt_c = { *kneg, rocket::move(*qcond), rocket::move(*qbtrue), rocket::move(*qbfalse) };
        return rocket::move(stmt_c);
      }

    Optional<std::pair<Cow_Vector<Xprunit>, Cow_Vector<Statement>>> do_accept_case_clause_opt(Token_Stream& tstrm)
      {
        // case-clause ::=
        //   "case" expression ":" statement-list-opt
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_case });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
        }
        auto qclause = do_accept_statement_as_block_opt(tstrm);
        if(!qclause) {
          qclause.emplace();
        }
        return std::make_pair(rocket::move(*qcond), rocket::move(*qclause));
      }

    Optional<std::pair<Cow_Vector<Xprunit>, Cow_Vector<Statement>>> do_accept_default_clause_opt(Token_Stream& tstrm)
      {
        // default-clause ::=
        //   "default" ":" statement-list-opt
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_default });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
        }
        auto qclause = do_accept_statement_as_block_opt(tstrm);
        if(!qclause) {
          qclause.emplace();
        }
        return std::make_pair(rocket::clear, rocket::move(*qclause));
      }

    Optional<std::pair<Cow_Vector<Xprunit>, Cow_Vector<Statement>>> do_accept_switch_clause_opt(Token_Stream& tstrm)
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

    Optional<Cow_Vector<std::pair<Cow_Vector<Xprunit>, Cow_Vector<Statement>>>> do_accept_switch_clause_list_opt(Token_Stream& tstrm)
      {
        // switch-clause-list ::=
        //   switch-clause switch-clause-list-opt
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_op });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Cow_Vector<std::pair<Cow_Vector<Xprunit>, Cow_Vector<Statement>>> clauses;
        for(;;) {
          auto qclause = do_accept_switch_clause_opt(tstrm);
          if(!qclause) {
            break;
          }
          clauses.emplace_back(rocket::move(*qclause));
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_switch_clause_expected);
        }
        return rocket::move(clauses);
      }

    Optional<Statement> do_accept_switch_statement_opt(Token_Stream& tstrm)
      {
        // switch-statement ::=
        //   "switch" "(" expression ")" switch-block
        // switch-block ::=
        //   "{" swtich-clause-list-opt "}"
        // switch-clause-list-opt ::=
        //   switch-clause-list | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_switch });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qctrl = do_accept_expression_opt(tstrm);
        if(!qctrl) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qclauses = do_accept_switch_clause_list_opt(tstrm);
        if(!qclauses) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_brace_expected);
        }
        Statement::S_switch stmt_c = { rocket::move(*qctrl), rocket::move(*qclauses) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_do_while_statement_opt(Token_Stream& tstrm)
      {
        // do-while-statement ::=
        //   "do" statement "while" negation-opt "(" expression ")" ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_do });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_while });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Statement::S_do_while stmt_c = { rocket::move(*qbody), *kneg, rocket::move(*qcond) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_while_statement_opt(Token_Stream& tstrm)
      {
        // while-statement ::=
        //   "while" negation-opt "(" expression ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_while });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qcond = do_accept_expression_opt(tstrm);
        if(!qcond) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_while stmt_c = { *kneg, rocket::move(*qcond), rocket::move(*qbody) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_for_complement_range_opt(Token_Stream& tstrm)
      {
        // for-complement-range ::=
        //   "each" identifier "," identifier ":" expression ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_each });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qkname = do_accept_identifier_opt(tstrm);
        if(!qkname) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_comma_expected);
        }
        auto qmname = do_accept_identifier_opt(tstrm);
        if(!qmname) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
        }
        auto qinit = do_accept_expression_opt(tstrm);
        if(!qinit) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_for_each stmt_c = { rocket::move(*qkname), rocket::move(*qmname), rocket::move(*qinit), rocket::move(*qbody) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_for_initializer_opt(Token_Stream& tstrm)
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

    Optional<Statement> do_accept_for_complement_triplet_opt(Token_Stream& tstrm)
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
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        auto kstep = do_accept_expression_opt(tstrm);
        if(!kstep) {
          kstep.emplace();
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qbody = do_accept_statement_as_block_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Cow_Vector<Statement> rvinit;
        rvinit.emplace_back(rocket::move(*qinit));
        Statement::S_for stmt_c = { rocket::move(rvinit), rocket::move(*qcond), rocket::move(*kstep), rocket::move(*qbody) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_for_complement_opt(Token_Stream& tstrm)
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

    Optional<Statement> do_accept_for_statement_opt(Token_Stream& tstrm)
      {
        // for-statement ::=
        //   "for" "(" for-complement
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_for });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qcompl = do_accept_for_complement_opt(tstrm);
        if(!qcompl) {
          throw do_make_parser_error(tstrm, Parser_Error::code_for_statement_initializer_expected);
        }
        return rocket::move(*qcompl);
      }

    Optional<Statement::Target> do_accept_break_target_opt(Token_Stream& tstrm)
      {
        // break-target-opt ::=
        //   "switch" | "while" | "for" | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_switch, Token::keyword_while, Token::keyword_for });
        if(qkwrd == Token::keyword_switch) {
          return Statement::target_switch;
        }
        if(qkwrd == Token::keyword_while) {
          return Statement::target_while;
        }
        if(qkwrd == Token::keyword_for) {
          return Statement::target_for;
        }
        return rocket::nullopt;
      }

    Optional<Statement> do_accept_break_statement_opt(Token_Stream& tstrm)
      {
        // break-statement ::=
        //   "break" break-target-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_break });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qtarget = do_accept_break_target_opt(tstrm);
        if(!qtarget) {
          qtarget.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_break stmt_c = { *qtarget };
        return rocket::move(stmt_c);
      }

    Optional<Statement::Target> do_accept_continue_target_opt(Token_Stream& tstrm)
      {
        // continue-target-opt ::=
        //   "while" | "for" | ""
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_while, Token::keyword_for });
        if(qkwrd == Token::keyword_while) {
          return Statement::target_while;
        }
        if(qkwrd == Token::keyword_for) {
          return Statement::target_for;
        }
        return rocket::nullopt;
      }

    Optional<Statement> do_accept_continue_statement_opt(Token_Stream& tstrm)
      {
        // continue-statement ::=
        //   "continue" continue-target-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_continue });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qtarget = do_accept_continue_target_opt(tstrm);
        if(!qtarget) {
          qtarget.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_continue stmt_c = { *qtarget };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_throw_statement_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // throw-statement ::=
        //   "throw" expression ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_throw });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_throw stmt_c = { rocket::move(sloc), rocket::move(*kexpr) };
        return rocket::move(stmt_c);
      }

    Optional<bool> do_accept_reference_specifier_opt(Token_Stream& tstrm)
      {
        // reference-specifier-opt ::=
        //   "&" | ""
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_andb });
        if(!kpunct) {
          return rocket::nullopt;
        }
        return true;
      }

    Optional<Statement> do_accept_return_statement_opt(Token_Stream& tstrm)
      {
        // return-statement ::=
        //   "return" reference-specifier-opt expression-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_return });
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
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_return stmt_c = { *qref, rocket::move(*kexpr) };
        return rocket::move(stmt_c);
      }

    Optional<Cow_String> do_accept_assert_message_opt(Token_Stream& tstrm)
      {
        // assert-message ::=
        //   ":" string-literal | ""
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
        if(!kpunct) {
          return rocket::nullopt;
        }
        auto kmsg = do_accept_string_literal_opt(tstrm);
        if(!kmsg) {
          throw do_make_parser_error(tstrm, Parser_Error::code_string_literal_expected);
        }
        return rocket::move(*kmsg);
      }

    Optional<Statement> do_accept_assert_statement_opt(Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // assert-statement ::=
        //   "assert" negation-opt expression assert-message-opt ";"
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_assert });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto kneg = do_accept_negation_opt(tstrm);
        if(!kneg) {
          kneg.emplace();
        }
        auto kexpr = do_accept_expression_opt(tstrm);
        if(!kexpr) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        auto kmsg = do_accept_assert_message_opt(tstrm);
        if(!kmsg) {
          kmsg.emplace();
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_semicol });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_assert stmt_c = { rocket::move(sloc), *kneg, rocket::move(*kexpr), rocket::move(*kmsg) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_try_statement_opt(Token_Stream& tstrm)
      {
        // try-statement ::=
        //   "try" statement "catch" "(" identifier ")" statement
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_try });
        if(!qkwrd) {
          return rocket::nullopt;
        }
        auto qbtry = do_accept_statement_as_block_opt(tstrm);
        if(!qbtry) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_catch });
        if(!qkwrd) {
          throw do_make_parser_error(tstrm, Parser_Error::code_keyword_catch_expected);
        }
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto kexcept = do_accept_identifier_opt(tstrm);
        if(!kexcept) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        auto qbcatch = do_accept_statement_as_block_opt(tstrm);
        if(!qbcatch) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_try stmt_c = { rocket::move(*qbtry), rocket::move(*kexcept), rocket::move(*qbcatch) };
        return rocket::move(stmt_c);
      }

    Optional<Statement> do_accept_nonblock_statement_opt(Token_Stream& tstrm)
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

    Optional<Statement> do_accept_statement_opt(Token_Stream& tstrm)
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

    Optional<Cow_Vector<Statement>> do_accept_statement_as_block_opt(Token_Stream& tstrm)
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

    bool do_accept_prefix_operator(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // prefix-operator ::=
        //   "+" | "-" | "~" | "!" | "++" | "--" | "unset" | "lengthof" | "typeof" | "not"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        Optional<Xprunit> qunit;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            // Hmm... use a lookup table?
            struct Keyword_Table
              {
                Token::Keyword keyword;
                Xprunit::Xop xop;
              }
            static constexpr s_table[] =
              {
                { Token::keyword_unset,     Xprunit::xop_prefix_unset    },
                { Token::keyword_lengthof,  Xprunit::xop_prefix_lengthof },
                { Token::keyword_typeof,    Xprunit::xop_prefix_typeof   },
                { Token::keyword_not,       Xprunit::xop_prefix_notl     },
              };
            auto qconf = std::find_if(std::begin(s_table), std::end(s_table),
                                      [&](const Keyword_Table& r) { return alt.keyword == r.keyword;  });
            if(qconf == std::end(s_table)) {
              return false;
            }
            // Return the prefix operator.
            Xprunit::S_operator_rpn unit_c = { qconf->xop, false };
            units.emplace_back(rocket::move(unit_c));
            break;
          }
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            // Hmm... use a lookup table?
            struct Punctuator_Table
              {
                Token::Punctuator punct;
                Xprunit::Xop xop;
              }
            static constexpr s_table[] =
              {
                { Token::punctuator_add,   Xprunit::xop_prefix_pos  },
                { Token::punctuator_sub,   Xprunit::xop_prefix_neg  },
                { Token::punctuator_notb,  Xprunit::xop_prefix_notb },
                { Token::punctuator_notl,  Xprunit::xop_prefix_notl },
                { Token::punctuator_inc,   Xprunit::xop_prefix_inc  },
                { Token::punctuator_dec,   Xprunit::xop_prefix_dec  },
              };
            auto qconf = std::find_if(std::begin(s_table), std::end(s_table),
                                      [&](const Punctuator_Table& r) { return alt.punct == r.punct;  });
            if(qconf == std::end(s_table)) {
              return false;
            }
            // Return the prefix operator.
            Xprunit::S_operator_rpn unit_c = { qconf->xop, false };
            units.emplace_back(rocket::move(unit_c));
            break;
          }
        default:
          return false;
        }
        // Discard this token.
        tstrm.shift();
        return true;
      }

    bool do_accept_named_reference(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
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
          Xprunit::S_literal unit_c = { D_string(sloc.file()) };
          units.emplace_back(rocket::move(unit_c));
          return true;
        }
        if(*qname == "__line") {
          Xprunit::S_literal unit_c = { D_integer(sloc.line()) };
          units.emplace_back(rocket::move(unit_c));
          return true;
        }
        Xprunit::S_named_reference unit_c = { rocket::move(*qname) };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_literal(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Get a literal as a `Value`.
        auto qvalue = do_accept_literal_value_opt(tstrm);
        if(!qvalue) {
          return false;
        }
        Xprunit::S_literal unit_c = { rocket::move(*qvalue) };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_this(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Get the keyword `this`.
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_this });
        if(!qkwrd) {
          return false;
        }
        Xprunit::S_named_reference unit_c = { rocket::sref("__this") };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    Optional<Cow_Vector<Statement>> do_accept_closure_body_opt(Token_Stream& tstrm)
      {
        // closure-body ::=
        //   block | equal-initializer
        auto qblock = do_accept_block_opt(tstrm);
        if(qblock) {
          return qblock;
        }
        auto qinit = do_accept_equal_initializer_opt(tstrm);
        if(qinit) {
          Statement::S_return stmt_c = { true, rocket::move(*qinit) };
          qblock.emplace().emplace_back(rocket::move(stmt_c));
          return qblock;
        }
        return qblock;
      }

    bool do_accept_closure_function(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // closure-function ::=
        //   "func" parameter-list closure-body
        auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_func });
        if(!qkwrd) {
          return false;
        }
        auto kparams = do_accept_parameter_list_opt(tstrm);
        if(!kparams) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        auto qbody = do_accept_closure_body_opt(tstrm);
        if(!qbody) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_brace_or_equal_initializer_expected);
        }
        Xprunit::S_closure_function unit_c = { rocket::move(sloc), rocket::move(*kparams), rocket::move(*qbody) };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_unnamed_array(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // unnamed-array ::=
        //   "[" array-element-list-opt "]"
        // array-element-list-opt ::=
        //   array-element-list | ""
        // array-element-list ::=
        //   expression ( ( "," | ";" ) array-element-list-opt | "" )
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_op });
        if(!kpunct) {
          return false;
        }
        std::size_t nelems = 0;
        for(;;) {
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            break;
          }
          nelems += 1;
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma, Token::punctuator_semicol });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_or_expression_expected);
        }
        Xprunit::S_unnamed_array unit_c = { nelems };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_unnamed_object(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // unnamed-object ::=
        //   "{" key-mapped-list-opt "}"
        // key-mapped-list-opt ::=
        //   key-mapped-list | ""
        // key-mapped-list ::=
        //   ( string-literal | identifier ) ( "=" | ":" ) expression ( ( "," | ";" ) key-mapped-list-opt | "" )
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_op });
        if(!kpunct) {
          return false;
        }
        Cow_Vector<PreHashed_String> keys;
        for(;;) {
          auto duplicate_key_error = do_make_parser_error(tstrm, Parser_Error::code_duplicate_object_key);
          auto qkey = do_accept_json5_key_opt(tstrm);
          if(!qkey) {
            break;
          }
          if(rocket::any_of(keys, [&](const PreHashed_String& r) { return r == *qkey;  })) {
            throw duplicate_key_error;
          }
          // Look for the initializer.
          kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_assign, Token::punctuator_colon });
          if(!kpunct) {
            throw do_make_parser_error(tstrm, Parser_Error::code_equals_sign_or_colon_expected);
          }
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
          keys.emplace_back(rocket::move(*qkey));
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma, Token::punctuator_semicol });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_object_key_expected);
        }
        Xprunit::S_unnamed_object unit_c = { rocket::move(keys) };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_nested_expression(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // nested-expression ::=
        //   "(" expression ")"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          return false;
        }
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        return true;
      }

    bool do_accept_primary_expression(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // primary-expression ::=
        //   identifier | literal | "this" | closure-function | unnamed-array | unnamed-object | nested-expression
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
        return succ;
      }

    bool do_accept_postfix_operator(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-operator ::=
        //   "++" | "--"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        Optional<Xprunit> qunit;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            // Hmm... use a lookup table?
            struct Punctuator_Table
              {
                Token::Punctuator punct;
                Xprunit::Xop xop;
              }
            static constexpr s_table[] =
              {
                { Token::punctuator_inc,   Xprunit::xop_postfix_inc  },
                { Token::punctuator_dec,   Xprunit::xop_postfix_dec  },
              };
            auto qconf = std::find_if(std::begin(s_table), std::end(s_table),
                                      [&](const Punctuator_Table& r) { return alt.punct == r.punct;  });
            if(qconf == std::end(s_table)) {
              return false;
            }
            // Return the postfix operator.
            Xprunit::S_operator_rpn unit_c = { qconf->xop, false };
            units.emplace_back(rocket::move(unit_c));
            break;
          }
        default:
          return false;
        }
        // Discard this token.
        tstrm.shift();
        return true;
      }

    bool do_accept_postfix_function_call(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // postfix-function-call ::=
        //   "(" expression-list-opt ")"
        // expression-list ::=
        //   expression ( "," expression-list | "" )
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_op });
        if(!kpunct) {
          return false;
        }
        std::size_t nargs = 0;
        for(;;) {
          bool succ = do_accept_expression(units, tstrm);
          if(!succ) {
            if(nargs == 0) {
              break;
            }
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
          nargs += 1;
          // Look for the separator.
          kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma });
          if(!kpunct) {
            break;
          }
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_parenth_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_or_argument_expected);
        }
        Xprunit::S_function_call unit_c = { rocket::move(sloc), nargs };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_postfix_subscript(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-subscript ::=
        //   "[" expression "]"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_op });
        if(!kpunct) {
          return false;
        }
        bool succ = do_accept_expression(units, tstrm);
        if(!succ) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
        }
        Xprunit::S_operator_rpn unit_c = { Xprunit::xop_postfix_at, false };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_postfix_member_access(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
      {
        // postfix-member-access ::=
        //   "." ( string-literal | identifier )
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_dot });
        if(!kpunct) {
          return false;
        }
        auto qkey = do_accept_json5_key_opt(tstrm);
        if(!qkey) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        Xprunit::S_member_access unit_c = { rocket::move(*qkey) };
        units.emplace_back(rocket::move(unit_c));
        return true;
      }

    bool do_accept_infix_element(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
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
        Cow_Vector<Xprunit> prefixes;
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
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
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

    Optional<Infix_Element> do_accept_infix_head_opt(Token_Stream& tstrm)
      {
        // infix-head ::=
        //   infix-element
        Cow_Vector<Xprunit> units;
        bool succ = do_accept_infix_element(units, tstrm);
        if(!succ) {
          return rocket::nullopt;
        }
        Infix_Element::S_head elem_c = { rocket::move(units) };
        return rocket::move(elem_c);
      }

    Optional<Infix_Element> do_accept_infix_operator_ternary_opt(Token_Stream& tstrm)
      {
        // infix-operator-ternary ::=
        //   ( "?" | "?=" ) expression ":"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_quest, Token::punctuator_quest_eq });
        if(!kpunct) {
          return rocket::nullopt;
        }
        auto qbtrue = do_accept_expression_opt(tstrm);
        if(!qbtrue) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_quest, Token::punctuator_colon });
        if(!kpunct) {
          throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
        }
        Infix_Element::S_ternary elem_c = { *kpunct == Token::punctuator_quest_eq, rocket::move(*qbtrue), rocket::clear };
        return rocket::move(elem_c);
      }

    Optional<Infix_Element> do_accept_infix_operator_logical_and_opt(Token_Stream& tstrm)
      {
        // infix-operator-logical-and ::=
        //   "&&" | "&&=" | "and"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_andl, Token::punctuator_andl_eq });
        if(!kpunct) {
          auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_and });
          if(!qkwrd) {
            return rocket::nullopt;
          }
          kpunct.emplace(Token::punctuator_andl);
        }
        Infix_Element::S_logical_and elem_c = { *kpunct == Token::punctuator_andl_eq, rocket::clear };
        return rocket::move(elem_c);
      }

    Optional<Infix_Element> do_accept_infix_operator_logical_or_opt(Token_Stream& tstrm)
      {
        // infix-operator-logical-or ::=
        //   "||" | "||=" | "or"
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_orl, Token::punctuator_orl_eq });
        if(!kpunct) {
          auto qkwrd = do_accept_keyword_opt(tstrm, { Token::keyword_or });
          if(!qkwrd) {
            return rocket::nullopt;
          }
          kpunct.emplace(Token::punctuator_orl);
        }
        Infix_Element::S_logical_or elem_c = { *kpunct == Token::punctuator_orl_eq, rocket::clear };
        return rocket::move(elem_c);
      }

    Optional<Infix_Element> do_accept_infix_operator_coalescence_opt(Token_Stream& tstrm)
      {
        // infix-operator-coalescence ::=
        //   "??" | "??="
        auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_coales, Token::punctuator_coales_eq });
        if(!kpunct) {
          return rocket::nullopt;
        }
        Infix_Element::S_coalescence elem_c = { *kpunct == Token::punctuator_coales_eq, rocket::clear };
        return rocket::move(elem_c);
      }

    Optional<Infix_Element> do_accept_infix_operator_general_opt(Token_Stream& tstrm)
      {
        // infix-operator-general ::=
        //   "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
        //   "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
        //   "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        Optional<Infix_Element> qelem;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            // Hmm... use a lookup table?
            struct Punctuator_Table
              {
                Token::Punctuator punct;
                Xprunit::Xop xop;
                bool assign;
              }
            static constexpr s_table[] =
              {
                { Token::punctuator_add,        Xprunit::xop_infix_add,       false },
                { Token::punctuator_sub,        Xprunit::xop_infix_sub,       false },
                { Token::punctuator_mul,        Xprunit::xop_infix_mul,       false },
                { Token::punctuator_div,        Xprunit::xop_infix_div,       false },
                { Token::punctuator_mod,        Xprunit::xop_infix_mod,       false },
                { Token::punctuator_andb,       Xprunit::xop_infix_andb,      false },
                { Token::punctuator_orb,        Xprunit::xop_infix_orb,       false },
                { Token::punctuator_xorb,       Xprunit::xop_infix_xorb,      false },
                { Token::punctuator_sla,        Xprunit::xop_infix_sla,       false },
                { Token::punctuator_sra,        Xprunit::xop_infix_sra,       false },
                { Token::punctuator_sll,        Xprunit::xop_infix_sll,       false },
                { Token::punctuator_srl,        Xprunit::xop_infix_srl,       false },
                { Token::punctuator_add_eq,     Xprunit::xop_infix_add,       true  },
                { Token::punctuator_sub_eq,     Xprunit::xop_infix_sub,       true  },
                { Token::punctuator_mul_eq,     Xprunit::xop_infix_mul,       true  },
                { Token::punctuator_div_eq,     Xprunit::xop_infix_div,       true  },
                { Token::punctuator_mod_eq,     Xprunit::xop_infix_mod,       true  },
                { Token::punctuator_andb_eq,    Xprunit::xop_infix_andb,      true  },
                { Token::punctuator_orb_eq,     Xprunit::xop_infix_orb,       true  },
                { Token::punctuator_xorb_eq,    Xprunit::xop_infix_xorb,      true  },
                { Token::punctuator_sla_eq,     Xprunit::xop_infix_sla,       true  },
                { Token::punctuator_sra_eq,     Xprunit::xop_infix_sra,       true  },
                { Token::punctuator_sll_eq,     Xprunit::xop_infix_sll,       true  },
                { Token::punctuator_srl_eq,     Xprunit::xop_infix_srl,       true  },
                { Token::punctuator_assign,     Xprunit::xop_infix_assign,    true  },
                { Token::punctuator_cmp_eq,     Xprunit::xop_infix_cmp_eq,    false },
                { Token::punctuator_cmp_ne,     Xprunit::xop_infix_cmp_ne,    false },
                { Token::punctuator_cmp_lt,     Xprunit::xop_infix_cmp_lt,    false },
                { Token::punctuator_cmp_gt,     Xprunit::xop_infix_cmp_gt,    false },
                { Token::punctuator_cmp_lte,    Xprunit::xop_infix_cmp_lte,   false },
                { Token::punctuator_cmp_gte,    Xprunit::xop_infix_cmp_gte,   false },
                { Token::punctuator_spaceship,  Xprunit::xop_infix_cmp_3way,  false },
              };
            auto qconf = std::find_if(std::begin(s_table), std::end(s_table),
                                      [&](const Punctuator_Table& r) { return alt.punct == r.punct;  });
            if(qconf == std::end(s_table)) {
              return rocket::nullopt;
            }
            // Return the infix operator.
            Infix_Element::S_general elem_c = { qconf->xop, qconf->assign, rocket::clear };
            qelem.emplace(rocket::move(elem_c));
            break;
          }
        default:
          return rocket::nullopt;
        }
        // Discard this token.
        tstrm.shift();
        return qelem;
      }

    Optional<Infix_Element> do_accept_infix_operator_opt(Token_Stream& tstrm)
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

    bool do_accept_expression(Cow_Vector<Xprunit>& units, Token_Stream& tstrm)
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
        Cow_Vector<Infix_Element> stack;
        stack.emplace_back(rocket::move(*qelem));
        for(;;) {
          auto qnext = do_accept_infix_operator_opt(tstrm);
          if(!qnext) {
            break;
          }
          bool succ = do_accept_infix_element(qnext->open_junction(), tstrm);
          if(!succ) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
          // Assignment operations have the lowest precedence and group from right to left.
          auto preced_top = stack.back().tell_precedence();
          if(preced_top < Infix_Element::precedence_assignment) {
            auto preced_next = qnext->tell_precedence();
            // Collapse elements that have no lower precedence and group from left to right.
            while((stack.size() > 1) && (preced_top <= preced_next)) {
              qelem = rocket::move(stack.mut_back());
              stack.pop_back();
              qelem->extract(stack.mut_back().open_junction());
            }
          }
          stack.emplace_back(rocket::move(*qnext));
        }
        while(stack.size() > 1) {
          qelem = rocket::move(stack.mut_back());
          stack.pop_back();
          qelem->extract(stack.mut_back().open_junction());
        }
        stack.mut_front().extract(units);
        return true;
      }

    }  // namespace

Parser_Error Parser::get_parser_error() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_no_data_loaded);
      }
    case state_error:
      {
        return this->m_stor.as<Parser_Error>();
      }
    case state_success:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_success);
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

bool Parser::empty() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return true;
      }
    case state_error:
      {
        return true;
      }
    case state_success:
      {
        return this->m_stor.as<Cow_Vector<Statement>>().empty();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

bool Parser::load(Token_Stream& tstrm, const Parser_Options& /*options*/)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor = nullptr;
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Cow_Vector<Statement> stmts;
    // document ::=
    //   statement-list-opt
    for(;;) {
      auto qstmt = do_accept_statement_opt(tstrm);
      if(!qstmt) {
        break;
      }
      stmts.emplace_back(rocket::move(*qstmt));
    }
    if(!tstrm.empty()) {
      throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
    }
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    this->m_stor = rocket::move(stmts);
    return true;
  } catch(Parser_Error& err) {  // Don't play with this at home.
    ASTERIA_DEBUG_LOG("Caught `Parser_Error`:\n",
                      "line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(), "\n",
                      "code = ", err.code(), ": ", Parser_Error::get_code_description(err.code()));
    this->m_stor = rocket::move(err);
    return false;
  }

void Parser::clear() noexcept
  {
    this->m_stor = nullptr;
  }

const Cow_Vector<Statement>& Parser::get_statements() const
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
        return this->m_stor.as<Cow_Vector<Statement>>();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

}  // namespace Asteria
