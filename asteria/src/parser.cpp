// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "statement.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "expression.hpp"
#include "utilities.hpp"

namespace Asteria {

Parser::~Parser()
  {
  }

namespace {

  Parser_result do_make_result(const Token *qtok_opt, Parser_result::Error error)
    {
      if(!qtok_opt) {
        return Parser_result(0, 0, 0, error);
      }
      return Parser_result(qtok_opt->get_line(), qtok_opt->get_offset(), qtok_opt->get_length(), error);
    }

  void do_tell_source_location(String &file_out, Uint64 &line_out, const Token_stream &toks)
    {
      const auto qtok = toks.peek_opt();
      if(!qtok) {
        return;
      }
      file_out = qtok->get_file();
      line_out = qtok->get_line();
    }

  Parser_result do_match_keyword(Token_stream &toks_inout, Token::Keyword keyword, Parser_result::Error noop_error)
    {
      const auto qtok = toks_inout.peek_opt();
      if(!qtok) {
        return do_make_result(nullptr, noop_error);
      }
      const auto qalt = qtok->opt<Token::S_keyword>();
      if(!qalt) {
        return do_make_result(qtok, noop_error);
      }
      if(qalt->keyword != keyword) {
        return do_make_result(qtok, noop_error);
      }
      toks_inout.shift();
      return do_make_result(nullptr, Parser_result::error_success);
    }

  Parser_result do_match_punctuator(Token_stream &toks_inout, Token::Punctuator punct, Parser_result::Error noop_error)
    {
      const auto qtok = toks_inout.peek_opt();
      if(!qtok) {
        return do_make_result(nullptr, noop_error);
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return do_make_result(qtok, noop_error);
      }
      if(qalt->punct != punct) {
        return do_make_result(qtok, noop_error);
      }
      toks_inout.shift();
      return do_make_result(nullptr, Parser_result::error_success);
    }

  Parser_result do_match_identifier(String &name_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      const auto qtok = toks_inout.peek_opt();
      if(!qtok) {
        return do_make_result(nullptr, noop_error);
      }
      const auto qalt = qtok->opt<Token::S_identifier>();
      if(!qalt) {
        return do_make_result(qtok, noop_error);
      }
      name_out = qalt->name;
      toks_inout.shift();
      return do_make_result(nullptr, Parser_result::error_success);
    }

  Parser_result do_match_string_literal(String &value_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      const auto qtok = toks_inout.peek_opt();
      if(!qtok) {
        return do_make_result(nullptr, noop_error);
      }
      const auto qalt = qtok->opt<Token::S_string_literal>();
      if(!qalt) {
        return do_make_result(qtok, noop_error);
      }
      value_out = qalt->value;
      toks_inout.shift();
      return do_make_result(nullptr, Parser_result::error_success);
    }

  Parser_result do_accept_export_directive(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // export-directive ::=
      //   "export" identifier ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_export, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      String name;
      res = do_match_identifier(name, toks_inout, Parser_result::error_identifier_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_export stmt_c = { std::move(name) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_import_directive(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // import-directive ::=
      //   "import" string-literal ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_import, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      String path;
      res = do_match_string_literal(path, toks_inout, Parser_result::error_string_literal_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_import stmt_c = { std::move(path) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  extern Parser_result do_accept_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error);
  extern Parser_result do_accept_statement(Block &block_out, Token_stream &toks_inout, Parser_result::Error noop_error);

  Parser_result do_accept_statement_list(Vector<Statement> &stmts_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // statement-list-opt ::=
      //   statement-list | ""
      // statement-list ::=
      //   statement statement-list-opt
      Statement stmt;
      auto res = do_accept_statement(stmt, toks_inout, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      stmts_out.emplace_back(std::move(stmt));
      for(;;) {
        res = do_accept_statement(stmt, toks_inout, Parser_result::error_no_operation_performed);
        if(res == Parser_result::error_no_operation_performed) {
          break;
        }
        if(res != Parser_result::error_success) {
          return res;
        }
        stmts_out.emplace_back(std::move(stmt));
      }
      return res;
    }

  Parser_result do_accept_block(Block &block_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // block ::=
      //   "{" statement-list-opt "}"
      auto res = do_match_punctuator(toks_inout, Token::punctuator_brace_op, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Vector<Statement> stmts;
      res = do_accept_statement_list(stmts, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_brace_cl, Parser_result::error_close_brace_or_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      block_out = std::move(stmts);
      return res;
    }

  Parser_result do_accept_null_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // null-statement ::=
      //   ";"
      auto res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_null stmt_c = { };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_expression(Expression &expr_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // TODO
      auto qk = toks_inout.peek_opt();
      if(!qk) {
        return do_make_result(nullptr, noop_error);
      }
      if(qk->index() == Token::index_keyword) {
        return do_make_result(qk, noop_error);
      }
      if(qk->index() == Token::index_punctuator) {
        return do_make_result(qk, noop_error);
      }
      toks_inout.shift();
      return do_make_result(nullptr, Parser_result::error_success);
    }

  Parser_result do_accept_expression_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // expression-statement ::=
      //   expression ";"
      Expression expr;
      auto res = do_accept_expression(expr, toks_inout, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_expr stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_variable_definition(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // variable-definition ::=
      //   "var" identifier equal-initailizer-opt ";"
      // equal-initializer-opt ::=
      //   equal-initializer | ""
      // equal-initializer ::=
      //   "=" expression
      auto res = do_match_keyword(toks_inout, Token::keyword_var, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      String name;
      res = do_match_identifier(name, toks_inout, Parser_result::error_identifier_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression init;
      res = do_match_punctuator(toks_inout, Token::punctuator_assign, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_accept_expression(init, toks_inout, Parser_result::error_expression_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_var_def stmt_c = { std::move(name), false, std::move(init) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_immutable_variable_definition(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // immutable-variable-definition ::=
      //   "const" identifier equal-initailizer ";"
      // equal-initializer ::=
      //   "=" expression
      auto res = do_match_keyword(toks_inout, Token::keyword_const, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      String name;
      res = do_match_identifier(name, toks_inout, Parser_result::error_identifier_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_assign, Parser_result::error_equals_sign_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression init;
      res = do_accept_expression(init, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_var_def stmt_c = { std::move(name), true, std::move(init) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_identifier_list(Vector<String> &names_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // identifier-list-opt ::=
      //   identifier-list | ""
      // identifier-list ::=
      //   identifier ( "," identifier-list | "" )
      String name;
      auto res = do_match_identifier(name, toks_inout, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      names_out.emplace_back(std::move(name));
      for(;;) {
        res = do_match_punctuator(toks_inout, Token::punctuator_comma, Parser_result::error_no_operation_performed);
        if(res == Parser_result::error_no_operation_performed) {
          break;
        }
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_match_identifier(name, toks_inout, Parser_result::error_identifier_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        names_out.emplace_back(std::move(name));
      }
      return res;
    }

  Parser_result do_accept_function_definition(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint64 line = 0;
      do_tell_source_location(file, line, toks_inout);
      // function-definition ::=
      //   "func" identifier "(" identifier-list-opt ")" statement
      auto res = do_match_keyword(toks_inout, Token::keyword_func, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      String name;
      res = do_match_identifier(name, toks_inout, Parser_result::error_identifier_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Vector<String> params;
      res = do_accept_identifier_list(params, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_or_parameter_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body;
      res = do_accept_statement(body, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_func_def stmt_c = { std::move(name), std::move(params), std::move(file), line, std::move(body) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_if_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // if-statement ::=
      //   "if" "(" expression ")" statement ( "else" statement | "" )
      auto res = do_match_keyword(toks_inout, Token::keyword_if, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression cond;
      res = do_accept_expression(cond, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block branch_true;
      res = do_accept_statement(branch_true, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block branch_false;
      res = do_match_keyword(toks_inout, Token::keyword_else, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_accept_statement(branch_false, toks_inout, Parser_result::error_statement_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
      } else {
        res = do_make_result(nullptr, Parser_result::error_success);
      }
      Statement::S_if stmt_c = { std::move(cond), std::move(branch_true), std::move(branch_false) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_switch_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // switch-statement ::=
      //   "switch" "(" expression ")" switch-block
      // switch-block ::=
      //   "{" swtich-clause-list-opt "}"
      // switch-clause-list-opt ::=
      //   switch-clause-list | ""
      // switch-clause-list ::=
      //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
      auto res = do_match_keyword(toks_inout, Token::keyword_switch, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression ctrl;
      res = do_accept_expression(ctrl, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_brace_op, Parser_result::error_open_brace_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Bivector<Expression, Block> clauses;
      for(;;) {
        Expression cond;
        res = do_match_keyword(toks_inout, Token::keyword_case, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
          res = do_accept_expression(cond, toks_inout, Parser_result::error_expression_expected);
          if(res != Parser_result::error_success) {
            return res;
          }
        } else {
          res = do_match_keyword(toks_inout, Token::keyword_default, Parser_result::error_no_operation_performed);
          if(res == Parser_result::error_no_operation_performed) {
            break;
          }
          if(res != Parser_result::error_success) {
            return res;
          }
        }
        res = do_match_punctuator(toks_inout, Token::punctuator_colon, Parser_result::error_colon_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        Vector<Statement> body;
        res = do_accept_statement_list(body, toks_inout, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
        }
        clauses.emplace_back(std::move(cond), std::move(body));
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_brace_cl, Parser_result::error_close_brace_or_switch_clause_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_switch stmt_c = { std::move(ctrl), std::move(clauses) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_do_while_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // do-while-statement ::=
      //   "do" statement "while" "(" expression ")" ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_do, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body;
      res = do_accept_statement(body, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_keyword(toks_inout, Token::keyword_while, Parser_result::error_keyword_while_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression cond;
      res = do_accept_expression(cond, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_while stmt_c = { true, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_while_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // while-statement ::=
      //   "while" "(" expression ")" statement
      auto res = do_match_keyword(toks_inout, Token::keyword_while, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression cond;
      res = do_accept_expression(cond, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body;
      res = do_accept_statement(body, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_while stmt_c = { false, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_for_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // for-statement ::=
      //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
      // for-statement-range ::=
      //   "each" identifier "," identifier ":" expression
      // for-statement-triplet ::=
      //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
      auto res = do_match_keyword(toks_inout, Token::keyword_for, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
      String key_name;
      String mapped_name;
      Statement init_stmt;
      Expression cond;
      Expression step;
      res = do_match_keyword(toks_inout, Token::keyword_each, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_match_identifier(key_name, toks_inout, Parser_result::error_identifier_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_match_punctuator(toks_inout, Token::punctuator_comma, Parser_result::error_comma_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_match_identifier(mapped_name, toks_inout, Parser_result::error_identifier_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_match_punctuator(toks_inout, Token::punctuator_colon, Parser_result::error_colon_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_accept_expression(step, toks_inout, Parser_result::error_expression_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
      } else {
        res = do_accept_variable_definition(init_stmt, toks_inout, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
          goto zin;
        }
        res = do_accept_null_statement(init_stmt, toks_inout, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
          goto zin;
        }
        res = do_accept_expression_statement(init_stmt, toks_inout, Parser_result::error_for_statement_initializer_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
    zin:
        res = do_accept_expression(cond, toks_inout, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
        }
        res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
        if(res != Parser_result::error_success) {
          return res;
        }
        res = do_accept_expression(step, toks_inout, Parser_result::error_no_operation_performed);
        if(res != Parser_result::error_no_operation_performed) {
          if(res != Parser_result::error_success) {
            return res;
          }
        }
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body;
      res = do_accept_statement(body, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      if(key_name.empty()) {
        Vector<Statement> init;
        init.emplace_back(std::move(init_stmt));
        Statement::S_for stmt_c = { std::move(init), std::move(cond), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      } else {
        Statement::S_for_each stmt_c = { std::move(key_name), std::move(mapped_name), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      }
      return res;
    }

  Parser_result do_accept_break_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // break-statement ::=
      //   "break" ( "switch" | "while" | "for" ) ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_break, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::Target target = Statement::target_unspec;
      res = do_match_keyword(toks_inout, Token::keyword_switch, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        target = Statement::target_switch;
        goto z;
      }
      res = do_match_keyword(toks_inout, Token::keyword_while, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        target = Statement::target_while;
        goto z;
      }
      res = do_match_keyword(toks_inout, Token::keyword_for, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        target = Statement::target_for;
        goto z;
      }
    z:
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_break stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_continue_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // continue-statement ::=
      //   "continue" ( "while" | "for" ) ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_continue, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::Target target = Statement::target_unspec;
      res = do_match_keyword(toks_inout, Token::keyword_while, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        target = Statement::target_while;
        goto z;
      }
      res = do_match_keyword(toks_inout, Token::keyword_for, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
        target = Statement::target_for;
        goto z;
      }
    z:
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_continue stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_throw_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // throw-statement ::=
      //   "throw" expression ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_throw, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression expr;
      res = do_accept_expression(expr, toks_inout, Parser_result::error_expression_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_throw stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_return_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // return-statement ::=
      //   "return" expression-opt ";"
      auto res = do_match_keyword(toks_inout, Token::keyword_return, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Expression expr;
      res = do_accept_expression(expr, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res != Parser_result::error_success) {
          return res;
        }
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_semicolon, Parser_result::error_semicolon_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_return stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_try_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // try-statement ::=
      //   "try" statement "catch" "(" identifier ")" statement
      auto res = do_match_keyword(toks_inout, Token::keyword_try, noop_error);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body_try;
      res = do_accept_statement(body_try, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_keyword(toks_inout, Token::keyword_catch, Parser_result::error_keyword_catch_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_op, Parser_result::error_open_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      String except_name;
      res = do_match_identifier(except_name, toks_inout, Parser_result::error_identifier_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      res = do_match_punctuator(toks_inout, Token::punctuator_parenth_cl, Parser_result::error_close_parenthesis_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Block body_catch;
      res = do_accept_statement(body_catch, toks_inout, Parser_result::error_statement_expected);
      if(res != Parser_result::error_success) {
        return res;
      }
      Statement::S_try stmt_c = { std::move(body_try), std::move(except_name), std::move(body_catch) };
      stmt_out = std::move(stmt_c);
      return res;
    }

  Parser_result do_accept_nonblock_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      // non-block-statement ::=
      //   null-statement | expression-statement |
      //   variable-definition | immutable-variable-definition | function-definition |
      //   if-statement | switch-statement |
      //   do-while-statement | while-statement | for-statement |
      //   break-statement | continue-statement | throw-statement | return-statement |
      //   try-statement
      auto res = do_accept_null_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_expression_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_variable_definition(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_immutable_variable_definition(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_function_definition(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_if_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_switch_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_do_while_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_while_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_for_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_break_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_continue_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_throw_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_return_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_try_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      // Be advised that a `return do_make_result(...);` would prevent NRVO.
      res = do_make_result(toks_inout.peek_opt(), noop_error);
      return res;
    }

  Parser_result do_accept_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      ASTERIA_DEBUG_LOG("Looking for a statement as `Statement`: ", *(toks_inout.peek_opt()));
      // statement ::=
      //   block | non-block-statement
      Block block;
      auto res = do_accept_block(block, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res == Parser_result::error_success) {
          Statement::S_block stmt_c = { std::move(block) };
          stmt_out = std::move(stmt_c);
        }
        return res;
      }
      res = do_accept_nonblock_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      // Be advised that a `return do_make_result(...);` would prevent NRVO.
      res = do_make_result(toks_inout.peek_opt(), noop_error);
      return res;
    }

  Parser_result do_accept_statement(Block &block_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      ASTERIA_DEBUG_LOG("Looking for a statement as `Block`: ", *(toks_inout.peek_opt()));
      // statement ::=
      //   block | non-block-statement
      auto res = do_accept_block(block_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      Statement stmt;
      res = do_accept_nonblock_statement(stmt, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        if(res == Parser_result::error_success) {
          Vector<Statement> stmts;
          stmts.emplace_back(std::move(stmt));
          block_out = std::move(stmts);
        }
        return res;
      }
      // Be advised that a `return do_make_result(...);` would prevent NRVO.
      res = do_make_result(toks_inout.peek_opt(), noop_error);
      return res;
    }

  Parser_result do_accept_directive_or_statement(Statement &stmt_out, Token_stream &toks_inout, Parser_result::Error noop_error)
    {
      ASTERIA_DEBUG_LOG("Looking for a directive or statement: ", *(toks_inout.peek_opt()));
      // directive-or-statement ::=
      //   directive | statement
      // directive ::=
      //   export-directive | import-directive
      auto res = do_accept_export_directive(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_import_directive(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      res = do_accept_statement(stmt_out, toks_inout, Parser_result::error_no_operation_performed);
      if(res != Parser_result::error_no_operation_performed) {
        return res;
      }
      // Be advised that a `return do_make_result(...);` would prevent NRVO.
      res = do_make_result(toks_inout.peek_opt(), noop_error);
      return res;
    }

}

Parser_result Parser::load(Token_stream &toks_inout)
  {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor.set(nullptr);
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Vector<Statement> stmts;
    while(toks_inout.size() != 0) {
      // document ::=
      //   directive-or-statement-list-opt
      // directive-or-statement-list-opt ::=
      //   directive-or-statement-list | ""
      // directive-or-statement-list ::=
      //   directive-or-statement directive-or-statement-list-opt
      Statement stmt;
      auto res = do_accept_directive_or_statement(stmt, toks_inout, Parser_result::error_directive_or_statement_expected);
      if(res != Parser_result::error_success) {
        ASTERIA_DEBUG_LOG("Parser error: ", res.get_error(), " (", Parser_result::describe_error(res.get_error()), ")");
        this->m_stor.set(res);
        return res;
      }
      stmts.emplace_back(std::move(stmt));
    }
    // Accept the result.
    this->m_stor.emplace<Block>(std::move(stmts));
    return Parser_result(0, 0, 0, Parser_result::error_success);
  }
void Parser::clear() noexcept
  {
    this->m_stor.set(nullptr);
  }

Parser_result Parser::get_result() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        return this->m_stor.as<Parser_result>();
      }
      case state_success: {
        return Parser_result(0, 0, 0, Parser_result::error_success);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }
const Block & Parser::get_document() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        return this->m_stor.as<Block>();
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }
Block Parser::extract_document()
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        auto block = std::move(this->m_stor.as<Block>());
        this->m_stor.set(nullptr);
        return block;
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

}
